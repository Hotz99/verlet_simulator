#pragma once
#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

struct TaskQueue
{
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::atomic<int> m_remaining_tasks = 0;

    void enqueue_task(std::function<void()> &&callback)
    {
        std::lock_guard<std::mutex> lock_guard{m_mutex};
        m_tasks.push(std::move(callback));
        m_remaining_tasks++;
    }

    void get_task(std::function<void()> &task)
    {
        std::lock_guard<std::mutex> lock_guard{m_mutex};
        if (m_tasks.empty())
            return;
        task = std::move(m_tasks.front());
        m_tasks.pop();
    }

    void await_completion() const
    {
        while (m_remaining_tasks > 0)
        {
            // `yield()` is a hint to the implementation to reschedule the execution of threads
            // essentially forfeits the remainder of the thread's time slice (allocated by the OS)
            std::this_thread::yield();
        }
    }

    void finish_task()
    {
        m_remaining_tasks--;
    }
};

struct Worker
{
    int id = 0;
    std::thread m_thread;
    // `task` is a pointer to a function that takes no arguments and returns nothing
    std::function<void()> task = nullptr;
    bool running = true;
    TaskQueue *task_queue = nullptr;

    Worker(TaskQueue &task_queue_, int id_)
        : id{id_}, task_queue{&task_queue_}
    {
        m_thread = std::thread([this]()
                               { run(); });
    }

    void run()
    {
        while (running)
        {
            task_queue->get_task(task);
            if (task == nullptr)
                std::this_thread::yield();
            else
            {
                task();
                task_queue->finish_task();
                task = nullptr;
            }
        }
    }

    void stop()
    {
        running = false;
        m_thread.join();
    }
};

// `struct` instead of `class` makes all members public, easy access without getters
// perhaps should refactor, open/closed principle would suggest this is a bad design
struct ThreadPool
{
    TaskQueue m_task_queue;
    int m_thread_count = 1;
    std::vector<Worker> m_workers;

    ThreadPool(int thread_count_)
        : m_thread_count{thread_count_}
    {
        m_workers.reserve(thread_count_);
        for (int i = 0; i < thread_count_; i++)
            m_workers.emplace_back(m_task_queue, i);
    }

    // `&&` denotes rvalue reference, allows moving the lambda instead of copying
    void parallel(int entity_count, std::function<void(int start, int end)> &&callback)
    {
        int slice_size = entity_count / m_thread_count;
        for (int i = 0; i < m_thread_count; i++)
        {
            int start = i * slice_size;
            int end = start + slice_size;
            m_task_queue.enqueue_task([start, end, &callback]()
                                      { callback(start, end); });
        }
        if (slice_size * m_thread_count < entity_count)
        {
            int start = slice_size * m_thread_count;
            callback(start, entity_count);
        }
        m_task_queue.await_completion();
    }
};
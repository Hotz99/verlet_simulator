#pragma once

#include <vector>
#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include "./particle.hpp"
#include "../thread_pool.hpp"

class Simulator
{
private:
    // TODO test diff values
    // if too strong, particles will skip cells
    float GRAVITY_STRENGTH = 200.0f;
    sf::Vector2f gravity = {0.0f, GRAVITY_STRENGTH};

    float step_dt = 1.0f / 60;
    int sub_steps = 8;

    // ctor initializes `window_size` and `grid_cell_size`
    float window_size;
    float grid_cell_size;
    // 2d vector where a cell is `vector<int>` of entity ids
    // so it's really a 3d vector
    std::vector<int> grid[350][350];
    int grid_cell_count = window_size / grid_cell_size;

    const float bounce_factor = 0.66f;

    ThreadPool &thread_pool;

    void resolve_boundary_collision(int entity_id)
    {
        Particle &ent = entities[entity_id];
        const sf::Vector2f pos = ent.position;
        sf::Vector2f new_pos = ent.position;
        sf::Vector2f vel = ent.get_velocity();

        sf::Vector2f dx = {-vel.x, vel.y};
        // bounce off left/right
        if (pos.x < grid_cell_size || pos.x > window_size - grid_cell_size)
        {
            if (pos.x < grid_cell_size)
                new_pos.x = grid_cell_size;
            if (pos.x > window_size - grid_cell_size)
                new_pos.x = window_size - grid_cell_size;

            ent.position = new_pos;
            ent.set_velocity(dx * bounce_factor, 1.0);
        }

        sf::Vector2f dy = {vel.x, -vel.y};
        // bounce off top/bottom
        if (pos.y < grid_cell_size || pos.y > window_size - grid_cell_size)
        {
            if (pos.y < grid_cell_size)
                new_pos.y = grid_cell_size;
            if (pos.y > window_size - grid_cell_size)
                new_pos.y = window_size - grid_cell_size;
            ent.position = new_pos;
            ent.set_velocity(dy * bounce_factor, 1.0);
        }
    }

    void resolve_cell_collisions(int row_1, int col_1, int row_2, int col_2)
    {
        for (int id_1 : grid[row_1][col_1])
        {
            Particle &ent_1 = entities[id_1];
            for (int id_2 : grid[row_2][col_2])
            {
                if (id_1 == id_2)
                    continue;

                Particle &ent_2 = entities[id_2];
                sf::Vector2f v = ent_1.position - ent_2.position;
                float dist = v.x * v.x + v.y * v.y;

                float min_dist = grid_cell_size;

                if (dist >= min_dist * min_dist)
                    continue;

                dist = sqrt(dist);
                float delta = 0.25f * (min_dist - dist);
                sf::Vector2f n = v / dist * delta;

                ent_1.position += n;
                ent_2.position -= n;

                // particles with greater mass move less when colliding
                // not relevant for our case since all particles have the same mass
                // but allows for future extension
                // ent_1.position += n * (1 - mass_ratio) * delta;
                // ent_2.position += -(n * mass_ratio * delta);
            }
        }
    }

    void process_grid_slice(int left_col, int right_col)
    {
        // https://en.wikipedia.org/wiki/Five-point_stencil
        int col_deltas[] = {1, 1, 0, 0, -1};
        int row_deltas[] = {0, 1, 0, 1, 1};
        int stencil_count = 5;

        // causes issues, likely due to race conditions on overlapping corner cells ?
        // https://en.wikipedia.org/wiki/Nine-point_stencil
        // int col_deltas[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        // int row_deltas[] = {-1, -1, -1, 0, 0, 1, 1, 1};
        // int stencil_count = 9;

        for (int col_1 = left_col; col_1 < right_col; col_1++)
        {
            for (int row_1 = 0; row_1 < grid_cell_count; row_1++)
            {
                if (grid[col_1][row_1].empty())
                    continue;

                for (int delta_idx = 0; delta_idx < stencil_count; delta_idx++)
                {
                    int col_2 = col_1 + col_deltas[delta_idx];
                    int row_2 = row_1 + row_deltas[delta_idx];

                    if (col_2 < 0 || row_2 < 0 || col_2 >= grid_cell_count || row_2 >= grid_cell_count)
                        continue;

                    resolve_cell_collisions(col_1, row_1, col_2, row_2);
                }
            }
        }
    }

    void resolve_particle_collisions()
    {
        int slice_count = thread_pool.m_thread_count * 2;
        int slice_size = grid_cell_count / slice_count;

        // perform two passes to avoid race conditions on overlapping cells-to-be-processed between threads
        // each slice is processed by a separate thread
        // then a slice is halved, we process the left half first, then the right half

        // left pass
        for (int i = 0; i < thread_pool.m_thread_count; i++)
        {
            thread_pool.m_task_queue.enqueue_task([this, i, slice_size]
                                                  {
                int slice_start = 2 * i * slice_size;
                int slice_end = slice_start + slice_size;
                process_grid_slice(slice_start, slice_end); });
        }

        // if grid_cell_count is not divisible by slice_count
        // create task for processing remaining cells
        if (slice_count * slice_size < grid_cell_count)
        {
            thread_pool.m_task_queue.enqueue_task([this, slice_count, slice_size]
                                                  { process_grid_slice(slice_count * slice_size, grid_cell_count); });
        }

        thread_pool.m_task_queue.await_completion();

        // right pass
        for (int i = 0; i < thread_pool.m_thread_count; i++)
        {
            thread_pool.m_task_queue.enqueue_task([this, i, slice_size]
                                                  {
                int slice_start = (2 * i + 1) * slice_size;
                int slice_end = slice_start + slice_size;
                process_grid_slice(slice_start, slice_end); });
        }

        thread_pool.m_task_queue.await_completion();
    }

    // logic to be parallelized
    void update_entities_thread(int start_id, int end_id, float dt)
    {
        for (int i = start_id; i < end_id; i++)
        {
            Particle &ent = entities[i];
            ent.update_position(dt);
            ent.grid_row = ent.position.x / grid_cell_size;
            ent.grid_col = ent.position.y / grid_cell_size;
            sf::Vector2f vel = ent.get_velocity();

            // prevent particles from skipping cells when moving fast
            // avoids missed collisions with other particles or window boundaries
            if (vel.x * vel.x + vel.y * vel.y > grid_cell_size)
                ent.set_velocity({0.0f, 0.0f}, 1.0);
        }
    }

    void update_grid()
    {
        for (int i = 0; i < grid_cell_count; i++)
            for (int j = 0; j < grid_cell_count; j++)
                grid[i][j].clear();

        for (Particle &ent : entities)
        {
            if (ent.grid_row < 0 || ent.grid_col < 0 || ent.grid_row >= grid_cell_count || ent.grid_col >= grid_cell_count)
                continue;
            grid[ent.grid_row][ent.grid_col].push_back(ent.id);
        }
    }

public:
    std::vector<Particle> entities;

    Simulator(float window_size_, float radius, ThreadPool &thread_pool_)
        : window_size{window_size_}, grid_cell_size{2 * radius}, thread_pool{thread_pool_}
    {
    }

    virtual ~Simulator()
    {
        for (Worker &worker : thread_pool.m_workers)
        {
            worker.stop();
        }
    }

    Particle &add_entity(sf::Vector2f position, float radius)
    {
        int grid_row = position.x / grid_cell_size;
        int grid_col = position.x / grid_cell_size;

        Particle new_particle = Particle(position, radius, grid_row, grid_col, entities.size());
        // `push_back` copies the object
        grid[grid_row][grid_col].push_back(new_particle.id);

        // `emplace_back` constructs the object in-place, redundant since new_particle already exists
        // but also returns a reference to the constructed object, so usefull here
        // q: not sure if returning a reference to the local variable `new_particle` is safe, Rust would not allow it (without valid lifetime definition)
        // a: local variables are destroyed when function returns, so a reference to a local variable would become a dangling pointer
        return entities.emplace_back(new_particle);
    }

    void update()
    {
        float substep_dt = step_dt / sub_steps;
        for (int i = 0; i < sub_steps; i++)
        {
            for (auto &ent : entities)
            {
                ent.apply_force(gravity);
            }

            resolve_particle_collisions();

            // enforce window boundaries
            thread_pool.parallel(entities.size(), [&](int start, int end)
                                 {
            for (int i = start; i < end; i++) resolve_boundary_collision(i); });

            // update entity positions with verlet integration
            thread_pool.parallel(entities.size(), [&](int start, int end)
                                 { update_entities_thread(start, end, substep_dt); });

            update_grid();
        }
    }

    void mouse_pull(sf::Vector2f mouse_pos, float radius)
    {
        for (Particle &ent : entities)
        {
            sf::Vector2f dir = mouse_pos - ent.position;
            float dist = sqrt(dir.x * dir.x + dir.y * dir.y);
            ent.apply_force(dir * std::max(0.0f, 5 * (radius - dist)));
        }
    }

    void mouse_push(sf::Vector2f mouse_pos, float radius)
    {
        for (Particle &ent : entities)
        {
            sf::Vector2f dir = mouse_pos - ent.position;
            float dist = sqrt(dir.x * dir.x + dir.y * dir.y);
            ent.apply_force(dir * std::min(0.0f, -5 * (radius - dist)));
        }
    }

    void set_entity_velocity(Particle &entity, sf::Vector2f vel)
    {
        entity.set_velocity(vel, step_dt / sub_steps);
    }

    void set_up_gravity()
    {
        gravity = {0.0f, -GRAVITY_STRENGTH};
    }

    void set_down_gravity()
    {
        gravity = {0.0f, GRAVITY_STRENGTH};
    }

    void set_left_gravity()
    {
        gravity = {-GRAVITY_STRENGTH, 0.0f};
    }

    void set_right_gravity()
    {
        gravity = {GRAVITY_STRENGTH, 0.0f};
    }
};
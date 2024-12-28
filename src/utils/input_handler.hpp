#include "../physics/simulator.hpp"

class InputHandler
{
private:
    const float m_push_radius = 120.0f;
    const float m_pull_radius = 120.0f;

    Simulator &simulator;
    sf::RenderWindow &window;
    const float window_width;

public:
    InputHandler(Simulator &solver_, sf::RenderWindow &window_, float window_width_)
        : simulator{solver_},
          window{window_},
          window_width{window_width_} {};

    void handle_input()
    {
        sf::Event event{};
        // `pollEvent()` is non-blocking
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
            {
                window.close();
            }
        }

        // react to mouse input
        // ratio ensures mouse coordinates are scaled correctly when window is resized
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        {
            float ratio = window_width / window.getSize().x;
            // `static_cast<T_2>(t_1)` is C++'s equivalent of C-style cast `(T_2)t_1`, entails compile-time type checking
            // we cast within the same level of inheritance (sf::Vector2i to sf:Vector2f)
            // so no need for `dynamic_cast` (entails runtime type checking)
            sf::Vector2f mouse_window_coords = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window)) * ratio;
            simulator.mouse_pull(mouse_window_coords, m_pull_radius);
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
        {
            float ratio = window_width / window.getSize().x;
            sf::Vector2f mouse_window_coords = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window)) * ratio;
            simulator.mouse_push(mouse_window_coords, m_push_radius);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            std::this_thread::sleep_for(std::chrono::milliseconds(360));
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            simulator.set_up_gravity();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            simulator.set_down_gravity();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            simulator.set_left_gravity();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            simulator.set_right_gravity();
    };
};
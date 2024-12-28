#pragma once
#include <vector>
#include <iostream>
#include <math.h>
#include <SFML/Graphics.hpp>
struct Particle
{
    sf::Vector2f position;
    sf::Vector2f position_last;
    sf::Vector2f acceleration;
    float radius;
    sf::Color color;
    int grid_row, grid_col, id;

    // `default` denotes default ctor and initilizes members with their default values
    Particle() = default;

    // args suffixed with `_` to avoid shadowing
    // https://en.wikipedia.org/wiki/Variable_shadowing
    Particle(sf::Vector2f position_, float radius_, int grid_x_, int grid_y_, int id_)
        : position{position_},
          position_last{position_},
          acceleration{0.0f, 0.0f},
          radius{radius_},
          grid_row{grid_x_},
          grid_col{grid_y_},
          id{id_}
    {
    }

    // https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
    void update_position(float dt)
    {
        // https://en.wikipedia.org/wiki/Displacement
        sf::Vector2f displacement = position - position_last;
        position_last = position;
        position = position + displacement + acceleration * (dt * dt);
        // reset acceleration
        acceleration = {0.0f, 0.0f};
    }

    void apply_force(sf::Vector2f v)
    {
        acceleration += v;
    }

    sf::Vector2f get_velocity()
    {
        return position - position_last;
    }

    void set_velocity(sf::Vector2f v, float dt)
    {
        position_last = position - (v * dt);
    }

    void add_velocity(sf::Vector2f v, float dt)
    {
        position_last -= v * dt;
    }
};
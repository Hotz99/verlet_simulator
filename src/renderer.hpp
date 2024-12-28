#pragma once
#include "./physics/simulator.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include "./thread_pool.hpp"

static const std::string CIRCLE_TEXTURE_PATH = "../assets/circle.png";

class Renderer
{
private:
    sf::RenderWindow &render_target;
    Simulator &simulator;
    ThreadPool &thread_pool;

    sf::Texture m_entity_texture;
    sf::VertexArray m_entity_vertex_array{sf::Quads};

public:
    Renderer(sf::RenderWindow &window_, ThreadPool &thread_pool_, Simulator &solver_)
        : render_target{window_}, simulator{solver_}, thread_pool{thread_pool_}
    {
        m_entity_texture.loadFromFile(CIRCLE_TEXTURE_PATH);
        m_entity_texture.generateMipmap();
        m_entity_texture.setSmooth(true);
    }

    void new_render()
    {
        render_target.clear(sf::Color::Black);
        update_vertex_array();

        sf::RenderStates render_states;
        render_states.texture = &m_entity_texture;
        render_target.draw(m_entity_vertex_array, render_states);
    }

    void update_vertex_array()
    {
        m_entity_vertex_array.resize(simulator.entities.size() * 4);
        const float texture_size = 1024.0f;
        // radius is the same for all entities
        const float radius = simulator.entities[0].radius;

        thread_pool.parallel(simulator.entities.size(), [&](int start, int end)
                             {
            for (int i = start; i < end; i++) {
                const Particle& ent = simulator.entities[i];
                const int id = i * 4;
                m_entity_vertex_array[id    ].position = ent.position + sf::Vector2f{-radius, -radius};
                m_entity_vertex_array[id + 1].position = ent.position + sf::Vector2f{ radius, -radius};
                m_entity_vertex_array[id + 2].position = ent.position + sf::Vector2f{ radius,  radius};
                m_entity_vertex_array[id + 3].position = ent.position + sf::Vector2f{-radius,  radius}; 

                m_entity_vertex_array[id    ].texCoords = {0.0f, 0.0f};
                m_entity_vertex_array[id + 1].texCoords = {texture_size, 0.0f};
                m_entity_vertex_array[id + 2].texCoords = {texture_size, texture_size};
                m_entity_vertex_array[id + 3].texCoords = {0.0f, texture_size};

                const sf::Color color = ent.color;
                m_entity_vertex_array[id    ].color = color;
                m_entity_vertex_array[id + 1].color = color;
                m_entity_vertex_array[id + 2].color = color;
                m_entity_vertex_array[id + 3].color = color;
            } });
    }
};
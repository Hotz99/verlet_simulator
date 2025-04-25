#pragma once
#include <SFML/Graphics.hpp>
#include <string>

#include "./physics/simulator.hpp"
#include "./thread_pool.hpp"

static const std::string CIRCLE_TEXTURE_PATH = "./assets/circle.png";

class Renderer {
private:
  sf::RenderWindow &render_target;
  Simulator &simulator;
  ThreadPool &thread_pool;

  sf::Texture m_entity_texture;

  // 2 triangles per entity
  sf::VertexArray m_entity_vertex_array{sf::PrimitiveType::Triangles};

public:
  Renderer(sf::RenderWindow &window_, ThreadPool &thread_pool_,
           Simulator &solver_)
      : render_target{window_}, simulator{solver_}, thread_pool{thread_pool_} {
    m_entity_texture.loadFromFile(CIRCLE_TEXTURE_PATH);
    m_entity_texture.generateMipmap();
    m_entity_texture.setSmooth(true);
  }

  void new_render() {
    render_target.clear(sf::Color::Black);
    update_vertex_array();

    sf::RenderStates render_states;
    render_states.texture = &m_entity_texture;
    render_target.draw(m_entity_vertex_array, render_states);
  }

  void update_vertex_array() {
    size_t entityCount = simulator.entities.size();

    // 6 vertices per 2 triangles
    m_entity_vertex_array.resize(entityCount * 6);
    const float texture_size = 1024.0f;

    // assuming all have same radius
    const float radius =
        simulator.entities.empty() ? 0.0f : simulator.entities[0].radius;

    thread_pool.parallel(entityCount, [&](int start, int end) {
      for (int i = start; i < end; i++) {
        const Particle &ent = simulator.entities[i];
        int id = i * 6;
        sf::Vector2f topLeft = ent.position + sf::Vector2f{-radius, -radius};
        sf::Vector2f topRight = ent.position + sf::Vector2f{radius, -radius};
        sf::Vector2f bottomRight = ent.position + sf::Vector2f{radius, radius};
        sf::Vector2f bottomLeft = ent.position + sf::Vector2f{-radius, radius};

        // triangle 1
        m_entity_vertex_array[id].position = topLeft;
        m_entity_vertex_array[id].texCoords = {0.0f, 0.0f};

        m_entity_vertex_array[id + 1].position = topRight;
        m_entity_vertex_array[id + 1].texCoords = {texture_size, 0.0f};

        m_entity_vertex_array[id + 2].position = bottomRight;
        m_entity_vertex_array[id + 2].texCoords = {texture_size, texture_size};

        // triangle 2
        m_entity_vertex_array[id + 3].position = bottomRight;
        m_entity_vertex_array[id + 3].texCoords = {texture_size, texture_size};

        m_entity_vertex_array[id + 4].position = bottomLeft;
        m_entity_vertex_array[id + 4].texCoords = {0.0f, texture_size};

        m_entity_vertex_array[id + 5].position = topLeft;
        m_entity_vertex_array[id + 5].texCoords = {0.0f, 0.0f};

        // color for all 6 vertices
        for (int j = 0; j < 6; ++j) {
          m_entity_vertex_array[id + j].color = ent.color;
        }
      }
    });
  }
};

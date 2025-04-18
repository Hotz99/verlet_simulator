#include <iostream>
#include <math.h>
#include <thread>

#include <SFML/Graphics.hpp>

#include "./physics/simulator.hpp"
#include "./renderer.hpp"
#include "./thread_pool.hpp"
#include "./utils/color_utils.hpp"
#include "./utils/input_handler.hpp"

static const std::string UI_FONT_PATH = "../assets/dejavu_sans.ttf";

int main() {
  // TODO finish deterministic rendering
  // freopen("colors.txt", "r", stdin);
  // freopen("positions.txt", "w", stdout);

  constexpr unsigned int window_width = 512;
  constexpr unsigned int window_height = 512;

  const float particle_radius = 2.0f;
  // min 60fps on ryzen 5800x: ~14500 entities, 512x512 window, 16 worker
  // threads
  const int max_entities = 12000;

  const float spawn_velocity = 500.0f;
  const float spawner_spacing = 4.0f;
  const int spawner_count = 20;

  const sf::Vector2f spawn_pos_1 = {4.0f, 50.0f};
  // `- particle_radius + spawner_spacing` to offset the discrepancy from the
  // spawn logic
  const sf::Vector2f spawn_pos_2 = {
      window_width - 4.0f, 50.0f - (particle_radius + spawner_spacing)};
  const sf::Vector2f spawn_angle_1 = sf::Vector2f{0.5, 0.5};
  const sf::Vector2f spawn_angle_2 = sf::Vector2f{-0.5, 0.5};

  sf::ContextSettings settings;
  settings.antiAliasingLevel = 1;
  sf::RenderWindow window(
      sf::VideoMode(sf::Vector2u{window_width, window_height}),
      "Verlet Simulator", sf::State::Windowed, settings);

  int x = (2560 - window_width) / 2;
  int y = (1440 - window_height) / 2;
  window.setPosition(sf::Vector2i(x, y));

  const int frame_rate = 60;
  window.setFramerateLimit(frame_rate);

  int available_thread_count = std::thread::hardware_concurrency();

  std::cout << "employing " << available_thread_count << " threads\n"
            << std::endl;

  ThreadPool thread_pool(available_thread_count);
  Simulator simulator(window_width, particle_radius, thread_pool);
  Renderer renderer(window, thread_pool, simulator);
  InputHandler input_handler(simulator, window, window_width);

  sf::Clock timer, fps_timer;
  int r, g, b;
  sf::Font ui_font;
  ui_font.openFromFile(UI_FONT_PATH);

  int start_up = 0;
  while (window.isOpen()) {

    int entity_count = simulator.entities.size();
    if (entity_count < max_entities) {
      sf::Color rgb_color =
          color_utils::get_time_based_rgb(timer.getElapsedTime().asSeconds());

      for (int spawner_idx = 0;
           spawner_idx < std::min(spawner_count, max_entities - entity_count);
           spawner_idx++) {
        sf::Vector2f spawner_offset =
            sf::Vector2f{0.0f, spawner_idx * spawner_spacing};
        sf::Vector2f spawn_pos =
            (spawner_idx % 2 == 0) ? spawn_pos_1 : spawn_pos_2;
        sf::Vector2f spawn_angle =
            (spawner_idx % 2 == 0) ? spawn_angle_1 : spawn_angle_2;

        auto &new_entity =
            simulator.add_entity(spawn_pos + spawner_offset, particle_radius);

        // std::cin >> r >> g >> b;
        // new_entity.color = {static_cast<uint8_t>(r), static_cast<uint8_t>(g),
        // static_cast<uint8_t>(b)};

        new_entity.color = rgb_color;
        simulator.set_entity_velocity(new_entity, spawn_velocity * spawn_angle);
      }
    }

    input_handler.handle_input();

    fps_timer.restart();
    simulator.update();
    float render_time_ms =
        1.0 * fps_timer.getElapsedTime().asMicroseconds() / 1000;
    renderer.new_render();

    // draw performance metrics
    sf::Text metrics{ui_font};

    metrics.setString(std::to_string(render_time_ms) + "ms render, " +
                      std::to_string(simulator.entities.size()) + " particles");
    metrics.setCharacterSize(18);
    metrics.setFillColor(sf::Color::White);
    window.draw(metrics);

    window.display();
  }
  return 0;
}

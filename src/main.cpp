#include <iostream>
#include <math.h>
#include <thread>

#include <SFML/Graphics.hpp>

#include "./physics/simulator.hpp"
#include "./renderer.hpp"
#include "./thread_pool.hpp"
#include "./utils/color_utils.hpp"
#include "./utils/input_handler.hpp"

static const std::string UI_FONT_PATH = "./assets/dejavu_sans.ttf";
constexpr unsigned WINDOW_WIDTH = 512;
constexpr unsigned WINDOW_HEIGHT = 512;
constexpr float PARTICLE_RADIUS = 2.0f;
// min 60fps on ryzen 5800x: ~14500 entities, 512x512 window, 16 threads
constexpr unsigned MAX_ENTITIES = 12000;

constexpr float SPAWN_VELOCITY = 500.0f;
constexpr float SPAWNER_SPACING = 4.0f;
constexpr unsigned SPAWNER_COUNT = 20;

int main() {
  // TODO finish deterministic rendering
  // freopen("colors.txt", "r", stdin);
  // freopen("positions.txt", "w", stdout);

  const sf::Vector2f spawn_pos_1 = {4.0f, 50.0f};
  // `- particle_radius + spawner_spacing` to offset the discrepancy from the
  // spawn logic
  const sf::Vector2f spawn_pos_2 = {
      WINDOW_WIDTH - 4.0f, 50.0f - (PARTICLE_RADIUS + SPAWNER_SPACING)};
  const sf::Vector2f spawn_angle_1 = sf::Vector2f{0.5, 0.5};
  const sf::Vector2f spawn_angle_2 = sf::Vector2f{-0.5, 0.5};

  sf::ContextSettings settings;
  settings.antiAliasingLevel = 1;
  sf::RenderWindow window(
      sf::VideoMode(sf::Vector2u{WINDOW_WIDTH, WINDOW_HEIGHT}),
      "Verlet Simulator", sf::State::Windowed, settings);

  int x = (2560 - WINDOW_WIDTH) / 2;
  int y = (1440 - WINDOW_HEIGHT) / 2;
  window.setPosition(sf::Vector2i(x, y));

  const int frame_rate = 60;
  window.setFramerateLimit(frame_rate);

  int available_thread_count = std::thread::hardware_concurrency();

  std::cout << "employing " << available_thread_count << " threads\n"
            << std::endl;

  ThreadPool thread_pool(available_thread_count);
  Simulator simulator(WINDOW_WIDTH, PARTICLE_RADIUS, thread_pool);
  Renderer renderer(window, thread_pool, simulator);
  InputHandler input_handler(simulator, window, WINDOW_WIDTH);

  sf::Clock timer, fps_timer;
  int r, g, b;
  sf::Font ui_font;
  ui_font.openFromFile(UI_FONT_PATH);

  while (window.isOpen()) {
    unsigned entity_count = simulator.entities.size();
    if (entity_count < MAX_ENTITIES) {
      sf::Color rgb_color =
          color_utils::get_time_based_rgb(timer.getElapsedTime().asSeconds());

      for (unsigned spawner_idx = 0;
           spawner_idx < std::min(SPAWNER_COUNT, MAX_ENTITIES - entity_count);
           spawner_idx++) {
        sf::Vector2f spawner_offset =
            sf::Vector2f{0.0f, spawner_idx * SPAWNER_SPACING};
        sf::Vector2f spawn_pos =
            (spawner_idx % 2 == 0) ? spawn_pos_1 : spawn_pos_2;
        sf::Vector2f spawn_angle =
            (spawner_idx % 2 == 0) ? spawn_angle_1 : spawn_angle_2;

        auto &new_entity =
            simulator.add_entity(spawn_pos + spawner_offset, PARTICLE_RADIUS);

        // std::cin >> r >> g >> b;
        // new_entity.color = {static_cast<uint8_t>(r), static_cast<uint8_t>(g),
        // static_cast<uint8_t>(b)};

        new_entity.color = rgb_color;
        simulator.set_entity_velocity(new_entity, SPAWN_VELOCITY * spawn_angle);
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

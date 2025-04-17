#include "./physics/particle.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>

const std::string UI_FONT_PATH = "../assets/dejavu_sans.ttf";
const unsigned UI_FONT_SIZE = 18;
sf::Font ui_font;

struct PartitionLine
{
  sf::Vector2f start;
  sf::Vector2f end;
};

// resolve the median of axis (x or y) for the given range of particles
float resolve_median(const std::vector<std::shared_ptr<Particle>> &particles,
                     char axis, size_t start_idx, size_t end_idx)
{
  std::vector<float> values;
  for (size_t i = start_idx; i < end_idx; ++i)
  {
    values.push_back(axis == 'x' ? particles[i]->position.x
                                 : particles[i]->position.y);
  }

  const auto median = values.begin() + values.size() / 2;
  std::nth_element(values.begin(), median, values.end());

  return *median;
}

void orb_partition(std::vector<std::shared_ptr<Particle>> &particles,
                   size_t start_idx, size_t end_idx, const sf::FloatRect &partition,
                   int depth, std::vector<PartitionLine> &partition_lines,
                   std::vector<sf::Text> &partition_labels,
                   sf::RenderWindow &window)
{
  sf::Text label(ui_font, "", UI_FONT_SIZE);
  label.setFillColor(sf::Color::White);
  label.setString(std::to_string(depth));
  label.setPosition({partition.position.x + 8.0f, partition.position.y + 6.0f});
  partition_labels.push_back(label);

  // base case: if 0 or 1 particles in this partition
  if (end_idx - start_idx <= 1)
    return;

  const bool vertical = depth % 2 == 0;
  const float median =
      resolve_median(particles, vertical ? 'x' : 'y', start_idx, end_idx);

  PartitionLine split_line;
  if (vertical)
  {
    split_line = {{median, partition.position.y},
                  {median, partition.position.y + partition.size.y}};
  }
  else
  {
    split_line = {
        {partition.position.x, median},
        {partition.position.x + partition.size.x,
         median},
    };
  }
  partition_lines.push_back(split_line);

  // partition particles with proper boundary checks
  auto median_partition = std::partition(particles.begin() + start_idx, particles.begin() + end_idx,
                                         [&](const auto &p)
                                         {
                                           if (vertical)
                                             return p->position.x < median;
                                           else
                                             return p->position.y < median;
                                         });

  // convert iterator to index, relative to start of particles array
  const size_t mid_idx = std::distance(particles.begin(), median_partition);

  // build geometrical representations of sub-partitions
  if (vertical)
  {
    // from original left to median
    sf::FloatRect left_partition({partition.position.x,          // left
                                  partition.position.y},         // top
                                 {median - partition.position.x, // width
                                  partition.size.y}              // height is same as parent
    );

    // from median to original right
    sf::FloatRect right_partition(
        {median,                                             // left
         partition.position.y},                              // top
        {(partition.position.x + partition.size.x) - median, // width
         partition.size.y}                                   // height is same as parent
    );

    orb_partition(particles, start_idx, mid_idx, left_partition, depth + 1, partition_lines,
                  partition_labels, window);
    orb_partition(particles, mid_idx, end_idx, right_partition, depth + 1, partition_lines,
                  partition_labels, window);
  }
  else
  {
    // from original top to median
    sf::FloatRect top_partition({partition.position.x,          // left
                                 partition.position.y},         // top
                                {partition.size.x,              // width is same as parent
                                 median - partition.position.y} // height
    );

    // from median to original bottom
    sf::FloatRect bottom_partition(
        {partition.position.x,                               // left
         median},                                            // top
        {partition.size.x,                                   // width
         (partition.position.y + partition.size.y) - median} // height
    );

    orb_partition(particles, start_idx, mid_idx, top_partition, depth + 1, partition_lines, partition_labels, window);
    orb_partition(particles, mid_idx, end_idx, bottom_partition, depth + 1, partition_lines, partition_labels, window);
  }
}

void setupTestParticles(std::vector<std::shared_ptr<Particle>> &particles)
{
  particles.clear();
  std::vector<sf::Vector2f> positions = {
      {100.f, 100.f}, {150.f, 150.f}, {120.f, 200.f}, // top-left
      {500.f, 100.f},
      {600.f, 120.f}, // top-right
      {100.f, 400.f},
      {150.f, 500.f},
      {200.f, 450.f}, // bottom-left
      {600.f, 400.f},
      {550.f, 500.f}, // bottom-right

      // {200.f, 150.f}, // center of top-left quadrant
      // {600.f, 150.f}, // center of top-right quadrant
      // {200.f, 450.f}, // center of bottom-left quadrant
      // {600.f, 450.f}  // center of bottom-right quadrant
  };

  for (const auto &pos : positions)
  {
    auto p = std::make_shared<Particle>();
    p->radius = 4.0f;
    p->position = pos;
    p->color = sf::Color::White;
    particles.push_back(p);
  }
}

int main()
{
  constexpr unsigned WINDOW_WIDTH = 800;
  constexpr unsigned WINDOW_HEIGHT = 600;

  sf::RenderWindow window(
      sf::VideoMode(sf::Vector2u{static_cast<unsigned>(WINDOW_WIDTH),
                                 static_cast<unsigned>(WINDOW_HEIGHT)}),
      "ORB Visualization");

  window.setVerticalSyncEnabled(true);
  ui_font.openFromFile(UI_FONT_PATH);
  // TODO why window_size = 512 ?
  int x = (2560 - 512) / 2;
  int y = (1440 - 512) / 2;
  window.setPosition(sf::Vector2i(x, y));

  std::vector<std::shared_ptr<Particle>> particles;
  setupTestParticles(particles);

  std::vector<PartitionLine> splitLines;
  std::vector<sf::Text> partition_labels;

  unsigned frameCount = 0;
  bool render_once = false;

  while (window.isOpen())
  {
    while (const std::optional event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>() ||
          (event->is<sf::Event::KeyPressed>() &&
           event->getIf<sf::Event::KeyPressed>()->code ==
               sf::Keyboard::Key::Escape))
      {
        window.close();
        break;
      }
    }

    if (render_once)
      continue;

    if (frameCount % 60 == 0)
    {
      orb_partition(particles, 0, particles.size(),
                    sf::FloatRect({0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}), 0,
                    splitLines, partition_labels, window);

      std::cout << "partitioning complete" << std::endl;
    }

    window.clear(sf::Color::Black);

    for (const auto &p : particles)
    {
      sf::CircleShape dot(p->radius);
      dot.setPosition(p->position);
      dot.setFillColor(p->color);
      window.draw(dot);
    }

    sf::VertexArray splitLinesVA(sf::PrimitiveType::Lines);
    for (const auto &s : splitLines)
    {
      splitLinesVA.append(sf::Vertex{s.start, sf::Color::Red});
      splitLinesVA.append(sf::Vertex{s.end, sf::Color::Red});
    }
    window.draw(splitLinesVA);

    for (const auto &label : partition_labels)
      window.draw(label);

    window.display();
    frameCount++;

    // TODO remove this
    render_once = true;
  }
  return 0;
}

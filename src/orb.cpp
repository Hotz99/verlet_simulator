#include "./physics/particle.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>

const std::string UI_FONT_PATH = "../assets/dejavu_sans.ttf";
sf::Font ui_font;

struct PartitionLine
{
  sf::Vector2f start;
  sf::Vector2f end;
};

float computeMedian(const std::vector<std::shared_ptr<Particle>> &particles,
                    char axis, size_t begin, size_t end)
{
  std::vector<float> values;
  for (size_t i = begin; i < end; ++i)
  {
    values.push_back(axis == 'x' ? particles[i]->position.x
                                 : particles[i]->position.y);
  }

  const auto middle = values.begin() + values.size() / 2;
  std::nth_element(values.begin(), middle, values.end());

  return *middle;
}

void orbPartition(std::vector<std::shared_ptr<Particle>> &particles,
                  size_t begin, size_t end, const sf::FloatRect &partition,
                  int depth, std::vector<PartitionLine> &partition_lines,
                  std::vector<sf::Text> &partition_labels,
                  sf::RenderWindow &window)
{
  std::cout << "depth is " << depth << std::endl;

  sf::Text label(ui_font, "", 24);
  label.setFillColor(sf::Color::White);
  label.setString(std::to_string(depth));
  label.setPosition({partition.position.x + 10.0f, partition.position.y + 6.0f});
  partition_labels.push_back(label);

  // base case: if 0 or 1 particles in this partition
  if (end - begin <= 1)
    return;

  // TODO the below should not be needed
  // if we reached a minimum partition size to prevent excessive
  // recursion
  if (partition.size.x < 1.0f || partition.size.y < 1.0f)
  {
    std::cout << "minimum partition size reached" << std::endl;
    return;
  }

  const bool vertical = depth % 2 == 0;
  const float median =
      computeMedian(particles, vertical ? 'x' : 'y', begin, end);

  PartitionLine splitLine;
  if (vertical)
  {
    splitLine = {{median, partition.position.y},
                 {median, partition.position.y + partition.size.y}};
  }
  else
  {
    splitLine = {
        {partition.position.x, median},
        {partition.position.x + partition.size.x,
         median},
    };
  }
  partition_lines.push_back(splitLine);

  // partition particles with proper boundary checks
  auto mid = std::partition(particles.begin() + begin, particles.begin() + end,
                            [&](const auto &p)
                            {
                              if (vertical)
                              {
                                // left side includes split line (SFML is
                                // left-inclusive)
                                return p->position.x <= median;
                              }
                              else
                              {
                                // top side includes split line (SFML is
                                // top-inclusive)
                                return p->position.y <= median;
                              }
                            });

  // TODO is this redundant ?
  const size_t mid_idx = mid - particles.begin();

  // ensure we partitioned something, otherwise stop recursing
  if (mid_idx == begin || mid_idx == end)
  {
    std::cout << "bisection failed to separate particles" << std::endl;
    return;
  }

  // compute sub-partitions
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

    orbPartition(particles, begin, mid_idx, left_partition, depth + 1, partition_lines,
                 partition_labels, window);
    orbPartition(particles, mid_idx, end, right_partition, depth + 1, partition_lines,
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

    orbPartition(particles, begin, mid_idx, top_partition, depth + 1, partition_lines, partition_labels, window);
    orbPartition(particles, mid_idx, end, bottom_partition, depth + 1, partition_lines, partition_labels, window);
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
      {550.f, 500.f} // bottom-right
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
      orbPartition(particles, 0, particles.size(),
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
    {
      std::cout << "drawing label with text " << label.getString().toAnsiString()
                << std::endl;
      window.draw(label);
    }

    window.display();
    frameCount++;

    // TODO remove this
    render_once = true;
  }
  return 0;
}

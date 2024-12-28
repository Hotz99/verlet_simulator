#pragma once
#include <SFML/Graphics/Color.hpp>

namespace color_utils
{
    static sf::Color get_time_based_rgb(float t)
    {
        const float r = sin(t);
        const float g = sin(t + 0.33f * 2.0f * M_PI);
        const float b = sin(t + 0.66f * 2.0f * M_PI);
        return {static_cast<uint8_t>(255.0f * r * r),
                static_cast<uint8_t>(255.0f * g * g),
                static_cast<uint8_t>(255.0f * b * b)};
    }
};
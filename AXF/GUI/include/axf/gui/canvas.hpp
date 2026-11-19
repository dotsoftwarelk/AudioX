#pragma once

#include "axf/gui/theme.hpp"

#include <cstdint>
#include <string_view>

namespace axf::gui
{

struct Rect final
{
    float x{0.0F};
    float y{0.0F};
    float width{0.0F};
    float height{0.0F};

    [[nodiscard]] bool intersects(const Rect& other) const noexcept
    {
        return x < other.x + other.width && other.x < x + width && y < other.y + other.height &&
               other.y < y + height;
    }

    [[nodiscard]] bool contains(const Rect& other) const noexcept
    {
        return other.x >= x && other.y >= y && other.x + other.width <= x + width &&
               other.y + other.height <= y + height;
    }
};

/// Control-thread drawing surface. Implementations own native resources.
class ICanvas
{
  public:
    virtual ~ICanvas() = default;
    virtual void clear(Rgba color) noexcept = 0;
    virtual void fillRect(Rect bounds, Rgba color) noexcept = 0;
    virtual void drawText(Rect bounds, std::string_view text, Rgba color,
                          float fontSize) noexcept = 0;
    virtual void present() noexcept = 0;
};

} // namespace axf::gui

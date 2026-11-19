#pragma once

#include "axf/core/export.hpp"

namespace axf::gui
{

struct Rgba final
{
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};
};

struct Theme final
{
    Rgba background;
    Rgba panel;
    Rgba text;
    Rgba accent;
    Rgba meterGreen;
    Rgba meterYellow;
    Rgba meterRed;
};

[[nodiscard]] AXF_API Theme defaultDarkTheme() noexcept;

} // namespace axf::gui

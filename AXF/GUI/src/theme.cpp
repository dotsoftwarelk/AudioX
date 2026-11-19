#include "axf/gui/theme.hpp"

namespace axf::gui
{

Theme defaultDarkTheme() noexcept
{
    return Theme{Rgba{0.055F, 0.060F, 0.070F, 1.0F}, Rgba{0.100F, 0.110F, 0.125F, 1.0F},
                 Rgba{0.880F, 0.890F, 0.900F, 1.0F}, Rgba{0.180F, 0.560F, 0.850F, 1.0F},
                 Rgba{0.180F, 0.780F, 0.360F, 1.0F}, Rgba{0.950F, 0.780F, 0.180F, 1.0F},
                 Rgba{0.930F, 0.280F, 0.240F, 1.0F}};
}

} // namespace axf::gui

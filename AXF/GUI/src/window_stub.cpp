#include "axf/gui/window.hpp"

#if !defined(_WIN32)
namespace axf::gui
{
std::unique_ptr<IWindow> createWindowsWindow(std::string, Rect, Theme)
{
    return nullptr;
}
} // namespace axf::gui
#endif

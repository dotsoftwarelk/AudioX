#pragma once

#include "axf/gui/canvas.hpp"
#include "axf/gui/widgets.hpp"

#include <memory>
#include <string>

namespace axf::gui
{

/// Top-level native window. All methods are control/UI-thread operations.
class IWindow
{
  public:
    virtual ~IWindow() = default;
    [[nodiscard]] virtual bool create() = 0;
    virtual void run() = 0;
    virtual void close() noexcept = 0;
    [[nodiscard]] virtual Scene& scene() noexcept = 0;
};

/// Real Win32/Direct2D/DirectWrite window on Windows; nullptr elsewhere.
[[nodiscard]] AXF_API std::unique_ptr<IWindow> createWindowsWindow(std::string title, Rect bounds,
                                                                   Theme theme);

} // namespace axf::gui

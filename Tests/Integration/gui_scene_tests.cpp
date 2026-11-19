#include <gtest/gtest.h>

#include "axf/gui/window.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace axf;

namespace
{
struct DrawCall final
{
    enum class Kind : std::uint8_t
    {
        Clear,
        Rect,
        Text,
    };

    Kind kind{Kind::Rect};
    gui::Rect bounds{};
    gui::Rgba color{};
    std::string text;
};

class RecordingCanvas final : public gui::ICanvas
{
  public:
    void clear(gui::Rgba color) noexcept override
    {
        calls.push_back({DrawCall::Kind::Clear, {}, color, {}});
    }
    void fillRect(gui::Rect bounds, gui::Rgba color) noexcept override
    {
        calls.push_back({DrawCall::Kind::Rect, bounds, color, {}});
    }
    void drawText(gui::Rect bounds, std::string_view text, gui::Rgba color, float) noexcept override
    {
        calls.push_back({DrawCall::Kind::Text, bounds, color, std::string(text)});
    }
    void present() noexcept override
    {
        ++presents;
    }

    std::vector<DrawCall> calls;
    int presents{0};
};
} // namespace

TEST(GuiScene, WidgetsDrawExpectedGeometry)
{
    auto panel = std::make_unique<gui::Panel>(gui::Rect{0.0F, 0.0F, 100.0F, 40.0F},
                                              gui::Rgba{0.2F, 0.3F, 0.4F, 1.0F});
    auto label = std::make_unique<gui::Label>(gui::Rect{0.0F, 40.0F, 100.0F, 20.0F}, "AXF");
    gui::Meter* meter =
        new gui::Meter(gui::Rect{0.0F, 60.0F, 100.0F, 10.0F}, gui::Rgba{0.0F, 1.0F, 0.0F, 1.0F});
    meter->setLevel(0.5F);
    auto meterOwner = std::unique_ptr<gui::Meter>(meter);

    gui::Scene scene;
    scene.addWidget(std::move(panel));
    scene.addWidget(std::move(label));
    scene.addWidget(std::move(meterOwner));
    EXPECT_EQ(scene.widgetCount(), 3U);
    EXPECT_TRUE(scene.dirty());

    RecordingCanvas canvas;
    scene.render(canvas, gui::Rect{0.0F, 0.0F, 100.0F, 100.0F});
    ASSERT_EQ(canvas.calls.size(), 4U);
    EXPECT_FLOAT_EQ(canvas.calls[0].bounds.width, 100.0F);
    EXPECT_FLOAT_EQ(canvas.calls[0].color.g, 0.3F);
    EXPECT_EQ(canvas.calls[1].text, "AXF");
    EXPECT_FLOAT_EQ(canvas.calls[3].bounds.width, 50.0F);
    EXPECT_FALSE(scene.dirty());

    scene.render(canvas, gui::Rect{0.0F, 0.0F, 100.0F, 100.0F});
    EXPECT_EQ(canvas.calls.size(), 4U);
}

TEST(GuiScene, CullsHiddenAndOutsideWidgets)
{
    gui::Scene scene;
    auto visible = std::make_unique<gui::Panel>(gui::Rect{0.0F, 0.0F, 10.0F, 10.0F}, gui::Rgba{});
    auto hidden = std::make_unique<gui::Panel>(gui::Rect{20.0F, 0.0F, 10.0F, 10.0F}, gui::Rgba{});
    hidden->setVisible(false);
    scene.addWidget(std::move(visible));
    scene.addWidget(std::move(hidden));

    RecordingCanvas canvas;
    scene.render(canvas, gui::Rect{0.0F, 0.0F, 10.0F, 10.0F});
    EXPECT_EQ(canvas.calls.size(), 1U);
}

TEST(GuiScene, LayoutStacksWidgets)
{
    gui::Scene scene;
    scene.addWidget(std::make_unique<gui::Panel>(gui::Rect{}, gui::Rgba{}));
    scene.addWidget(std::make_unique<gui::Label>(gui::Rect{}, "one"));
    scene.layout(gui::Rect{10.0F, 20.0F, 200.0F, 100.0F}, 5.0F);

    RecordingCanvas canvas;
    scene.render(canvas, gui::Rect{0.0F, 0.0F, 400.0F, 400.0F});
    ASSERT_EQ(canvas.calls.size(), 2U);
    EXPECT_FLOAT_EQ(canvas.calls[0].bounds.x, 10.0F);
    EXPECT_FLOAT_EQ(canvas.calls[1].bounds.y, 20.0F + canvas.calls[0].bounds.height + 5.0F);
}

#if !defined(_WIN32)
TEST(GuiWindow, NativeWindowUnavailableOffWindows)
{
    EXPECT_EQ(gui::createWindowsWindow("title", gui::Rect{0.0F, 0.0F, 1.0F, 1.0F},
                                       gui::defaultDarkTheme()),
              nullptr);
}
#endif

#pragma once

#include "axf/gui/canvas.hpp"

#include <memory>
#include <string>
#include <vector>

namespace axf::gui
{

class Widget
{
  public:
    explicit Widget(Rect bounds) noexcept : bounds_(bounds)
    {
    }
    virtual ~Widget() = default;
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    [[nodiscard]] Rect bounds() const noexcept
    {
        return bounds_;
    }
    void setBounds(Rect bounds) noexcept
    {
        bounds_ = bounds;
        invalidate();
    }
    [[nodiscard]] bool visible() const noexcept
    {
        return visible_;
    }
    void setVisible(bool value) noexcept
    {
        visible_ = value;
        invalidate();
    }
    [[nodiscard]] bool dirty() const noexcept
    {
        return dirty_;
    }
    void invalidate() noexcept
    {
        dirty_ = true;
    }
    void markPainted() noexcept
    {
        dirty_ = false;
    }

    virtual void draw(ICanvas& canvas) noexcept = 0;

  private:
    Rect bounds_{};
    bool visible_{true};
    bool dirty_{true};
};

class Panel final : public Widget
{
  public:
    explicit Panel(Rect bounds, Rgba color) noexcept : Widget(bounds), color_(color)
    {
    }
    void setColor(Rgba color) noexcept
    {
        color_ = color;
        invalidate();
    }
    void draw(ICanvas& canvas) noexcept override
    {
        canvas.fillRect(bounds(), color_);
    }

  private:
    Rgba color_{};
};

class Label final : public Widget
{
  public:
    Label(Rect bounds, std::string text, float fontSize = 14.0F)
        : Widget(bounds), text_(std::move(text)), fontSize_(fontSize)
    {
    }
    void setText(std::string text)
    {
        text_ = std::move(text);
        invalidate();
    }
    [[nodiscard]] const std::string& text() const noexcept
    {
        return text_;
    }
    void draw(ICanvas& canvas) noexcept override
    {
        canvas.drawText(bounds(), text_, textColor_, fontSize_);
    }

  private:
    std::string text_;
    float fontSize_{14.0F};
    Rgba textColor_{0.9F, 0.9F, 0.9F, 1.0F};
};

class Meter final : public Widget
{
  public:
    Meter(Rect bounds, Rgba fill) noexcept : Widget(bounds), fill_(fill)
    {
    }
    void setLevel(float level) noexcept
    {
        level_ = level < 0.0F ? 0.0F : level > 1.0F ? 1.0F : level;
        invalidate();
    }
    [[nodiscard]] float level() const noexcept
    {
        return level_;
    }
    void draw(ICanvas& canvas) noexcept override
    {
        const Rect bounds = this->bounds();
        canvas.fillRect(bounds, background_);
        Rect levelBounds = bounds;
        levelBounds.width *= level_;
        canvas.fillRect(levelBounds, fill_);
    }

  private:
    Rgba fill_{};
    Rgba background_{0.12F, 0.12F, 0.12F, 1.0F};
    float level_{0.0F};
};

class Scene final
{
  public:
    void addWidget(std::unique_ptr<Widget> widget);
    [[nodiscard]] std::size_t widgetCount() const noexcept;
    [[nodiscard]] bool dirty() const noexcept;

    /// Simple vertical stack layout, control-thread only.
    void layout(Rect area, float spacing = 4.0F) noexcept;
    void render(ICanvas& canvas, Rect clip) noexcept;

  private:
    std::vector<std::unique_ptr<Widget>> widgets_;
};

} // namespace axf::gui

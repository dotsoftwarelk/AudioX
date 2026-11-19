#include "axf/gui/widgets.hpp"

#include <algorithm>

namespace axf::gui
{

void Scene::addWidget(std::unique_ptr<Widget> widget)
{
    if (widget != nullptr)
    {
        widget->invalidate();
        widgets_.push_back(std::move(widget));
    }
}

std::size_t Scene::widgetCount() const noexcept
{
    return widgets_.size();
}

bool Scene::dirty() const noexcept
{
    return std::any_of(widgets_.begin(), widgets_.end(),
                       [](const std::unique_ptr<Widget>& widget) { return widget->dirty(); });
}

void Scene::layout(Rect area, float spacing) noexcept
{
    float cursor = area.y;
    for (const auto& widget : widgets_)
    {
        Rect bounds = widget->bounds();
        bounds.x = area.x;
        bounds.width = area.width;
        bounds.y = cursor;
        widget->setBounds(bounds);
        cursor += bounds.height + spacing;
    }
}

void Scene::render(ICanvas& canvas, Rect clip) noexcept
{
    for (const auto& widget : widgets_)
    {
        if (!widget->visible() || !widget->dirty() || !clip.intersects(widget->bounds()))
        {
            continue;
        }
        widget->draw(canvas);
        widget->markPainted();
    }
}

} // namespace axf::gui

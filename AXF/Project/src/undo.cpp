#include "axf/project/undo.hpp"

#include <utility>

namespace axf::project
{

UndoStack::UndoStack(std::size_t limit) : limit_(limit == 0 ? 1 : limit)
{
}

void UndoStack::clear() noexcept
{
    undo_.clear();
    redo_.clear();
}

bool UndoStack::canUndo() const noexcept
{
    return !undo_.empty();
}

bool UndoStack::canRedo() const noexcept
{
    return !redo_.empty();
}

std::size_t UndoStack::undoCount() const noexcept
{
    return undo_.size();
}

std::size_t UndoStack::redoCount() const noexcept
{
    return redo_.size();
}

std::string_view UndoStack::undoDescription() const noexcept
{
    return undo_.empty() ? std::string_view{} : std::string_view{undo_.back().label};
}

std::string_view UndoStack::redoDescription() const noexcept
{
    return redo_.empty() ? std::string_view{} : std::string_view{redo_.back().label};
}

core::Result<void> UndoStack::apply(ProjectDocument& document, std::string label,
                                    const std::function<core::Result<void>(ProjectDocument&)>& edit)
{
    if (!edit)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "undo edit is empty");
    }
    const std::string before = document.serialize();
    const auto result = edit(document);
    if (!result)
    {
        return result;
    }
    const std::string after = document.serialize();
    if (before == after)
    {
        return core::success();
    }
    redo_.clear();
    undo_.push_back(Entry{std::move(label), before, after});
    while (undo_.size() > limit_)
    {
        undo_.erase(undo_.begin());
    }
    return core::success();
}

core::Result<void> UndoStack::undo(ProjectDocument& document)
{
    if (undo_.empty())
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "nothing to undo");
    }
    auto restored = ProjectDocument::parse(undo_.back().before);
    if (!restored)
    {
        return restored.error();
    }
    redo_.push_back(std::move(undo_.back()));
    undo_.pop_back();
    document = std::move(restored.value());
    return core::success();
}

core::Result<void> UndoStack::redo(ProjectDocument& document)
{
    if (redo_.empty())
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "nothing to redo");
    }
    auto restored = ProjectDocument::parse(redo_.back().after);
    if (!restored)
    {
        return restored.error();
    }
    undo_.push_back(std::move(redo_.back()));
    redo_.pop_back();
    document = std::move(restored.value());
    return core::success();
}

} // namespace axf::project

#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/project/document.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace axf::project
{

/// Bounded, snapshot-based undo history for control-thread edits. The
/// snapshot is the document's deterministic text format, so an undo restores
/// every field including IDs, clips and automation lanes.
class UndoStack final
{
  public:
    explicit UndoStack(std::size_t limit = 64);

    void clear() noexcept;
    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;
    [[nodiscard]] std::size_t undoCount() const noexcept;
    [[nodiscard]] std::size_t redoCount() const noexcept;
    [[nodiscard]] std::string_view undoDescription() const noexcept;
    [[nodiscard]] std::string_view redoDescription() const noexcept;

    /// Applies `edit` and records it when it succeeds and changes the document.
    [[nodiscard]] core::Result<void> apply(
        ProjectDocument& document, std::string label,
        const std::function<core::Result<void>(ProjectDocument&)>& edit);

    [[nodiscard]] core::Result<void> undo(ProjectDocument& document);
    [[nodiscard]] core::Result<void> redo(ProjectDocument& document);

  private:
    struct Entry final
    {
        std::string label;
        std::string before;
        std::string after;
    };

    std::vector<Entry> undo_;
    std::vector<Entry> redo_;
    std::size_t limit_;
};

} // namespace axf::project

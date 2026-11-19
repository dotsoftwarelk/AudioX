#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/project/document.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace axf::project
{
/// Control-thread-only bounded autosave. Each save writes a temporary file
/// and atomically replaces one of N revision files, then updates a small
/// manifest. The project format itself is unchanged.
class ProjectAutosave final
{
  public:
    explicit ProjectAutosave(std::size_t revisions = 3);

    [[nodiscard]] core::Result<void> save(const ProjectDocument& document,
                                          const std::filesystem::path& directory,
                                          std::string baseName);
    [[nodiscard]] core::Result<ProjectDocument> recover(const std::filesystem::path& directory,
                                                        const std::string& baseName) const;
    [[nodiscard]] core::Result<void> clear(const std::filesystem::path& directory,
                                           const std::string& baseName) const;
    [[nodiscard]] std::size_t revisionCount() const noexcept;

  private:
    std::size_t revisions_;
};

} // namespace axf::project

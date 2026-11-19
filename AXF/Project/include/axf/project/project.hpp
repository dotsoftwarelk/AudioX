#pragma once

#include "axf/core/export.hpp"
#include "axf/core/types.hpp"

#include <string>

namespace axf::project
{

struct ProjectManifest final
{
    core::Version formatVersion{core::kAxfVersion};
    std::string name;
    std::string author;
};

/// Project files are versioned. Phase 6 adds serialization, track/clip
/// models, automation data, autosave and recovery.
[[nodiscard]] AXF_API bool isCompatibleVersion(core::Version version) noexcept;
[[nodiscard]] AXF_API const char* projectFormatVersion() noexcept;

} // namespace axf::project

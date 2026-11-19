#include "axf/project/project.hpp"

namespace axf::project
{

bool isCompatibleVersion(core::Version version) noexcept
{
    return version.major == core::kAxfVersion.major && version.minor <= core::kAxfVersion.minor;
}

const char* projectFormatVersion() noexcept
{
    return "0.1";
}

} // namespace axf::project

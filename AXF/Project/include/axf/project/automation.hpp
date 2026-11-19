#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace axf::project
{

enum class AutomationParameter : std::uint8_t
{
    Gain,
    Pan,
};

struct AutomationPoint final
{
    std::uint64_t frame{0};
    float value{0.0F};
};

struct AutomationLane final
{
    std::uint64_t trackId{0};
    AutomationParameter parameter{AutomationParameter::Gain};
    std::vector<AutomationPoint> points;
};

[[nodiscard]] AXF_API const char* automationParameterName(AutomationParameter parameter) noexcept;
[[nodiscard]] AXF_API bool parseAutomationParameter(std::string_view name,
                                                    AutomationParameter& parameter) noexcept;
[[nodiscard]] AXF_API bool validAutomationValue(AutomationParameter parameter,
                                                float value) noexcept;
[[nodiscard]] AXF_API float interpolateAutomation(const AutomationLane& lane,
                                                  std::uint64_t frame) noexcept;

} // namespace axf::project

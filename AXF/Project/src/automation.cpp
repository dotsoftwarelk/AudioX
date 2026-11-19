#include "axf/project/automation.hpp"

#include <algorithm>
#include <cmath>

namespace axf::project
{

const char* automationParameterName(AutomationParameter parameter) noexcept
{
    switch (parameter)
    {
    case AutomationParameter::Gain:
        return "gain";
    case AutomationParameter::Pan:
        return "pan";
    }
    return "unknown";
}

bool parseAutomationParameter(std::string_view name, AutomationParameter& parameter) noexcept
{
    if (name == "gain")
    {
        parameter = AutomationParameter::Gain;
        return true;
    }
    if (name == "pan")
    {
        parameter = AutomationParameter::Pan;
        return true;
    }
    return false;
}

bool validAutomationValue(AutomationParameter parameter, float value) noexcept
{
    if (!std::isfinite(value))
    {
        return false;
    }
    return parameter == AutomationParameter::Gain ? (value >= 0.0F && value <= 4.0F)
                                                  : (value >= -1.0F && value <= 1.0F);
}

float interpolateAutomation(const AutomationLane& lane, std::uint64_t frame) noexcept
{
    if (lane.points.empty())
    {
        return 0.0F;
    }
    if (frame <= lane.points.front().frame)
    {
        return lane.points.front().value;
    }
    if (frame >= lane.points.back().frame)
    {
        return lane.points.back().value;
    }

    const auto upper = std::upper_bound(lane.points.begin(), lane.points.end(), frame,
                                        [](std::uint64_t value, const AutomationPoint& point)
                                        { return value < point.frame; });
    if (upper == lane.points.begin())
    {
        return lane.points.front().value;
    }
    const AutomationPoint& before = *(upper - 1);
    const AutomationPoint& after = *upper;
    const double span = static_cast<double>(after.frame - before.frame);
    const double offset = static_cast<double>(frame - before.frame);
    const double ratio = span == 0.0 ? 0.0 : offset / span;
    return static_cast<float>(
        static_cast<double>(before.value) +
        ratio * (static_cast<double>(after.value) - static_cast<double>(before.value)));
}

} // namespace axf::project

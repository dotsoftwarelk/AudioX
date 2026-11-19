#include "axf/core/time.hpp"

#include <iomanip>
#include <sstream>

namespace axf::core::time
{

std::string formatTimecode(FrameIndex frame, SampleRate rate)
{
    if (frame < 0 || !isValidSampleRate(rate))
    {
        return "--:--:--:--";
    }
    const FrameIndex framesPerSecond = static_cast<FrameIndex>(rate);
    const FrameIndex hours = frame / (framesPerSecond * 3600);
    const FrameIndex minutes = (frame / (framesPerSecond * 60)) % 60;
    const FrameIndex seconds = (frame / framesPerSecond) % 60;
    const FrameIndex frames = frame % framesPerSecond;

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(2) << hours << ':' << std::setw(2) << minutes << ':'
           << std::setw(2) << seconds << ':' << std::setw(2) << frames;
    return stream.str();
}

} // namespace axf::core::time

#include "axf/project/document.hpp"

#include "axf/project/project.hpp"

#include "axf/project/automation.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

namespace axf::project
{
namespace
{
constexpr std::string_view kMagic = "AXF-PROJECT";

std::string escape(std::string_view value)
{
    std::string result;
    for (const char character : value)
    {
        if (character == '\\')
        {
            result += "\\\\";
        }
        else if (character == '\t')
        {
            result += "\\t";
        }
        else if (character == '\n')
        {
            result += "\\n";
        }
        else
        {
            result.push_back(character);
        }
    }
    return result;
}

core::Result<std::string> unescape(std::string_view value)
{
    std::string result;
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] != '\\')
        {
            result.push_back(value[index]);
            continue;
        }
        if (++index >= value.size())
        {
            return core::makeError(core::ErrorCode::InvalidArgument,
                                   "dangling escape in project file");
        }
        switch (value[index])
        {
        case '\\':
            result.push_back('\\');
            break;
        case 't':
            result.push_back('\t');
            break;
        case 'n':
            result.push_back('\n');
            break;
        default:
            return core::makeError(core::ErrorCode::InvalidArgument,
                                   "unknown escape in project file");
        }
    }
    return result;
}

std::vector<std::string_view> splitTabs(std::string_view line)
{
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    for (std::size_t index = 0; index <= line.size(); ++index)
    {
        if (index == line.size() || line[index] == '\t')
        {
            fields.push_back(line.substr(start, index - start));
            start = index + 1;
        }
    }
    return fields;
}

template <class T> core::Result<T> parseNumber(std::string_view text) noexcept
{
    T value{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "invalid project number");
    }
    return value;
}

bool validTempo(double tempo) noexcept
{
    return std::isfinite(tempo) && tempo >= 20.0 && tempo <= 300.0;
}

bool validGain(float gain) noexcept
{
    return std::isfinite(gain) && gain >= 0.0F && gain <= 4.0F;
}

bool validPan(float pan) noexcept
{
    return std::isfinite(pan) && pan >= -1.0F && pan <= 1.0F;
}

constexpr std::size_t kMaxAutomationPoints = 4096;
} // namespace

ProjectDocument::ProjectDocument() = default;

void ProjectDocument::setName(std::string name) noexcept
{
    name_ = std::move(name);
}

const std::string& ProjectDocument::name() const noexcept
{
    return name_;
}

void ProjectDocument::setTempo(double beatsPerMinute) noexcept
{
    if (validTempo(beatsPerMinute))
    {
        tempo_ = beatsPerMinute;
    }
}

double ProjectDocument::tempo() const noexcept
{
    return tempo_;
}

core::Result<std::uint64_t> ProjectDocument::addTrack(std::string name)
{
    const std::uint64_t id = nextId_++;
    tracks_.push_back(Track{id, std::move(name), 1.0F, 0.0F, false, false, {}});
    return id;
}

core::Result<void> ProjectDocument::removeTrack(std::uint64_t trackId) noexcept
{
    const auto before = tracks_.size();
    tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
                                 [trackId](const Track& track) { return track.id == trackId; }),
                  tracks_.end());
    if (tracks_.size() == before)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    return core::success();
}

const std::vector<Track>& ProjectDocument::tracks() const noexcept
{
    return tracks_;
}

Track* ProjectDocument::findTrack(std::uint64_t trackId) noexcept
{
    for (Track& track : tracks_)
    {
        if (track.id == trackId)
        {
            return &track;
        }
    }
    return nullptr;
}

core::Result<void> ProjectDocument::setTrackGain(std::uint64_t trackId, float gain) noexcept
{
    if (!validGain(gain))
    {
        return core::makeError(core::ErrorCode::OutOfRange, "gain must be in [0, 4]");
    }
    Track* track = findTrack(trackId);
    if (track == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    track->gain = gain;
    return core::success();
}

core::Result<void> ProjectDocument::setTrackPan(std::uint64_t trackId, float pan) noexcept
{
    if (!validPan(pan))
    {
        return core::makeError(core::ErrorCode::OutOfRange, "pan must be in [-1, 1]");
    }
    Track* track = findTrack(trackId);
    if (track == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    track->pan = pan;
    return core::success();
}

core::Result<void> ProjectDocument::setTrackMute(std::uint64_t trackId, bool muted) noexcept
{
    Track* track = findTrack(trackId);
    if (track == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    track->muted = muted;
    return core::success();
}

core::Result<void> ProjectDocument::setTrackSolo(std::uint64_t trackId, bool soloed) noexcept
{
    Track* track = findTrack(trackId);
    if (track == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    track->soloed = soloed;
    return core::success();
}

core::Result<std::uint64_t> ProjectDocument::addClip(std::uint64_t trackId, std::string source,
                                                     std::uint64_t startFrame,
                                                     std::uint64_t lengthFrames) noexcept
{
    Track* track = findTrack(trackId);
    if (track == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    if (lengthFrames == 0 || startFrame > std::numeric_limits<std::uint64_t>::max() - lengthFrames)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "invalid clip range");
    }
    const std::uint64_t id = nextId_++;
    track->clips.push_back(Clip{id, startFrame, lengthFrames, std::move(source)});
    return id;
}

core::Result<void> ProjectDocument::setAutomation(std::uint64_t trackId,
                                                  AutomationParameter parameter,
                                                  std::vector<AutomationPoint> points) noexcept
{
    if (findTrack(trackId) == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    if (points.size() > kMaxAutomationPoints)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "too many automation points");
    }
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        if (i > 0 && points[i].frame <= points[i - 1].frame)
        {
            return core::makeError(core::ErrorCode::InvalidArgument,
                                   "automation frames must strictly increase");
        }
        if (!validAutomationValue(parameter, points[i].value))
        {
            return core::makeError(core::ErrorCode::OutOfRange, "automation value out of range");
        }
    }
    automation_.erase(std::remove_if(automation_.begin(), automation_.end(),
                                     [trackId, parameter](const AutomationLane& lane) {
                                         return lane.trackId == trackId &&
                                                lane.parameter == parameter;
                                     }),
                      automation_.end());
    if (!points.empty())
    {
        automation_.push_back(AutomationLane{trackId, parameter, std::move(points)});
    }
    return core::success();
}

core::Result<void> ProjectDocument::clearAutomation(std::uint64_t trackId,
                                                    AutomationParameter parameter) noexcept
{
    const auto before = automation_.size();
    automation_.erase(std::remove_if(automation_.begin(), automation_.end(),
                                     [trackId, parameter](const AutomationLane& lane) {
                                         return lane.trackId == trackId &&
                                                lane.parameter == parameter;
                                     }),
                      automation_.end());
    return automation_.size() == before
               ? core::makeError(core::ErrorCode::NotFound, "automation lane not found")
               : core::success();
}

const std::vector<AutomationLane>& ProjectDocument::automation() const noexcept
{
    return automation_;
}

core::Result<float> ProjectDocument::evaluateAutomation(std::uint64_t trackId,
                                                        AutomationParameter parameter,
                                                        std::uint64_t frame) const noexcept
{
    const Track* track = nullptr;
    for (const Track& candidate : tracks_)
    {
        if (candidate.id == trackId)
        {
            track = &candidate;
            break;
        }
    }
    if (track == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "track not found");
    }
    const float base = parameter == AutomationParameter::Gain ? track->gain : track->pan;
    for (const AutomationLane& lane : automation_)
    {
        if (lane.trackId == trackId && lane.parameter == parameter)
        {
            return interpolateAutomation(lane, frame);
        }
    }
    return base;
}

core::Result<void> ProjectDocument::removeClip(std::uint64_t clipId) noexcept
{
    for (Track& track : tracks_)
    {
        const auto before = track.clips.size();
        track.clips.erase(std::remove_if(track.clips.begin(), track.clips.end(),
                                         [clipId](const Clip& clip) { return clip.id == clipId; }),
                          track.clips.end());
        if (track.clips.size() != before)
        {
            return core::success();
        }
    }
    return core::makeError(core::ErrorCode::NotFound, "clip not found");
}

std::string ProjectDocument::serialize() const
{
    std::string output;
    output += std::string(kMagic) + "\t" + projectFormatVersion() + "\n";
    output += "NAME\t" + escape(name_) + "\n";
    output += "TEMPO\t" + std::to_string(tempo_) + "\n";
    for (const Track& track : tracks_)
    {
        output += "TRACK\t" + std::to_string(track.id) + "\t" + escape(track.name) + "\t" +
                  std::to_string(track.gain) + "\t" + std::to_string(track.pan) + "\t" +
                  (track.muted ? "1" : "0") + "\t" + (track.soloed ? "1" : "0") + "\n";
        for (const Clip& clip : track.clips)
        {
            output += "CLIP\t" + std::to_string(clip.id) + "\t" + std::to_string(clip.startFrame) +
                      "\t" + std::to_string(clip.lengthFrames) + "\t" + escape(clip.source) + "\n";
        }
    }
    for (const AutomationLane& lane : automation_)
    {
        for (const AutomationPoint& point : lane.points)
        {
            output += "AUTOMATION\t" + std::to_string(lane.trackId) + "\t" +
                      automationParameterName(lane.parameter) + "\t" + std::to_string(point.frame) +
                      "\t" + std::to_string(point.value) + "\n";
        }
    }
    output += "END\n";
    return output;
}

core::Result<ProjectDocument> ProjectDocument::parse(std::string_view text)
{
    ProjectDocument document;
    std::size_t lineStart = 0;
    Track* currentTrack = nullptr;
    bool haveHeader = false;
    bool haveName = false;
    bool haveEnd = false;

    while (lineStart < text.size())
    {
        const std::size_t newline = text.find('\n', lineStart);
        const std::string_view line =
            text.substr(lineStart, newline == std::string_view::npos ? std::string_view::npos
                                                                     : newline - lineStart);
        lineStart = newline == std::string_view::npos ? text.size() : newline + 1;
        if (line.empty())
        {
            continue;
        }
        const auto fields = splitTabs(line);
        if (!haveHeader)
        {
            if (fields.size() != 2 || fields[0] != kMagic)
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "bad project header");
            }
            haveHeader = true;
            continue;
        }
        if (fields[0] == "NAME" && fields.size() == 2)
        {
            auto value = unescape(fields[1]);
            if (!value)
            {
                return value.error();
            }
            document.setName(std::move(value.value()));
            haveName = true;
        }
        else if (fields[0] == "TEMPO" && fields.size() == 2)
        {
            const auto tempo = parseNumber<double>(fields[1]);
            if (!tempo || !validTempo(tempo.value()))
            {
                return core::makeError(core::ErrorCode::OutOfRange, "invalid project tempo");
            }
            document.setTempo(tempo.value());
        }
        else if (fields[0] == "TRACK" && fields.size() == 7)
        {
            const auto id = parseNumber<std::uint64_t>(fields[1]);
            const auto gain = parseNumber<float>(fields[3]);
            const auto pan = parseNumber<float>(fields[4]);
            auto name = unescape(fields[2]);
            if (!id || !gain || !pan || !name || !validGain(gain.value()) ||
                !validPan(pan.value()) || (fields[5] != "0" && fields[5] != "1") ||
                (fields[6] != "0" && fields[6] != "1"))
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "invalid track record");
            }
            document.tracks_.push_back(Track{id.value(),
                                             std::move(name.value()),
                                             gain.value(),
                                             pan.value(),
                                             fields[5] == "1",
                                             fields[6] == "1",
                                             {}});
            currentTrack = &document.tracks_.back();
            document.nextId_ = std::max(document.nextId_, id.value() + 1);
        }
        else if (fields[0] == "AUTOMATION" && fields.size() == 5)
        {
            const auto trackId = parseNumber<std::uint64_t>(fields[1]);
            AutomationParameter parameter{};
            const auto frame = parseNumber<std::uint64_t>(fields[3]);
            const auto value = parseNumber<float>(fields[4]);
            if (!trackId || !frame || !value || !parseAutomationParameter(fields[2], parameter) ||
                !validAutomationValue(parameter, value.value()))
            {
                return core::makeError(core::ErrorCode::InvalidArgument,
                                       "invalid automation record");
            }
            if (document.findTrack(trackId.value()) == nullptr)
            {
                return core::makeError(core::ErrorCode::InvalidArgument,
                                       "automation for unknown track");
            }
            AutomationLane* lane = nullptr;
            for (AutomationLane& candidate : document.automation_)
            {
                if (candidate.trackId == trackId.value() && candidate.parameter == parameter)
                {
                    lane = &candidate;
                    break;
                }
            }
            if (lane == nullptr)
            {
                document.automation_.push_back(AutomationLane{trackId.value(), parameter, {}});
                lane = &document.automation_.back();
            }
            if (lane->points.size() >= kMaxAutomationPoints ||
                (!lane->points.empty() && frame.value() <= lane->points.back().frame))
            {
                return core::makeError(core::ErrorCode::InvalidArgument,
                                       "invalid automation point order");
            }
            lane->points.push_back(AutomationPoint{frame.value(), value.value()});
        }
        else if (fields[0] == "CLIP" && fields.size() == 5)
        {
            if (currentTrack == nullptr)
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "clip without track");
            }
            const auto id = parseNumber<std::uint64_t>(fields[1]);
            const auto start = parseNumber<std::uint64_t>(fields[2]);
            const auto length = parseNumber<std::uint64_t>(fields[3]);
            auto source = unescape(fields[4]);
            if (!id || !start || !length || !source || length.value() == 0 ||
                start.value() > std::numeric_limits<std::uint64_t>::max() - length.value())
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "invalid clip record");
            }
            currentTrack->clips.push_back(
                Clip{id.value(), start.value(), length.value(), std::move(source.value())});
            document.nextId_ = std::max(document.nextId_, id.value() + 1);
        }
        else if (fields[0] == "END")
        {
            haveEnd = true;
        }
        else
        {
            return core::makeError(core::ErrorCode::InvalidArgument, "unknown project record");
        }
    }

    if (!haveHeader || !haveName || !haveEnd)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "incomplete project file");
    }
    return document;
}

} // namespace axf::project

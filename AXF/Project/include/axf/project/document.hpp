#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/project/automation.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace axf::project
{

struct Clip final
{
    std::uint64_t id{0};
    std::uint64_t startFrame{0};
    std::uint64_t lengthFrames{0};
    std::string source;
};

struct Track final
{
    std::uint64_t id{0};
    std::string name;
    float gain{1.0F};
    float pan{0.0F};
    bool muted{false};
    bool soloed{false};
    std::vector<Clip> clips;
};

/// Control-thread project model. It uses only AXF public types and never
/// touches the audio callback.
class ProjectDocument final
{
  public:
    ProjectDocument();

    void setName(std::string name) noexcept;
    [[nodiscard]] const std::string& name() const noexcept;
    void setTempo(double beatsPerMinute) noexcept;
    [[nodiscard]] double tempo() const noexcept;

    [[nodiscard]] core::Result<std::uint64_t> addTrack(std::string name);
    [[nodiscard]] core::Result<void> removeTrack(std::uint64_t trackId) noexcept;
    [[nodiscard]] const std::vector<Track>& tracks() const noexcept;
    [[nodiscard]] Track* findTrack(std::uint64_t trackId) noexcept;

    [[nodiscard]] core::Result<void> setTrackGain(std::uint64_t trackId, float gain) noexcept;
    [[nodiscard]] core::Result<void> setTrackPan(std::uint64_t trackId, float pan) noexcept;
    [[nodiscard]] core::Result<void> setTrackMute(std::uint64_t trackId, bool muted) noexcept;
    [[nodiscard]] core::Result<void> setTrackSolo(std::uint64_t trackId, bool soloed) noexcept;

    [[nodiscard]] core::Result<std::uint64_t> addClip(std::uint64_t trackId, std::string source,
                                                      std::uint64_t startFrame,
                                                      std::uint64_t lengthFrames) noexcept;
    [[nodiscard]] core::Result<void> removeClip(std::uint64_t clipId) noexcept;

    /// Replaces one automation lane. Points must be strictly increasing in
    /// frame order and within the parameter's value range.
    [[nodiscard]] core::Result<void> setAutomation(std::uint64_t trackId,
                                                   AutomationParameter parameter,
                                                   std::vector<AutomationPoint> points) noexcept;
    [[nodiscard]] core::Result<void> clearAutomation(std::uint64_t trackId,
                                                     AutomationParameter parameter) noexcept;
    [[nodiscard]] const std::vector<AutomationLane>& automation() const noexcept;
    /// Returns the interpolated value, or the track's static value when no
    /// lane exists. Control-thread use only.
    [[nodiscard]] core::Result<float> evaluateAutomation(std::uint64_t trackId,
                                                         AutomationParameter parameter,
                                                         std::uint64_t frame) const noexcept;

    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static core::Result<ProjectDocument> parse(std::string_view text);

  private:
    std::string name_{"Untitled"};
    double tempo_{120.0};
    std::uint64_t nextId_{1};
    std::vector<Track> tracks_;
    std::vector<AutomationLane> automation_;
};

} // namespace axf::project

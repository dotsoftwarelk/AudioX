#pragma once

#include "axf/devices/device.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace axf::devices
{

namespace detail
{
/// Windows-only factory. Declared unconditionally so the public header stays
/// identical on every platform; returns nullptr when not compiled in.
[[nodiscard]] AXF_API std::unique_ptr<IDeviceBackend> createWasapiBackendWindows();
[[nodiscard]] AXF_API std::unique_ptr<IDeviceBackend> createWasapiExclusiveBackendWindows();
[[nodiscard]] AXF_API std::unique_ptr<IDeviceBackend> createAsioBackendWindows();
} // namespace detail

/// Owns backends and provides a single discovery/opening entry point. All
/// registration and listener management is control-thread only.
class DeviceManager final : public IDeviceChangeListener
{
  public:
    DeviceManager();
    ~DeviceManager();

    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    void addBackend(std::unique_ptr<IDeviceBackend> backend);

    /// Aggregates devices from every registered backend.
    [[nodiscard]] core::Result<std::vector<DeviceInfo>> enumerate() const;

    /// Opens a device. If config.backend is None, every backend is tried.
    [[nodiscard]] core::Result<std::unique_ptr<IAudioDevice>> open(const std::string& deviceId,
                                                                   const DeviceConfig& config);

    /// Finds the first matching active endpoint for a backend and direction.
    [[nodiscard]] core::Result<DeviceInfo> defaultDevice(Backend backend, bool forInput) const;

    void addChangeListener(IDeviceChangeListener* listener) noexcept;
    void removeChangeListener(IDeviceChangeListener* listener) noexcept;
    void notifyDevicesChanged() noexcept;

    [[nodiscard]] std::size_t backendCount() const noexcept;
    [[nodiscard]] std::size_t listenerCount() const noexcept;

    void onDevicesChanged() noexcept override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// Returns a real WASAPI backend on Windows, or nullptr on other platforms.
/// A nullptr means the backend is unavailable; callers must handle it.
[[nodiscard]] AXF_API std::unique_ptr<IDeviceBackend> createWasapiBackend();

/// Returns a real ASIO backend on Windows when AXF is built with the Steinberg
/// ASIO SDK, or nullptr otherwise. A nullptr means the backend is unavailable;
/// callers must handle it.
[[nodiscard]] AXF_API std::unique_ptr<IDeviceBackend> createAsioBackend();

} // namespace axf::devices

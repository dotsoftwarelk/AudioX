#include "axf/devices/device_manager.hpp"

#include <algorithm>

namespace axf::devices
{

struct DeviceManager::Impl final
{
    std::vector<std::unique_ptr<IDeviceBackend>> backends;
    std::vector<IDeviceChangeListener*> listeners;
};

DeviceManager::DeviceManager() : impl_(std::make_unique<Impl>())
{
}
DeviceManager::~DeviceManager() = default;

void DeviceManager::addBackend(std::unique_ptr<IDeviceBackend> backend)
{
    if (backend != nullptr)
    {
        impl_->backends.push_back(std::move(backend));
    }
}

core::Result<std::vector<DeviceInfo>> DeviceManager::enumerate() const
{
    std::vector<DeviceInfo> all;
    for (const auto& backend : impl_->backends)
    {
        auto devices = backend->enumerate();
        if (!devices)
        {
            return devices.error();
        }
        for (auto& device : devices.value())
        {
            all.push_back(std::move(device));
        }
    }
    std::sort(all.begin(), all.end(),
              [](const DeviceInfo& left, const DeviceInfo& right)
              {
                  if (left.backend != right.backend)
                  {
                      return static_cast<std::uint8_t>(left.backend) <
                             static_cast<std::uint8_t>(right.backend);
                  }
                  return left.name < right.name;
              });
    return all;
}

core::Result<std::unique_ptr<IAudioDevice>> DeviceManager::open(const std::string& deviceId,
                                                                const DeviceConfig& config)
{
    const auto validation = validateConfig(config);
    if (!validation)
    {
        return validation.error();
    }
    for (auto& backend : impl_->backends)
    {
        if (config.backend != Backend::None && backend->backend() != config.backend)
        {
            continue;
        }
        auto device = backend->open(deviceId, config);
        if (device)
        {
            return device;
        }
    }
    return core::makeError(core::ErrorCode::NotFound, "no backend could open the device");
}

core::Result<DeviceInfo> DeviceManager::defaultDevice(Backend backend, bool forInput) const
{
    auto devices = enumerate();
    if (!devices)
    {
        return devices.error();
    }
    for (const DeviceInfo& device : devices.value())
    {
        if (backend != Backend::None && device.backend != backend)
        {
            continue;
        }
        if (forInput ? device.maxInputChannels > 0 : device.maxOutputChannels > 0)
        {
            return device;
        }
    }
    return core::makeError(core::ErrorCode::NotFound, "no matching device found");
}

void DeviceManager::addChangeListener(IDeviceChangeListener* listener) noexcept
{
    if (listener == nullptr)
    {
        return;
    }
    if (std::find(impl_->listeners.begin(), impl_->listeners.end(), listener) ==
        impl_->listeners.end())
    {
        impl_->listeners.push_back(listener);
    }
}

void DeviceManager::removeChangeListener(IDeviceChangeListener* listener) noexcept
{
    impl_->listeners.erase(std::remove(impl_->listeners.begin(), impl_->listeners.end(), listener),
                           impl_->listeners.end());
}

void DeviceManager::onDevicesChanged() noexcept
{
    notifyDevicesChanged();
}

void DeviceManager::notifyDevicesChanged() noexcept
{
    for (IDeviceChangeListener* listener : impl_->listeners)
    {
        listener->onDevicesChanged();
    }
}

std::size_t DeviceManager::backendCount() const noexcept
{
    return impl_->backends.size();
}

std::size_t DeviceManager::listenerCount() const noexcept
{
    return impl_->listeners.size();
}

std::unique_ptr<IDeviceBackend> createWasapiBackend()
{
#if defined(AXF_HAS_WASAPI) && defined(_WIN32)
    return detail::createWasapiBackendWindows();
#else
    return nullptr;
#endif
}

} // namespace axf::devices

#if !defined(_WIN32)
namespace axf::devices::detail
{
std::unique_ptr<IDeviceBackend> createWasapiBackendWindows()
{
    return nullptr;
}

std::unique_ptr<IDeviceBackend> createWasapiExclusiveBackendWindows()
{
    return nullptr;
}
} // namespace axf::devices::detail
#endif

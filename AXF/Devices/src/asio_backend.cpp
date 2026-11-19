// Windows-only ASIO backend. Compiled only when AXF_ENABLE_ASIO=ON and the
// Steinberg ASIO SDK root is supplied. The SDK is never vendored into AXF.
#if defined(_WIN32) && defined(AXF_HAS_ASIO)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// The ASIO SDK's iasiodrv.h predates modern MSVC headers and expects the old
// MIDL `interface` macro. Define it before any SDK header is included.
#ifndef interface
#define interface struct
#endif
#include "axf/devices/device_manager.hpp"

#include "axf/core/rt.hpp"

#include <unknwn.h>

#include <asiodrivers.h>
#include <iasiodrv.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// Defined by the SDK's common/asio.cpp translation unit.
extern IASIO* theAsioDriver;

namespace axf::devices::detail
{
namespace
{
constexpr long kMaxAsioDrivers = 64;
std::atomic<bool> g_open{false};
std::atomic<AsioDevice*> g_activeDevice{nullptr};

core::Result<void> asioStatus(ASIOError error) noexcept
{
    if (error == ASE_OK || error == ASE_SUCCESS)
    {
        return core::success();
    }
    return AXF_ERROR(core::ErrorCode::DeviceError, "ASIO driver operation failed");
}

bool isFloat32(ASIOSampleType type) noexcept
{
    return type == ASIOSTFloat32LSB;
}
} // namespace

class AsioDevice final : public IAudioDevice
{
  public:
    AsioDevice(DeviceInfo info, DeviceConfig config, std::unique_ptr<AsioDrivers> drivers,
               std::vector<ASIOBufferInfo> buffers, std::uint32_t frames)
        : info_(std::move(info)), config_(config), drivers_(std::move(drivers)),
          buffers_(std::move(buffers)), frames_(frames),
          inputPointers_(std::make_unique<float*[]>(config.inputChannels)),
          outputPointers_(std::make_unique<float*[]>(config.outputChannels))
    {
    }

    ~AsioDevice() override
    {
        stop();
        if (!buffers_.empty() && theAsioDriver != nullptr)
        {
            theAsioDriver->disposeBuffers();
        }
        buffers_.clear();
        if (drivers_ != nullptr)
        {
            drivers_->removeCurrentDriver();
        }
        theAsioDriver = nullptr;
        g_open.store(false, std::memory_order_release);
    }

    AsioDevice(const AsioDevice&) = delete;
    AsioDevice& operator=(const AsioDevice&) = delete;

    [[nodiscard]] const DeviceInfo& info() const noexcept override
    {
        return info_;
    }

    [[nodiscard]] const DeviceConfig& config() const noexcept override
    {
        return config_;
    }

    [[nodiscard]] core::Result<void> start(IAudioDeviceCallback* callback) noexcept override
    {
        if (theAsioDriver == nullptr || buffers_.empty())
        {
            return AXF_ERROR(core::ErrorCode::DeviceError, "ASIO device is not prepared");
        }
        if (state() == DeviceState::Disconnected)
        {
            return AXF_ERROR(core::ErrorCode::DeviceError, "ASIO driver requested a reset");
        }
        callback_.store(callback, std::memory_order_release);
        g_activeDevice.store(this, std::memory_order_release);
        const auto result = asioStatus(theAsioDriver->start());
        if (!result)
        {
            g_activeDevice.store(nullptr, std::memory_order_release);
            callback_.store(nullptr, std::memory_order_release);
            return result;
        }
        running_.store(true, std::memory_order_release);
        state_.store(DeviceState::Active, std::memory_order_release);
        return core::success();
    }

    void stop() noexcept override
    {
        if (running_.exchange(false, std::memory_order_acq_rel))
        {
            if (theAsioDriver != nullptr)
            {
                theAsioDriver->stop();
            }
        }
        callback_.store(nullptr, std::memory_order_release);
        AsioDevice* expected = this;
        g_activeDevice.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        state_.store(DeviceState::Unknown, std::memory_order_release);
    }

    [[nodiscard]] bool isRunning() const noexcept override
    {
        return running_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double reportedLatencySeconds() const noexcept override
    {
        if (latenciesChanged_.exchange(false, std::memory_order_acq_rel) &&
            theAsioDriver != nullptr)
        {
            long input = 0;
            long output = 0;
            if (theAsioDriver->getLatencies(&input, &output) == ASE_OK)
            {
                latencySamples_.store(static_cast<double>(input + output),
                                      std::memory_order_release);
            }
        }
        if (config_.sampleRate == 0)
        {
            return 0.0;
        }
        return latencySamples_.load(std::memory_order_acquire) /
               static_cast<double>(config_.sampleRate);
    }

    [[nodiscard]] DeviceState state() const noexcept override
    {
        if (resetRequested_.load(std::memory_order_acquire))
        {
            return DeviceState::Disconnected;
        }
        return state_.load(std::memory_order_acquire);
    }

    [[nodiscard]] DeviceDiagnostics diagnostics() const noexcept override
    {
        DeviceDiagnostics result{};
        result.callbackCount = callbackCount_.load(std::memory_order_relaxed);
        result.underrunCount = underrunCount_.load(std::memory_order_relaxed);
        result.overrunCount = overrunCount_.load(std::memory_order_relaxed);
        result.lastCallbackFrames = frames_;
        result.lastCallbackMicroseconds = lastCallbackMicroseconds_.load(std::memory_order_relaxed);
        return result;
    }

    void setLatencySamples(long input, long output) noexcept
    {
        latencySamples_.store(static_cast<double>(input + output), std::memory_order_release);
    }

    [[nodiscard]] ASIOBufferInfo* buffersForCreation() noexcept
    {
        return buffers_.data();
    }

    [[nodiscard]] std::size_t bufferCount() const noexcept
    {
        return buffers_.size();
    }

    AsioCallbacks* callbacks() noexcept
    {
        callbacks_.bufferSwitch = &AsioDevice::onBufferSwitch;
        callbacks_.sampleRateDidChange = &AsioDevice::onSampleRateDidChange;
        callbacks_.asioMessage = &AsioDevice::onAsioMessage;
        callbacks_.bufferSwitchTimeInfo = nullptr;
        return &callbacks_;
    }

  private:
    static void onBufferSwitch(long doubleBufferIndex, ASIOBool) noexcept
    {
        AsioDevice* self = g_activeDevice.load(std::memory_order_acquire);
        if (self == nullptr || self->buffers_.empty())
        {
            return;
        }
        core::rt::ScopedAudioThread audioThread;

        const auto begin = std::chrono::steady_clock::now();
        const std::size_t index = doubleBufferIndex == 0 ? 0U : 1U;
        for (std::size_t channel = 0; channel < self->config_.inputChannels; ++channel)
        {
            self->inputPointers_[channel] =
                static_cast<float*>(self->buffers_[channel].buffers[index]);
        }
        for (std::size_t channel = 0; channel < self->config_.outputChannels; ++channel)
        {
            self->outputPointers_[channel] = static_cast<float*>(
                self->buffers_[self->config_.inputChannels + channel].buffers[index]);
        }

        IAudioDeviceCallback* callback = self->callback_.load(std::memory_order_acquire);
        if (callback != nullptr)
        {
            const std::uint64_t position = self->frameCounter_.load(std::memory_order_relaxed);
            callback->onAudioCallback(self->inputPointers_.get(), self->outputPointers_.get(),
                                      self->frames_, position);
        }
        self->frameCounter_.fetch_add(self->frames_, std::memory_order_relaxed);
        self->callbackCount_.fetch_add(1, std::memory_order_relaxed);

        const auto elapsed =
            std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - begin)
                .count();
        self->lastCallbackMicroseconds_.store(elapsed, std::memory_order_relaxed);

        // ASIOOutputReady is a no-op on drivers that do not support it. It is
        // called from the audio thread as required by the ASIO contract.
        if (theAsioDriver != nullptr)
        {
            theAsioDriver->outputReady();
        }
    }

    static void onSampleRateDidChange(ASIOSampleRate rate) noexcept
    {
        AsioDevice* self = g_activeDevice.load(std::memory_order_acquire);
        if (self != nullptr)
        {
            self->sampleRateChanged_.store(rate, std::memory_order_relaxed);
        }
    }

    static long onAsioMessage(long selector, long, void*, double*) noexcept
    {
        AsioDevice* self = g_activeDevice.load(std::memory_order_acquire);
        switch (selector)
        {
        case kAsioEngineVersion:
            return 2;
        case kAsioSupportsTimeInfo:
        case kAsioSupportsTimeCode:
            return 0;
        case kAsioResetRequest:
            if (self != nullptr)
            {
                self->resetRequested_.store(true, std::memory_order_release);
            }
            return 1;
        case kAsioResyncRequest:
            if (self != nullptr)
            {
                self->underrunCount_.fetch_add(1, std::memory_order_relaxed);
            }
            return 1;
        case kAsioLatenciesChanged:
            if (self != nullptr)
            {
                self->latenciesChanged_.store(true, std::memory_order_release);
            }
            return 1;
        case kAsioOverload:
            if (self != nullptr)
            {
                self->overrunCount_.fetch_add(1, std::memory_order_relaxed);
            }
            return 1;
        default:
            return 0;
        }
    }

    DeviceInfo info_;
    DeviceConfig config_;
    std::unique_ptr<AsioDrivers> drivers_;
    std::vector<ASIOBufferInfo> buffers_;
    std::uint32_t frames_;
    std::unique_ptr<float*[]> inputPointers_;
    std::unique_ptr<float*[]> outputPointers_;
    ASIOCallbacks callbacks_{};
    std::atomic<IAudioDeviceCallback*> callback_{nullptr};
    std::atomic<DeviceState> state_{DeviceState::Unknown};
    std::atomic<bool> running_{false};
    std::atomic<bool> resetRequested_{false};
    mutable std::atomic<bool> latenciesChanged_{false};
    mutable std::atomic<double> latencySamples_{0.0};
    std::atomic<double> sampleRateChanged_{0.0};
    std::atomic<std::uint64_t> frameCounter_{0};
    std::atomic<std::uint64_t> callbackCount_{0};
    std::atomic<std::uint64_t> underrunCount_{0};
    std::atomic<std::uint64_t> overrunCount_{0};
    std::atomic<double> lastCallbackMicroseconds_{0.0};
};

class AsioBackend final : public IDeviceBackend
{
  public:
    [[nodiscard]] Backend backend() const noexcept override
    {
        return Backend::Asio;
    }

    [[nodiscard]] core::Result<std::vector<DeviceInfo>> enumerate() override
    {
        if (g_open.load(std::memory_order_acquire))
        {
            return AXF_ERROR(core::ErrorCode::DeviceError,
                             "cannot enumerate ASIO drivers while a device is open");
        }
        auto drivers = std::make_unique<AsioDrivers>();
        std::array<std::array<char, 32>, kMaxAsioDrivers> names{};
        std::array<char*, kMaxAsioDrivers> pointers{};
        for (long i = 0; i < kMaxAsioDrivers; ++i)
        {
            pointers[static_cast<std::size_t>(i)] = names[static_cast<std::size_t>(i)].data();
        }
        const long count = drivers->getDriverNames(pointers.data(), kMaxAsioDrivers);
        std::vector<DeviceInfo> devices;
        for (long i = 0; i < count; ++i)
        {
            const char* name = pointers[static_cast<std::size_t>(i)];
            if (name == nullptr || name[0] == '\0')
            {
                continue;
            }
            std::array<char, 64> driverName{};
            std::snprintf(driverName.data(), driverName.size(), "%s", name);
            DeviceInfo info;
            info.name = name;
            info.id = std::string("asio:") + name;
            info.backend = Backend::Asio;
            if (drivers->loadDriver(driverName.data()) && theAsioDriver != nullptr &&
                theAsioDriver->init(nullptr) != ASIOFalse)
            {
                long inputs = 0;
                long outputs = 0;
                if (theAsioDriver->getChannels(&inputs, &outputs) == ASE_OK)
                {
                    info.maxInputChannels = static_cast<core::ChannelCount>(std::max(0L, inputs));
                    info.maxOutputChannels = static_cast<core::ChannelCount>(std::max(0L, outputs));
                }
                for (core::SampleRate rate : {44100U, 48000U, 88200U, 96000U, 176400U, 192000U})
                {
                    if (theAsioDriver->canSampleRate(static_cast<ASIOSampleRate>(rate)) == ASE_OK)
                    {
                        info.supportedSampleRates.push_back(rate);
                    }
                }
            }
            drivers->removeCurrentDriver();
            theAsioDriver = nullptr;
            devices.push_back(std::move(info));
        }
        return devices;
    }

    [[nodiscard]] core::Result<std::unique_ptr<IAudioDevice>> open(
        const std::string& deviceId, const DeviceConfig& config) override
    {
        const auto validation = validateAsioConfig(config);
        if (!validation)
        {
            return validation.error();
        }
        if (g_open.exchange(true, std::memory_order_acq_rel))
        {
            return AXF_ERROR(core::ErrorCode::AlreadyExists,
                             "only one ASIO device may be open at a time");
        }

        std::string driverName = deviceId;
        if (driverName.rfind("asio:", 0) == 0)
        {
            driverName.erase(0, 5);
        }
        std::array<char, 64> rawName{};
        std::snprintf(rawName.data(), rawName.size(), "%s", driverName.c_str());

        auto drivers = std::make_unique<AsioDrivers>();
        if (!drivers->loadDriver(rawName.data()) || theAsioDriver == nullptr)
        {
            g_open.store(false, std::memory_order_release);
            theAsioDriver = nullptr;
            return AXF_ERROR(core::ErrorCode::NotFound, "ASIO driver not found");
        }
        if (theAsioDriver->init(nullptr) == ASIOFalse)
        {
            drivers->removeCurrentDriver();
            theAsioDriver = nullptr;
            g_open.store(false, std::memory_order_release);
            return AXF_ERROR(core::ErrorCode::DeviceError, "ASIO driver initialisation failed");
        }

        long inputs = 0;
        long outputs = 0;
        if (theAsioDriver->getChannels(&inputs, &outputs) != ASE_OK ||
            config.inputChannels > static_cast<core::ChannelCount>(std::max(0L, inputs)) ||
            config.outputChannels > static_cast<core::ChannelCount>(std::max(0L, outputs)))
        {
            drivers->removeCurrentDriver();
            theAsioDriver = nullptr;
            g_open.store(false, std::memory_order_release);
            return AXF_ERROR(core::ErrorCode::OutOfRange, "ASIO channel count unsupported");
        }

        ASIOSampleRate currentRate = 0.0;
        if (theAsioDriver->getSampleRate(&currentRate) != ASE_OK ||
            (std::abs(currentRate - static_cast<ASIOSampleRate>(config.sampleRate)) > 0.5 &&
             (theAsioDriver->canSampleRate(static_cast<ASIOSampleRate>(config.sampleRate)) !=
                  ASE_OK ||
              theAsioDriver->setSampleRate(static_cast<ASIOSampleRate>(config.sampleRate)) !=
                  ASE_OK)))
        {
            drivers->removeCurrentDriver();
            theAsioDriver = nullptr;
            g_open.store(false, std::memory_order_release);
            return AXF_ERROR(core::ErrorCode::Unsupported, "ASIO sample rate unsupported");
        }

        long minSize = 0;
        long maxSize = 0;
        long preferredSize = 0;
        long granularity = 0;
        if (theAsioDriver->getBufferSize(&minSize, &maxSize, &preferredSize, &granularity) !=
                ASE_OK ||
            config.bufferFrames < static_cast<std::uint32_t>(std::max(0L, minSize)) ||
            config.bufferFrames > static_cast<std::uint32_t>(std::max(0L, maxSize)) ||
            (granularity > 0 && config.bufferFrames % static_cast<std::uint32_t>(granularity) != 0))
        {
            drivers->removeCurrentDriver();
            theAsioDriver = nullptr;
            g_open.store(false, std::memory_order_release);
            return AXF_ERROR(core::ErrorCode::Unsupported, "ASIO buffer size unsupported");
        }

        std::vector<ASIOBufferInfo> buffers;
        buffers.reserve(config.inputChannels + config.outputChannels);
        for (core::ChannelCount channel = 0; channel < config.inputChannels; ++channel)
        {
            ASIOChannelInfo info{};
            info.channel = static_cast<long>(channel);
            info.isInput = ASIOTrue;
            if (theAsioDriver->getChannelInfo(&info) != ASE_OK || !isFloat32(info.type))
            {
                drivers->removeCurrentDriver();
                theAsioDriver = nullptr;
                g_open.store(false, std::memory_order_release);
                return AXF_ERROR(core::ErrorCode::Unsupported,
                                 "ASIO input channel is not 32-bit float");
            }
            ASIOBufferInfo buffer{};
            buffer.isInput = ASIOTrue;
            buffer.channelNum = static_cast<long>(channel);
            buffers.push_back(buffer);
        }
        for (core::ChannelCount channel = 0; channel < config.outputChannels; ++channel)
        {
            ASIOChannelInfo info{};
            info.channel = static_cast<long>(channel);
            info.isInput = ASIOFalse;
            if (theAsioDriver->getChannelInfo(&info) != ASE_OK || !isFloat32(info.type))
            {
                drivers->removeCurrentDriver();
                theAsioDriver = nullptr;
                g_open.store(false, std::memory_order_release);
                return AXF_ERROR(core::ErrorCode::Unsupported,
                                 "ASIO output channel is not 32-bit float");
            }
            ASIOBufferInfo buffer{};
            buffer.isInput = ASIOFalse;
            buffer.channelNum = static_cast<long>(channel);
            buffers.push_back(buffer);
        }

        DeviceInfo info;
        info.id = std::move(deviceId);
        info.name = driverName;
        info.backend = Backend::Asio;
        info.maxInputChannels = static_cast<core::ChannelCount>(std::max(0L, inputs));
        info.maxOutputChannels = static_cast<core::ChannelCount>(std::max(0L, outputs));
        info.supportedSampleRates = {config.sampleRate};

        auto device = std::make_unique<AsioDevice>(std::move(info), config, std::move(drivers),
                                                   std::move(buffers), config.bufferFrames);
        long inputLatency = 0;
        long outputLatency = 0;
        if (theAsioDriver->getLatencies(&inputLatency, &outputLatency) == ASE_OK)
        {
            device->setLatencySamples(inputLatency, outputLatency);
        }
        if (theAsioDriver->createBuffers(
                device->buffersForCreation(), static_cast<long>(device->bufferCount()),
                static_cast<long>(config.bufferFrames), device->callbacks()) != ASE_OK)
        {
            g_open.store(false, std::memory_order_release);
            theAsioDriver = nullptr;
            return AXF_ERROR(core::ErrorCode::DeviceError, "ASIO buffer creation failed");
        }
        return device;
    }
};

AXF_API std::unique_ptr<IDeviceBackend> createAsioBackendWindows()
{
    return std::make_unique<AsioBackend>();
}

} // namespace axf::devices::detail

#endif // _WIN32 && AXF_HAS_ASIO

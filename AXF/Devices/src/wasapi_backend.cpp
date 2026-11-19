// Windows-only WASAPI backend. This file is compiled only for _WIN32 builds
// with AXF_HAS_WASAPI. No Steinberg SDK is involved.
#if defined(_WIN32) && defined(AXF_HAS_WASAPI)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "axf/devices/device_manager.hpp"

#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <windows.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace axf::devices::detail
{
using Microsoft::WRL::ComPtr;

namespace
{
constexpr REFERENCE_TIME kHundredNanosPerSecond = 10000000;

// 00000001-0000-0010-8000-00AA00389B71
constexpr GUID kIeeeFloatSubtype = {
    0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

bool isIeeeFloatFormat(const WAVEFORMATEX* format) noexcept
{
    if (format == nullptr || format->wBitsPerSample != 32)
    {
        return false;
    }
    if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        return true;
    }
    if (format->wFormatTag != WAVE_FORMAT_EXTENSIBLE ||
        format->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
    {
        return false;
    }
    const auto* extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
    return extensible->SubFormat == kIeeeFloatSubtype;
}

std::string wideToUtf8(const WCHAR* value)
{
    if (value == nullptr)
    {
        return {};
    }
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
    {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.resize(result.size() - 1);
    return result;
}

std::wstring utf8ToWide(const std::string& value)
{
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 0)
    {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    result.resize(result.size() - 1);
    return result;
}

class ComInitializer final
{
  public:
    ComInitializer() noexcept : result_(::CoInitializeEx(nullptr, COINIT_MULTITHREADED))
    {
    }
    ~ComInitializer()
    {
        if (SUCCEEDED(result_))
        {
            ::CoUninitialize();
        }
    }
    ComInitializer(const ComInitializer&) = delete;
    ComInitializer& operator=(const ComInitializer&) = delete;
    [[nodiscard]] bool ok() const noexcept
    {
        return SUCCEEDED(result_);
    }

  private:
    HRESULT result_;
};

bool isDeviceLoss(HRESULT result) noexcept
{
    return result == AUDCLNT_E_DEVICE_INVALIDATED || result == AUDCLNT_E_SERVICE_NOT_RUNNING ||
           result == AUDCLNT_E_RESOURCES_INVALIDATED ||
           result == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
}

float clampSample(float value) noexcept
{
    if (!std::isfinite(value))
    {
        return 0.0F;
    }
    return std::clamp(value, -1.0F, 1.0F);
}

void planarToInterleaved(const float* planar, std::uint8_t* destination, std::uint32_t frames,
                         core::ChannelCount channels, bool isFloat) noexcept
{
    if (isFloat)
    {
        auto* output = reinterpret_cast<float*>(destination);
        for (std::uint32_t frame = 0; frame < frames; ++frame)
        {
            for (core::ChannelCount channel = 0; channel < channels; ++channel)
            {
                output[static_cast<std::size_t>(frame) * channels + channel] =
                    clampSample(planar[static_cast<std::size_t>(channel) * frames + frame]);
            }
        }
        return;
    }

    auto* output = reinterpret_cast<std::int16_t*>(destination);
    for (std::uint32_t frame = 0; frame < frames; ++frame)
    {
        for (core::ChannelCount channel = 0; channel < channels; ++channel)
        {
            const float value =
                clampSample(planar[static_cast<std::size_t>(channel) * frames + frame]);
            output[static_cast<std::size_t>(frame) * channels + channel] =
                static_cast<std::int16_t>(value * 32767.0F);
        }
    }
}

void interleavedToPlanar(const std::uint8_t* source, float* planar, std::uint32_t frames,
                         core::ChannelCount channels, bool isFloat) noexcept
{
    if (isFloat)
    {
        const auto* input = reinterpret_cast<const float*>(source);
        for (std::uint32_t frame = 0; frame < frames; ++frame)
        {
            for (core::ChannelCount channel = 0; channel < channels; ++channel)
            {
                planar[static_cast<std::size_t>(channel) * frames + frame] =
                    clampSample(input[static_cast<std::size_t>(frame) * channels + channel]);
            }
        }
        return;
    }

    const auto* input = reinterpret_cast<const std::int16_t*>(source);
    for (std::uint32_t frame = 0; frame < frames; ++frame)
    {
        for (core::ChannelCount channel = 0; channel < channels; ++channel)
        {
            planar[static_cast<std::size_t>(channel) * frames + frame] =
                static_cast<float>(input[static_cast<std::size_t>(frame) * channels + channel]) /
                32768.0F;
        }
    }
}

DeviceInfo describeDevice(const ComPtr<IMMDevice>& endpoint, EDataFlow flow, Backend backend)
{
    DeviceInfo info;
    info.backend = backend;

    LPWSTR id = nullptr;
    if (SUCCEEDED(endpoint->GetId(&id)) && id != nullptr)
    {
        info.id = wideToUtf8(id);
        ::CoTaskMemFree(id);
    }

    ComPtr<IPropertyStore> properties;
    if (SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, &properties)))
    {
        PROPVARIANT value;
        ::PropVariantInit(&value);
        if (SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &value)) &&
            value.pwszVal != nullptr)
        {
            info.name = wideToUtf8(value.pwszVal);
        }
        ::PropVariantClear(&value);
    }
    if (info.name.empty())
    {
        info.name = info.id.empty() ? "Unknown WASAPI endpoint" : info.id;
    }

    ComPtr<IAudioClient> client;
    if (SUCCEEDED(endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &client)))
    {
        WAVEFORMATEX* format = nullptr;
        if (SUCCEEDED(client->GetMixFormat(&format)) && format != nullptr)
        {
            info.maxInputChannels = flow == eCapture ? format->nChannels : 0;
            info.maxOutputChannels = flow == eRender ? format->nChannels : 0;
            info.supportedSampleRates.push_back(
                static_cast<core::SampleRate>(format->nSamplesPerSec));
            ::CoTaskMemFree(format);
        }
    }
    return info;
}

class WasapiNotificationClient final : public IMMNotificationClient
{
  public:
    explicit WasapiNotificationClient(IDeviceChangeListener* listener) noexcept
        : listener_(listener), references_(1)
    {
    }

    STDMETHODIMP QueryInterface(REFIID identifier, void** result) override
    {
        if (identifier == IID_IUnknown || identifier == IID_IMMNotificationClient)
        {
            *result = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        *result = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return static_cast<ULONG>(::InterlockedIncrement(&references_));
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG remaining = ::InterlockedDecrement(&references_);
        if (remaining == 0)
        {
            delete this;
        }
        return static_cast<ULONG>(remaining);
    }

    STDMETHODIMP OnDeviceStateChanged(LPCWSTR, DWORD) override
    {
        notify();
        return S_OK;
    }
    STDMETHODIMP OnDeviceAdded(LPCWSTR) override
    {
        notify();
        return S_OK;
    }
    STDMETHODIMP OnDeviceRemoved(LPCWSTR) override
    {
        notify();
        return S_OK;
    }
    STDMETHODIMP OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override
    {
        notify();
        return S_OK;
    }
    STDMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        notify();
        return S_OK;
    }

  private:
    void notify() noexcept
    {
        if (listener_ != nullptr)
        {
            listener_->onDevicesChanged();
        }
    }

    IDeviceChangeListener* listener_;
    LONG references_;
};

class WasapiDevice final : public IAudioDevice
{
  public:
    WasapiDevice(const DeviceInfo& info, const DeviceConfig& config,
                 const ComPtr<IMMDevice>& endpoint)
        : info_(info), config_(config), endpoint_(endpoint)
    {
    }

    ~WasapiDevice() override
    {
        stop();
        closeHandles();
    }

    core::Result<void> open() noexcept
    {
        HRESULT result =
            endpoint_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(client_.ReleaseAndGetAddressOf()));
        if (FAILED(result))
        {
            return core::makeError(core::ErrorCode::DeviceError, "IAudioClient activation failed");
        }

        core::ChannelCount channels =
            config_.outputChannels > 0 ? config_.outputChannels : config_.inputChannels;
        if (config_.backend == Backend::WasapiExclusive && channels > 2)
        {
            return core::makeError(core::ErrorCode::Unsupported,
                                   "exclusive mode supports at most two channels in Phase 3");
        }

        if (config_.backend == Backend::WasapiShared)
        {
            WAVEFORMATEX* mixFormat = nullptr;
            result = client_->GetMixFormat(&mixFormat);
            if (FAILED(result) || mixFormat == nullptr)
            {
                return core::makeError(core::ErrorCode::DeviceError, "GetMixFormat failed");
            }
            isFloat_ = mixFormat->wBitsPerSample == 32 &&
                       (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                        mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE);
            channels = static_cast<core::ChannelCount>(mixFormat->nChannels);
            result = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mixFormat, nullptr);
            ::CoTaskMemFree(mixFormat);
            if (FAILED(result))
            {
                return core::makeError(core::ErrorCode::DeviceError,
                                       "WASAPI shared-mode initialisation failed");
            }
        }
        else
        {
            WAVEFORMATEX format{};
            format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            format.nChannels = channels;
            format.nSamplesPerSec = static_cast<DWORD>(config_.sampleRate);
            format.wBitsPerSample = 32;
            format.nBlockAlign = static_cast<WORD>(channels * (format.wBitsPerSample / 8));
            format.nAvgBytesPerSec = format.nSamplesPerSec * static_cast<DWORD>(format.nBlockAlign);
            format.cbSize = 0;
            isFloat_ = true;

            const REFERENCE_TIME duration = static_cast<REFERENCE_TIME>(config_.bufferFrames) *
                                            kHundredNanosPerSecond /
                                            static_cast<REFERENCE_TIME>(config_.sampleRate);
            result =
                client_->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                    duration, duration, &format, nullptr);
            if (FAILED(result))
            {
                return core::makeError(core::ErrorCode::DeviceError,
                                       "WASAPI exclusive-mode initialisation failed");
            }
            audioEvent_ = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (audioEvent_ == nullptr || FAILED(client_->SetEventHandle(audioEvent_)))
            {
                return core::makeError(core::ErrorCode::DeviceError,
                                       "failed to install the WASAPI event handle");
            }
        }

        result = client_->GetBufferSize(&bufferFrames_);
        if (FAILED(result))
        {
            return core::makeError(core::ErrorCode::DeviceError, "GetBufferSize failed");
        }

        if (config_.outputChannels > 0)
        {
            result =
                client_->GetService(__uuidof(IAudioRenderClient), CLSCTX_ALL, nullptr,
                                    reinterpret_cast<void**>(render_.ReleaseAndGetAddressOf()));
        }
        else
        {
            result =
                client_->GetService(__uuidof(IAudioCaptureClient), CLSCTX_ALL, nullptr,
                                    reinterpret_cast<void**>(capture_.ReleaseAndGetAddressOf()));
        }
        if (FAILED(result))
        {
            return core::makeError(core::ErrorCode::DeviceError,
                                   "failed to obtain the WASAPI render/capture client");
        }

        channelCount_ = channels;
        planar_.assign(static_cast<std::size_t>(channels) * bufferFrames_, 0.0F);
        channelPointers_.assign(channels, nullptr);
        stopEvent_ = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (stopEvent_ == nullptr)
        {
            return core::makeError(core::ErrorCode::DeviceError, "failed to create the stop event");
        }
        state_.store(DeviceState::Active, std::memory_order_release);

        if (isFloat_)
        {
            info_.supportedSampleRates.assign(1, static_cast<core::SampleRate>(config_.sampleRate));
        }
        return core::success();
    }

    [[nodiscard]] const DeviceInfo& info() const noexcept override
    {
        return info_;
    }
    [[nodiscard]] const DeviceConfig& config() const noexcept override
    {
        return config_;
    }

    [[nodiscard]] core::Result<void> start(IAudioDeviceCallback* callback) override
    {
        if (callback == nullptr)
        {
            return core::makeError(core::ErrorCode::InvalidArgument, "audio callback is null");
        }
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true))
        {
            return core::makeError(core::ErrorCode::AlreadyExists, "device is already running");
        }
        callback_ = callback;
        ::ResetEvent(stopEvent_);
        worker_ = std::thread([this] { run(); });
        return core::success();
    }

    void stop() noexcept override
    {
        if (!running_.exchange(false))
        {
            return;
        }
        if (stopEvent_ != nullptr)
        {
            ::SetEvent(stopEvent_);
        }
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    [[nodiscard]] bool isRunning() const noexcept
    {
        return running_.load(std::memory_order_acquire);
    }

    [[nodiscard]] double reportedLatencySeconds() const noexcept override
    {
        return static_cast<double>(bufferFrames_) / static_cast<double>(config_.sampleRate);
    }

    [[nodiscard]] DeviceState state() const noexcept override
    {
        return state_.load(std::memory_order_acquire);
    }

    [[nodiscard]] DeviceDiagnostics diagnostics() const noexcept override
    {
        DeviceDiagnostics result;
        result.callbackCount = callbackCount_.load(std::memory_order_relaxed);
        result.underrunCount = underrunCount_.load(std::memory_order_relaxed);
        result.overrunCount = overrunCount_.load(std::memory_order_relaxed);
        result.lastCallbackFrames = lastFrames_.load(std::memory_order_relaxed);
        result.lastCallbackMicroseconds = lastMicroseconds_.load(std::memory_order_relaxed);
        return result;
    }

  private:
    void closeHandles() noexcept
    {
        if (audioEvent_ != nullptr)
        {
            ::CloseHandle(audioEvent_);
            audioEvent_ = nullptr;
        }
        if (stopEvent_ != nullptr)
        {
            ::CloseHandle(stopEvent_);
            stopEvent_ = nullptr;
        }
    }

    bool stopping() const noexcept
    {
        return stopEvent_ == nullptr || ::WaitForSingleObject(stopEvent_, 0) == WAIT_OBJECT_0;
    }

    void run() noexcept
    {
        ComInitializer com;
        if (!com.ok())
        {
            state_.store(DeviceState::Disconnected, std::memory_order_release);
            return;
        }
        if (FAILED(client_->Start()))
        {
            state_.store(DeviceState::Disconnected, std::memory_order_release);
            return;
        }

        for (;;)
        {
            if (stopping())
            {
                break;
            }
            if (config_.backend == Backend::WasapiExclusive)
            {
                HANDLE handles[2] = {stopEvent_, audioEvent_};
                const DWORD wait = ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                if (wait == WAIT_OBJECT_0 || wait == WAIT_FAILED)
                {
                    break;
                }
            }

            const auto began = std::chrono::steady_clock::now();
            if (render_ != nullptr)
            {
                if (!renderBlock())
                {
                    break;
                }
            }
            else if (!captureBlock())
            {
                break;
            }

            const auto elapsed = std::chrono::steady_clock::now() - began;
            lastMicroseconds_.store(std::chrono::duration<double, std::micro>(elapsed).count(),
                                    std::memory_order_relaxed);
            callbackCount_.fetch_add(1, std::memory_order_relaxed);
        }

        client_->Stop();
        state_.store(stopping() ? DeviceState::Unknown : DeviceState::Disconnected,
                     std::memory_order_release);
    }

    bool renderBlock() noexcept
    {
        UINT32 padding = 0;
        if (FAILED(client_->GetCurrentPadding(&padding)))
        {
            return fail();
        }
        UINT32 frames = bufferFrames_ - padding;
        if (config_.backend == Backend::WasapiShared)
        {
            if (frames == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return true;
            }
        }
        else
        {
            frames = bufferFrames_;
        }

        std::uint8_t* data = nullptr;
        if (FAILED(render_->GetBuffer(frames, &data)) || data == nullptr)
        {
            return fail();
        }
        for (core::ChannelCount channel = 0; channel < channelCount_; ++channel)
        {
            channelPointers_[channel] = planar_.data() + static_cast<std::size_t>(channel) * frames;
        }
        callback_->onAudioCallback(nullptr, channelPointers_.data(), frames, streamFrame_);
        planarToInterleaved(planar_.data(), data, frames, channelCount_, isFloat_);
        streamFrame_ += frames;
        lastFrames_.store(frames, std::memory_order_relaxed);
        return SUCCEEDED(render_->ReleaseBuffer(frames, 0));
    }

    bool captureBlock() noexcept
    {
        UINT32 packet = 0;
        if (FAILED(capture_->GetNextPacketSize(&packet)))
        {
            return fail();
        }
        if (packet == 0)
        {
            if (config_.backend == Backend::WasapiShared)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return true;
        }

        while (packet > 0)
        {
            std::uint8_t* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture_->GetBuffer(&data, &frames, &flags, nullptr, nullptr)) ||
                data == nullptr)
            {
                return fail();
            }
            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0)
            {
                std::fill(planar_.begin(), planar_.end(), 0.0F);
            }
            else
            {
                for (core::ChannelCount channel = 0; channel < channelCount_; ++channel)
                {
                    channelPointers_[channel] =
                        planar_.data() + static_cast<std::size_t>(channel) * frames;
                }
                interleavedToPlanar(data, planar_.data(), frames, channelCount_, isFloat_);
            }
            callback_->onAudioCallback(channelPointers_.data(), nullptr, frames, streamFrame_);
            streamFrame_ += frames;
            lastFrames_.store(frames, std::memory_order_relaxed);
            if (FAILED(capture_->ReleaseBuffer(frames)))
            {
                return fail();
            }
            if (FAILED(capture_->GetNextPacketSize(&packet)))
            {
                return fail();
            }
        }
        return true;
    }

    bool fail() noexcept
    {
        underrunCount_.fetch_add(1, std::memory_order_relaxed);
        state_.store(DeviceState::Disconnected, std::memory_order_release);
        return false;
    }

    DeviceInfo info_;
    DeviceConfig config_;
    ComPtr<IMMDevice> endpoint_;
    ComPtr<IAudioClient> client_;
    ComPtr<IAudioRenderClient> render_;
    ComPtr<IAudioCaptureClient> capture_;
    HANDLE stopEvent_{nullptr};
    HANDLE audioEvent_{nullptr};
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<DeviceState> state_{DeviceState::Unknown};
    IAudioDeviceCallback* callback_{nullptr};
    std::vector<float> planar_;
    std::vector<float*> channelPointers_;
    core::ChannelCount channelCount_{0};
    UINT32 bufferFrames_{0};
    bool isFloat_{true};
    std::uint64_t streamFrame_{0};
    std::atomic<std::uint64_t> callbackCount_{0};
    std::atomic<std::uint64_t> underrunCount_{0};
    std::atomic<std::uint64_t> overrunCount_{0};
    std::atomic<std::uint32_t> lastFrames_{0};
    std::atomic<double> lastMicroseconds_{0.0};
};

class WasapiBackend final : public IDeviceBackend
{
  public:
    explicit WasapiBackend(Backend backend) : backend_(backend)
    {
    }

    ~WasapiBackend() override
    {
        if (enumerator_ != nullptr && notification_ != nullptr)
        {
            enumerator_->UnregisterEndpointNotificationCallback(notification_);
        }
        if (notification_ != nullptr)
        {
            notification_->Release();
            notification_ = nullptr;
        }
    }

    [[nodiscard]] Backend backend() const noexcept override
    {
        return backend_;
    }

    void setChangeListener(IDeviceChangeListener* listener) noexcept override
    {
        ComInitializer com;
        if (!com.ok())
        {
            return;
        }
        if (enumerator_ == nullptr)
        {
            if (FAILED(::CoCreateInstance(
                    CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                    reinterpret_cast<void**>(enumerator_.ReleaseAndGetAddressOf()))))
            {
                return;
            }
        }
        if (notification_ != nullptr)
        {
            enumerator_->UnregisterEndpointNotificationCallback(notification_);
            notification_->Release();
            notification_ = nullptr;
        }
        if (listener != nullptr)
        {
            notification_ = new WasapiNotificationClient(listener);
            enumerator_->RegisterEndpointNotificationCallback(notification_);
        }
    }

    [[nodiscard]] core::Result<std::vector<DeviceInfo>> enumerate() override
    {
        ComInitializer com;
        if (!com.ok())
        {
            return core::makeError(core::ErrorCode::ThreadError, "CoInitializeEx failed");
        }
        ComPtr<IMMDeviceEnumerator> enumerator;
        if (FAILED(::CoCreateInstance(
                CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(enumerator.ReleaseAndGetAddressOf()))))
        {
            return core::makeError(core::ErrorCode::DeviceError,
                                   "failed to create the WASAPI device enumerator");
        }

        std::vector<DeviceInfo> result;
        for (EDataFlow flow : {eRender, eCapture})
        {
            ComPtr<IMMDeviceCollection> collection;
            if (FAILED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection)))
            {
                continue;
            }
            UINT count = 0;
            collection->GetCount(&count);
            for (UINT index = 0; index < count; ++index)
            {
                ComPtr<IMMDevice> endpoint;
                if (SUCCEEDED(collection->Item(index, &endpoint)))
                {
                    result.push_back(describeDevice(endpoint, flow, backend_));
                }
            }
        }
        return result;
    }

    [[nodiscard]] core::Result<std::unique_ptr<IAudioDevice>> open(
        const std::string& deviceId, const DeviceConfig& config) override
    {
        const auto validation = validateWasapiConfig(config);
        if (!validation)
        {
            return validation.error();
        }
        if (config.backend != backend_)
        {
            return core::makeError(core::ErrorCode::InvalidArgument,
                                   "configuration backend does not match this WASAPI backend");
        }

        ComInitializer com;
        if (!com.ok())
        {
            return core::makeError(core::ErrorCode::ThreadError, "CoInitializeEx failed");
        }
        ComPtr<IMMDeviceEnumerator> enumerator;
        if (FAILED(::CoCreateInstance(
                CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(enumerator.ReleaseAndGetAddressOf()))))
        {
            return core::makeError(core::ErrorCode::DeviceError,
                                   "failed to create the WASAPI device enumerator");
        }

        ComPtr<IMMDevice> endpoint;
        const std::wstring wideId = utf8ToWide(deviceId);
        if (FAILED(enumerator->GetDevice(wideId.c_str(), &endpoint)))
        {
            return core::makeError(core::ErrorCode::NotFound, "WASAPI device not found");
        }

        const EDataFlow flow = config.outputChannels > 0 ? eRender : eCapture;
        DeviceInfo info = describeDevice(endpoint, flow, backend_);
        info.id = deviceId;
        info.supportedSampleRates.assign(1, config.sampleRate);

        auto device = std::make_unique<WasapiDevice>(info, config, endpoint);
        const auto opened = device->open();
        if (!opened)
        {
            return opened.error();
        }
        std::unique_ptr<IAudioDevice> result = std::move(device);
        return result;
    }

  private:
    Backend backend_;
    ComPtr<IMMDeviceEnumerator> enumerator_;
    IMMNotificationClient* notification_{nullptr};
};

} // namespace

std::unique_ptr<IDeviceBackend> createWasapiBackendWindows()
{
    return std::make_unique<WasapiBackend>(Backend::WasapiShared);
}

std::unique_ptr<IDeviceBackend> createWasapiExclusiveBackendWindows()
{
    return std::make_unique<WasapiBackend>(Backend::WasapiExclusive);
}

} // namespace axf::devices::detail
#endif

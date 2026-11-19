#include "axf/plugins/plugin_host.hpp"

#if defined(AXF_HAS_VST3) && AXF_HAS_VST3

#if defined(_MSC_VER)
#pragma warning(push, 0)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/processdata.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace axf::plugins
{
namespace
{
namespace fs = std::filesystem;
using Steinberg::IPtr;
using Steinberg::Vst::EventList;
using Steinberg::Vst::HostProcessData;
using Steinberg::Vst::IAudioProcessor;
using Steinberg::Vst::IComponent;
using Steinberg::Vst::IEditController;
using Steinberg::Vst::PlugProvider;
using Steinberg::Vst::ProcessContext;
using VST3::Hosting::ClassInfo;
using VST3::Hosting::Module;

IPtr<Steinberg::Vst::HostApplication>& hostContext() noexcept
{
    static IPtr<Steinberg::Vst::HostApplication> context;
    return context;
}

void ensureHostContext() noexcept
{
    if (!hostContext())
    {
        hostContext() = Steinberg::owned(new Steinberg::Vst::HostApplication());
        Steinberg::Vst::PluginContextFactory::instance().setPluginContext(hostContext());
    }
}

bool hasVst3Extension(const fs::path& path) noexcept
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension == ".vst3";
}

std::uint32_t toFrames(std::uint64_t frame, std::size_t frames) noexcept
{
    if (frame >= frames)
    {
        return frames == 0 ? 0U : static_cast<std::uint32_t>(frames - 1U);
    }
    return static_cast<std::uint32_t>(frame);
}

constexpr std::array<std::byte, 4> kStateMagic{std::byte{'A'}, std::byte{'X'}, std::byte{'S'},
                                               std::byte{'1'}};

void appendUint64(std::vector<std::byte>& bytes, std::uint64_t value) noexcept
{
    for (std::size_t byte = 0; byte < 8; ++byte)
    {
        bytes.push_back(static_cast<std::byte>((value >> (8U * byte)) & 0xFFU));
    }
}

[[nodiscard]] bool readUint64(const std::vector<std::byte>& bytes, std::size_t& offset,
                              std::uint64_t& value) noexcept
{
    if (bytes.size() - offset < 8)
    {
        return false;
    }
    value = 0;
    for (std::size_t byte = 0; byte < 8; ++byte)
    {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[offset + byte]))
                 << (8U * byte);
    }
    offset += 8;
    return true;
}

[[nodiscard]] std::vector<std::byte> streamBytes(Steinberg::MemoryStream& stream) noexcept
{
    const auto size = static_cast<std::size_t>(std::max<Steinberg::TSize>(0, stream.getSize()));
    std::vector<std::byte> bytes(size);
    if (size == 0)
    {
        return bytes;
    }
    stream.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);
    Steinberg::int32 read = 0;
    if (stream.read(bytes.data(), static_cast<Steinberg::int32>(size), &read) !=
            Steinberg::kResultOk ||
        read <= 0)
    {
        return {};
    }
    bytes.resize(static_cast<std::size_t>(read));
    return bytes;
}

class Vst3Instance final : public IPluginInstance
{
  public:
    Vst3Instance(std::shared_ptr<Module> module, IPtr<PlugProvider> provider,
                 IPtr<IComponent> component, IPtr<IAudioProcessor> processor,
                 IPtr<IEditController> controller)
        : module_(std::move(module)), provider_(std::move(provider)),
          component_(std::move(component)), processor_(std::move(processor)),
          controller_(std::move(controller)), pointers_(512, nullptr)
    {
        processData_.processContext = &context_;
        processData_.inputEvents = &eventList_;
        processData_.outputEvents = nullptr;
        processData_.inputParameterChanges = nullptr;
        processData_.outputParameterChanges = nullptr;
        context_.tempo = 120.0;
        context_.timeSigNumerator = 4;
        context_.timeSigDenominator = 4;
    }

    ~Vst3Instance() override
    {
        if (prepared_)
        {
            processor_->setProcessing(false);
            component_->setActive(false);
            processData_.unprepare();
        }
    }

    Vst3Instance(const Vst3Instance&) = delete;
    Vst3Instance& operator=(const Vst3Instance&) = delete;

    void prepare(double sampleRate, std::uint32_t maxFrames) noexcept override
    {
        if (prepared_ || !processor_ || !component_)
        {
            return;
        }
        Steinberg::Vst::ProcessSetup setup{};
        setup.processMode = Steinberg::Vst::kRealtime;
        setup.symbolicSampleSize = Steinberg::Vst::kSample32;
        setup.maxSamplesPerBlock = static_cast<Steinberg::int32>(maxFrames);
        setup.sampleRate = static_cast<Steinberg::Vst::SampleRate>(sampleRate);
        if (processor_->setupProcessing(setup) != Steinberg::kResultOk ||
            !processData_.prepare(*component_, 0, Steinberg::Vst::kSample32) ||
            component_->setActive(true) != Steinberg::kResultOk ||
            processor_->setProcessing(true) != Steinberg::kResultOk)
        {
            processor_->setProcessing(false);
            component_->setActive(false);
            processData_.unprepare();
            return;
        }
        sampleRate_ = sampleRate;
        context_.sampleRate = sampleRate;
        prepared_ = true;
    }

    void reset() noexcept override
    {
        if (processor_)
        {
            // VST3 has no direct reset(); toggling processing is the SDK's
            // documented way to clear processor state such as delay tails.
            processor_->setProcessing(false);
            processor_->setProcessing(true);
        }
    }

    void process(audio::AudioBufferView& audio, midi::MidiEventBufferView& midi) noexcept override
    {
        if (!prepared_ || !audio.valid())
        {
            return;
        }
        eventList_.clear();
        for (std::size_t i = 0; i < midi.size; ++i)
        {
            const midi::MidiEvent& source = midi.data[i];
            Steinberg::Vst::Event event{};
            event.busIndex = 0;
            event.sampleOffset =
                static_cast<Steinberg::int32>(toFrames(source.frame, audio.frames));
            event.flags = Steinberg::Vst::Event::kIsLive;

            switch (midi::messageType(source))
            {
            case midi::MidiMessageType::NoteOn:
            {
                Steinberg::Vst::NoteOnEvent note{};
                note.channel = static_cast<Steinberg::int16>(midi::eventChannel(source));
                note.pitch = source.data1;
                note.velocity = static_cast<float>(source.data2) / 127.0F;
                note.noteId = -1;
                event.noteOn = note;
                break;
            }
            case midi::MidiMessageType::NoteOff:
            {
                Steinberg::Vst::NoteOffEvent note{};
                note.channel = static_cast<Steinberg::int16>(midi::eventChannel(source));
                note.pitch = source.data1;
                note.velocity = static_cast<float>(source.data2) / 127.0F;
                note.noteId = -1;
                event.noteOff = note;
                break;
            }
            case midi::MidiMessageType::Aftertouch:
            {
                Steinberg::Vst::PolyPressureEvent pressure{};
                pressure.channel = static_cast<Steinberg::int16>(midi::eventChannel(source));
                pressure.pitch = source.data1;
                pressure.pressure = static_cast<float>(source.data2) / 127.0F;
                event.polyPressure = pressure;
                break;
            }
            default:
                continue;
            }
            eventList_.addEvent(event);
        }

        for (Steinberg::int32 bus = 0; bus < processData_.numInputs; ++bus)
        {
            const auto channels = static_cast<std::size_t>(
                std::max<Steinberg::int32>(0, processData_.inputs[bus].numChannels));
            const auto count = std::min(channels, static_cast<std::size_t>(audio.channels));
            for (std::size_t channel = 0; channel < count; ++channel)
            {
                pointers_[channel] = audio.channelData[channel];
            }
            processData_.setChannelBuffers(Steinberg::Vst::kInput, bus, pointers_.data(),
                                           static_cast<Steinberg::int32>(count));
        }
        for (Steinberg::int32 bus = 0; bus < processData_.numOutputs; ++bus)
        {
            const auto channels = static_cast<std::size_t>(
                std::max<Steinberg::int32>(0, processData_.outputs[bus].numChannels));
            const auto count = std::min(channels, static_cast<std::size_t>(audio.channels));
            for (std::size_t channel = 0; channel < count; ++channel)
            {
                pointers_[channel] = audio.channelData[channel];
            }
            processData_.setChannelBuffers(Steinberg::Vst::kOutput, bus, pointers_.data(),
                                           static_cast<Steinberg::int32>(count));
        }

        processData_.numSamples = static_cast<Steinberg::int32>(audio.frames);
        context_.sampleRate = sampleRate_;
        processor_->process(processData_);
    }

    [[nodiscard]] std::uint32_t latencySamples() const noexcept override
    {
        return processor_ ? static_cast<std::uint32_t>(
                                std::max<Steinberg::int32>(0, processor_->getLatencySamples()))
                          : 0U;
    }

    [[nodiscard]] core::Result<std::vector<std::byte>> getState() const override
    {
        if (!component_)
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "VST3 component is unavailable");
        }
        Steinberg::MemoryStream componentStream;
        if (component_->getState(&componentStream) != Steinberg::kResultOk)
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "VST3 component state read failed");
        }
        componentStream.truncateToCursor();
        const auto componentBytes = streamBytes(componentStream);

        std::vector<std::byte> controllerBytes;
        if (controller_)
        {
            Steinberg::MemoryStream controllerStream;
            if (controller_->getState(&controllerStream) == Steinberg::kResultOk)
            {
                controllerStream.truncateToCursor();
                controllerBytes = streamBytes(controllerStream);
            }
        }

        std::vector<std::byte> framed;
        framed.reserve(kStateMagic.size() + 16U + componentBytes.size() + controllerBytes.size());
        framed.insert(framed.end(), kStateMagic.begin(), kStateMagic.end());
        appendUint64(framed, componentBytes.size());
        framed.insert(framed.end(), componentBytes.begin(), componentBytes.end());
        appendUint64(framed, controllerBytes.size());
        framed.insert(framed.end(), controllerBytes.begin(), controllerBytes.end());
        return framed;
    }

    [[nodiscard]] core::Result<void> setState(const std::vector<std::byte>& state) override
    {
        if (!component_)
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "VST3 component is unavailable");
        }
        if (state.size() < kStateMagic.size() ||
            !std::equal(kStateMagic.begin(), kStateMagic.end(), state.begin()))
        {
            return AXF_ERROR(core::ErrorCode::InvalidArgument, "unrecognised VST3 state format");
        }
        std::size_t offset = kStateMagic.size();
        std::uint64_t componentSize = 0;
        std::uint64_t controllerSize = 0;
        if (!readUint64(state, offset, componentSize) || componentSize > state.size() - offset)
        {
            return AXF_ERROR(core::ErrorCode::InvalidArgument, "invalid VST3 component state size");
        }
        const auto componentBegin = state.begin() + static_cast<std::ptrdiff_t>(offset);
        const std::vector<std::byte> componentBytes(
            componentBegin, componentBegin + static_cast<std::ptrdiff_t>(componentSize));
        offset += static_cast<std::size_t>(componentSize);
        if (!readUint64(state, offset, controllerSize) || controllerSize > state.size() - offset)
        {
            return AXF_ERROR(core::ErrorCode::InvalidArgument,
                             "invalid VST3 controller state size");
        }
        const auto controllerBegin = state.begin() + static_cast<std::ptrdiff_t>(offset);
        const std::vector<std::byte> controllerBytes(
            controllerBegin, controllerBegin + static_cast<std::ptrdiff_t>(controllerSize));
        offset += static_cast<std::size_t>(controllerSize);
        if (offset != state.size())
        {
            return AXF_ERROR(core::ErrorCode::InvalidArgument, "trailing VST3 state bytes");
        }

        void* componentMemory = const_cast<void*>(static_cast<const void*>(componentBytes.data()));
        Steinberg::MemoryStream componentStream(
            componentMemory, static_cast<Steinberg::int64>(componentBytes.size()));
        if (!componentBytes.empty() &&
            component_->setState(&componentStream) != Steinberg::kResultOk)
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "VST3 component state write failed");
        }
        if (!controllerBytes.empty() && controller_)
        {
            void* controllerMemory =
                const_cast<void*>(static_cast<const void*>(controllerBytes.data()));
            Steinberg::MemoryStream controllerStream(
                controllerMemory, static_cast<Steinberg::int64>(controllerBytes.size()));
            if (controller_->setState(&controllerStream) != Steinberg::kResultOk)
            {
                return AXF_ERROR(core::ErrorCode::PluginError,
                                 "VST3 controller state write failed");
            }
        }
        return core::success();
    }

  private:
    std::shared_ptr<Module> module_;
    IPtr<PlugProvider> provider_;
    IPtr<IComponent> component_;
    IPtr<IAudioProcessor> processor_;
    IPtr<IEditController> controller_;
    HostProcessData processData_;
    EventList eventList_{64};
    ProcessContext context_{};
    std::vector<float*> pointers_;
    double sampleRate_{48000.0};
    bool prepared_{false};
};

class Vst3Host final : public IPluginHost
{
  public:
    [[nodiscard]] core::Result<std::vector<PluginDescriptor>> scan(
        const std::string& searchPath) override
    {
        ensureHostContext();
        std::error_code error;
        fs::directory_iterator iterator(fs::path(searchPath), error);
        if (error)
        {
            return AXF_ERROR(core::ErrorCode::IoError, "cannot open VST3 search path");
        }

        std::vector<PluginDescriptor> descriptors;
        for (const fs::directory_entry& entry : iterator)
        {
            if (!hasVst3Extension(entry.path()))
            {
                continue;
            }
            std::string moduleError;
            auto module = Module::create(entry.path().string(), moduleError);
            if (!module)
            {
                continue;
            }
            const auto factory = module->getFactory();
            for (const ClassInfo& info : factory.classInfos())
            {
                if (info.category() != kVstAudioEffectClass)
                {
                    continue;
                }
                PluginDescriptor descriptor;
                descriptor.id = info.ID().toString();
                descriptor.name = info.name();
                descriptor.vendor = info.vendor();
                descriptor.version = info.version();
                descriptor.path = entry.path().string();
                descriptor.format = PluginFormat::Vst3;
                descriptors.push_back(std::move(descriptor));
            }
        }
        return descriptors;
    }

    [[nodiscard]] core::Result<std::unique_ptr<IPluginInstance>> create(
        const PluginDescriptor& descriptor) override
    {
        if (descriptor.format != PluginFormat::Vst3 || descriptor.path.empty() ||
            descriptor.id.empty())
        {
            return AXF_ERROR(core::ErrorCode::InvalidArgument, "invalid VST3 descriptor");
        }
        ensureHostContext();
        if (descriptor.id.empty())
        {
            return AXF_ERROR(core::ErrorCode::InvalidArgument, "invalid VST3 class ID");
        }
        std::string moduleError;
        auto module = Module::create(descriptor.path, moduleError);
        if (!module)
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "cannot load VST3 module");
        }
        const auto factory = module->getFactory();
        ClassInfo selected;
        bool found = false;
        for (const ClassInfo& info : factory.classInfos())
        {
            if (info.category() == kVstAudioEffectClass && info.ID().toString() == descriptor.id)
            {
                selected = info;
                found = true;
                break;
            }
        }
        if (!found)
        {
            return AXF_ERROR(core::ErrorCode::NotFound, "VST3 class not found in module");
        }

        IPtr<PlugProvider> provider = Steinberg::owned(new PlugProvider(factory, selected, true));
        if (!provider->initialize())
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "VST3 plug-in initialisation failed");
        }
        IPtr<IComponent> component = provider->getComponent();
        if (!component)
        {
            return AXF_ERROR(core::ErrorCode::PluginError, "VST3 component creation failed");
        }
        Steinberg::FUnknownPtr<IAudioProcessor> processor(component.get());
        if (!processor)
        {
            return AXF_ERROR(core::ErrorCode::Unsupported, "VST3 audio processor unavailable");
        }
        IPtr<IEditController> controller = provider->getController();

        auto instance = std::make_unique<Vst3Instance>(std::move(module), std::move(provider),
                                                       std::move(component), std::move(processor),
                                                       std::move(controller));
        return std::unique_ptr<IPluginInstance>(std::move(instance));
    }
};
} // namespace

std::unique_ptr<IPluginHost> createVst3Host()
{
    return std::make_unique<Vst3Host>();
}

} // namespace axf::plugins

#else // !AXF_HAS_VST3

#include "axf/plugins/plugin_host.hpp"

namespace axf::plugins
{
std::unique_ptr<IPluginHost> createVst3Host()
{
    return nullptr;
}
} // namespace axf::plugins

#endif

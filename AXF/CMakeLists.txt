# AXF is an independent SDK. Nothing in this directory may depend on AudioX/.
axf_add_module(Platform ${CMAKE_CURRENT_SOURCE_DIR}/Platform ${CMAKE_CURRENT_SOURCE_DIR}/Platform/src/platform.cpp)
axf_add_module(Core ${CMAKE_CURRENT_SOURCE_DIR}/Core      ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/error.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/log.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/thread.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/rt.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/event.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/job_system.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/config.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/time.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/memory.cpp)
axf_add_module(Audio ${CMAKE_CURRENT_SOURCE_DIR}/Audio ${CMAKE_CURRENT_SOURCE_DIR}/Audio/src/audio.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Audio/src/compiled_graph.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Audio/src/engine.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Audio/src/buffer_pool.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Audio/src/scheduler.cpp)
axf_add_module(Devices ${CMAKE_CURRENT_SOURCE_DIR}/Devices ${CMAKE_CURRENT_SOURCE_DIR}/Devices/src/device.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Devices/src/device_manager.cpp)
axf_add_module(DSP ${CMAKE_CURRENT_SOURCE_DIR}/DSP       ${CMAKE_CURRENT_SOURCE_DIR}/DSP/src/dsp.cpp ${CMAKE_CURRENT_SOURCE_DIR}/DSP/src/gain_processor.cpp ${CMAKE_CURRENT_SOURCE_DIR}/DSP/src/biquad.cpp ${CMAKE_CURRENT_SOURCE_DIR}/DSP/src/panner.cpp)
axf_add_module(Graph ${CMAKE_CURRENT_SOURCE_DIR}/Graph     ${CMAKE_CURRENT_SOURCE_DIR}/Graph/src/graph.cpp)
axf_add_module(Plugins ${CMAKE_CURRENT_SOURCE_DIR}/Plugins   ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/src/plugin_host.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/src/vst3_host.cpp)
axf_add_module(MIDI ${CMAKE_CURRENT_SOURCE_DIR}/MIDI      ${CMAKE_CURRENT_SOURCE_DIR}/MIDI/src/event.cpp ${CMAKE_CURRENT_SOURCE_DIR}/MIDI/src/smf.cpp)
axf_add_module(Media ${CMAKE_CURRENT_SOURCE_DIR}/Media     ${CMAKE_CURRENT_SOURCE_DIR}/Media/src/audio_file.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Media/src/wav_file.cpp)
axf_add_module(Project ${CMAKE_CURRENT_SOURCE_DIR}/Project   ${CMAKE_CURRENT_SOURCE_DIR}/Project/src/project.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Project/src/document.cpp)
axf_add_module(GUI ${CMAKE_CURRENT_SOURCE_DIR}/GUI       ${CMAKE_CURRENT_SOURCE_DIR}/GUI/src/theme.cpp ${CMAKE_CURRENT_SOURCE_DIR}/GUI/src/widgets.cpp ${CMAKE_CURRENT_SOURCE_DIR}/GUI/src/window_stub.cpp)

target_link_libraries(axf_core PUBLIC Threads::Threads)
target_link_libraries(axf_platform PUBLIC axf_core)
target_link_libraries(axf_audio PUBLIC axf_core axf_graph)
target_link_libraries(axf_devices PUBLIC axf_core axf_audio)
target_link_libraries(axf_dsp PUBLIC axf_core axf_audio)
target_link_libraries(axf_graph PUBLIC axf_core)
target_link_libraries(axf_plugins PUBLIC axf_core axf_audio axf_midi)
target_link_libraries(axf_midi PUBLIC axf_core)
target_link_libraries(axf_media PUBLIC axf_core axf_audio)
target_link_libraries(axf_project PUBLIC axf_core axf_audio axf_midi axf_graph)
target_link_libraries(axf_gui PUBLIC axf_core axf_audio)

if(AXF_ENABLE_VST3 OR AXF_ENABLE_ASIO)
  target_compile_definitions(axf_plugins PUBLIC AXF_HAS_VST3=$<BOOL:${AXF_ENABLE_VST3}>)
  target_compile_definitions(axf_devices PUBLIC AXF_HAS_ASIO=$<BOOL:${AXF_ENABLE_ASIO}>)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/AXFConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/AXFConfig.cmake
  INSTALL_DESTINATION lib/cmake/AXF)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AXFConfig.cmake DESTINATION lib/cmake/AXF)
install(EXPORT AXFTargets FILE AXFTargets.cmake NAMESPACE AXF:: DESTINATION lib/cmake/AXF)

if(WIN32)
  target_sources(axf_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/GUI/src/win32_window.cpp)
  target_link_libraries(axf_gui PRIVATE d2d1 dwrite)
endif()

if(WIN32)
  target_sources(axf_devices PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Devices/src/wasapi_backend.cpp)
  target_compile_definitions(axf_devices PRIVATE AXF_HAS_WASAPI=1)
  target_link_libraries(axf_devices PRIVATE ole32 uuid)
endif()

if(AXF_ENABLE_VST3)
  # The SDK hosting target deliberately omits its platform module and memory
  # stream implementation; provide them as a build-only library so AXF never
  # installs or vendors SDK sources.
  if(WIN32)
    set(AXF_VST3_MODULE_SOURCE ${AXF_VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_win32.cpp)
  elseif(UNIX AND NOT APPLE)
    set(AXF_VST3_MODULE_SOURCE ${AXF_VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_linux.cpp)
  else()
    message(FATAL_ERROR "AXF_ENABLE_VST3 is currently supported only on Windows and Linux")
  endif()
  add_library(axf_vst3_platform STATIC
    ${AXF_VST3_MODULE_SOURCE}
    ${AXF_VST3_SDK_ROOT}/public.sdk/source/common/memorystream.cpp
    ${AXF_VST3_SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp)
  target_include_directories(axf_vst3_platform PUBLIC ${AXF_VST3_SDK_ROOT})
  target_compile_features(axf_vst3_platform PUBLIC cxx_std_20)
  if(MSVC)
    # The SDK's Win32 module source still uses u8 literals as char pointers.
    target_compile_options(axf_vst3_platform PRIVATE /W0 /Zc:char8_t-)
  else()
    target_compile_options(axf_vst3_platform PRIVATE -w)
  endif()
  target_link_libraries(axf_vst3_platform PUBLIC sdk_hosting)

  target_link_libraries(axf_plugins PRIVATE
    $<BUILD_INTERFACE:axf_vst3_platform>
    $<BUILD_INTERFACE:sdk_hosting>)
endif()

if(WIN32 AND AXF_ENABLE_ASIO)
  # The SDK is consumed from AXF_ASIO_SDK_ROOT and is never copied into the
  # repository or installed with AXF. ASIO-enabled binaries inherit the SDK's
  # GPLv3 option unless a separate Steinberg proprietary licence is obtained.
  add_library(axf_asio_sdk STATIC
    ${AXF_ASIO_SDK_ROOT}/common/asio.cpp
    ${AXF_ASIO_SDK_ROOT}/common/combase.cpp
    ${AXF_ASIO_SDK_ROOT}/host/asiodrivers.cpp
    ${AXF_ASIO_SDK_ROOT}/host/pc/asiolist.cpp)
  target_include_directories(axf_asio_sdk PUBLIC
    ${AXF_ASIO_SDK_ROOT}/common
    ${AXF_ASIO_SDK_ROOT}/host
    ${AXF_ASIO_SDK_ROOT}/host/pc)
  target_compile_features(axf_asio_sdk PUBLIC cxx_std_20)
  if(MSVC)
    target_compile_options(axf_asio_sdk PRIVATE /W0)
  else()
    target_compile_options(axf_asio_sdk PRIVATE -w)
  endif()
  target_link_libraries(axf_devices PRIVATE $<BUILD_INTERFACE:axf_asio_sdk> ole32 advapi32)
  target_sources(axf_devices PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Devices/src/asio_backend.cpp)
endif()

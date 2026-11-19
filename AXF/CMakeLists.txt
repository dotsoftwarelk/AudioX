# AXF is an independent SDK. Nothing in this directory may depend on AudioX/.
axf_add_module(Platform ${CMAKE_CURRENT_SOURCE_DIR}/Platform ${CMAKE_CURRENT_SOURCE_DIR}/Platform/src/platform.cpp)
axf_add_module(Core ${CMAKE_CURRENT_SOURCE_DIR}/Core      ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/error.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/log.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/src/thread.cpp)
axf_add_module(Audio ${CMAKE_CURRENT_SOURCE_DIR}/Audio     ${CMAKE_CURRENT_SOURCE_DIR}/Audio/src/audio.cpp)
axf_add_module(Devices ${CMAKE_CURRENT_SOURCE_DIR}/Devices   ${CMAKE_CURRENT_SOURCE_DIR}/Devices/src/device.cpp)
axf_add_module(DSP ${CMAKE_CURRENT_SOURCE_DIR}/DSP       ${CMAKE_CURRENT_SOURCE_DIR}/DSP/src/dsp.cpp)
axf_add_module(Graph ${CMAKE_CURRENT_SOURCE_DIR}/Graph     ${CMAKE_CURRENT_SOURCE_DIR}/Graph/src/graph.cpp)
axf_add_module(Plugins ${CMAKE_CURRENT_SOURCE_DIR}/Plugins   ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/src/plugin_host.cpp)
axf_add_module(MIDI ${CMAKE_CURRENT_SOURCE_DIR}/MIDI      ${CMAKE_CURRENT_SOURCE_DIR}/MIDI/src/event.cpp)
axf_add_module(Media ${CMAKE_CURRENT_SOURCE_DIR}/Media     ${CMAKE_CURRENT_SOURCE_DIR}/Media/src/audio_file.cpp)
axf_add_module(Project ${CMAKE_CURRENT_SOURCE_DIR}/Project   ${CMAKE_CURRENT_SOURCE_DIR}/Project/src/project.cpp)
axf_add_module(GUI ${CMAKE_CURRENT_SOURCE_DIR}/GUI       ${CMAKE_CURRENT_SOURCE_DIR}/GUI/src/theme.cpp)

target_link_libraries(axf_core PUBLIC Threads::Threads)
target_link_libraries(axf_platform PUBLIC axf_core)
target_link_libraries(axf_audio PUBLIC axf_core)
target_link_libraries(axf_devices PUBLIC axf_core)
target_link_libraries(axf_dsp PUBLIC axf_core axf_audio)
target_link_libraries(axf_graph PUBLIC axf_core axf_audio axf_dsp)
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

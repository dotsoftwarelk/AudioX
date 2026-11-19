function(axf_add_test NAME)
  add_executable(${NAME} ${ARGN})
  target_link_libraries(${NAME} PRIVATE
    GTest::gtest_main AXF::Core AXF::Platform AXF::Audio AXF::DSP AXF::Graph
    AXF::Devices AXF::MIDI AXF::Plugins AXF::Media AXF::Project AXF::GUI)
  target_compile_options(${NAME} PRIVATE ${AXF_WARNING_FLAGS})
  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

axf_add_test(axf_core_tests Unit/core_tests.cpp)
axf_add_test(axf_audio_tests Unit/audio_tests.cpp Audio/buffer_tests.cpp)
axf_add_test(axf_dsp_tests DSP/dsp_tests.cpp)
axf_add_test(axf_midi_tests Unit/midi_tests.cpp)
axf_add_test(axf_graph_tests Integration/graph_tests.cpp Integration/device_config_tests.cpp)
axf_add_test(axf_platform_tests Platform/platform_tests.cpp Plugins/plugin_tests.cpp)

add_executable(axf_benchmarks Benchmarks/benchmarks.cpp)
target_link_libraries(axf_benchmarks PRIVATE AXF::Core AXF::Audio AXF::DSP AXF::Graph)
target_compile_options(axf_benchmarks PRIVATE ${AXF_WARNING_FLAGS})
add_test(NAME axf_benchmarks COMMAND axf_benchmarks)

# Hardware-dependent tests are registered but disabled by default. They must
# be run on a physical Windows machine with a real device.
add_test(NAME axf_hardware_placeholder
  COMMAND ${CMAKE_COMMAND} -E echo "Hardware tests require a physical Windows machine; skipped by default.")
set_tests_properties(axf_hardware_placeholder PROPERTIES DISABLED TRUE)

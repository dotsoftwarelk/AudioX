function(axf_add_test NAME)
  add_executable(${NAME} ${ARGN})
  target_link_libraries(${NAME} PRIVATE
    GTest::gtest_main AXF::Core AXF::Platform AXF::Audio AXF::DSP AXF::Graph
    AXF::Devices AXF::MIDI AXF::Plugins AXF::Media AXF::Project AXF::GUI)
  target_compile_options(${NAME} PRIVATE ${AXF_WARNING_FLAGS})
  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

axf_add_test(axf_core_tests
  Unit/core_tests.cpp
  Unit/event_tests.cpp
  Unit/job_system_tests.cpp
  Unit/config_tests.cpp
  Unit/time_tests.cpp
  Unit/memory_tests.cpp
  Unit/thread_tests.cpp
  Unit/log_tests.cpp)
axf_add_test(axf_audio_tests Unit/audio_tests.cpp Audio/buffer_tests.cpp)
axf_add_test(axf_dsp_tests DSP/dsp_tests.cpp)
axf_add_test(axf_midi_tests Unit/midi_tests.cpp)
axf_add_test(axf_graph_tests Integration/graph_tests.cpp Integration/device_config_tests.cpp)
axf_add_test(axf_platform_tests Platform/platform_tests.cpp Plugins/plugin_tests.cpp)

# Dedicated build of global operator new/delete instrumentation. It is linked
# only into the audit test and is never installed with the AXF SDK.
add_library(axf_rt_alloc_override STATIC
  ${CMAKE_SOURCE_DIR}/AXF/Core/src/rt_alloc_override.cpp)
target_include_directories(axf_rt_alloc_override PUBLIC
  ${CMAKE_SOURCE_DIR}/AXF/Core/include)
target_compile_features(axf_rt_alloc_override PUBLIC cxx_std_20)
target_link_libraries(axf_rt_alloc_override PUBLIC AXF::Core)
target_compile_options(axf_rt_alloc_override PRIVATE ${AXF_WARNING_FLAGS})

axf_add_test(axf_rt_audit_tests Unit/rt_audit_tests.cpp
  Integration/engine_audit_tests.cpp)
target_link_libraries(axf_rt_audit_tests PRIVATE axf_rt_alloc_override)

axf_add_test(axf_engine_tests Integration/engine_tests.cpp)

add_executable(axf_benchmarks Benchmarks/benchmarks.cpp)
target_link_libraries(axf_benchmarks PRIVATE AXF::Core AXF::Audio AXF::DSP AXF::Graph)
target_compile_options(axf_benchmarks PRIVATE ${AXF_WARNING_FLAGS})
add_test(NAME axf_benchmarks COMMAND axf_benchmarks)

# Hardware-dependent tests are registered but disabled by default. They must
# be run on a physical Windows machine with a real device.
add_test(NAME axf_hardware_placeholder
  COMMAND ${CMAKE_COMMAND} -E echo "Hardware tests require a physical Windows machine; skipped by default.")
set_tests_properties(axf_hardware_placeholder PROPERTIES DISABLED TRUE)

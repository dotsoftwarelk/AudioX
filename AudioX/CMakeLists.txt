# The DAW depends only on AXF public targets. It must never reach into
# AXF/*/src or include private headers.
add_executable(audiox Application/main.cpp)
target_link_libraries(audiox PRIVATE
  AXF::Core AXF::Platform AXF::Audio AXF::DSP AXF::Graph AXF::Devices
  AXF::MIDI AXF::Media AXF::Project AXF::GUI)
target_compile_options(audiox PRIVATE ${AXF_WARNING_FLAGS})
install(TARGETS audiox RUNTIME DESTINATION bin)

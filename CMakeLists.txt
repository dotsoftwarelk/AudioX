cmake_minimum_required(VERSION 3.25)
project(AudioX VERSION 0.1.0 DESCRIPTION "AudioX Framework and DAW" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(AXF_BUILD_TESTS "Build AXF test suite" ON)
option(AXF_BUILD_APP "Build the AudioX DAW shell" ON)
option(AXF_WERROR "Treat compiler warnings as errors" OFF)
option(AXF_ENABLE_VST3 "Enable VST3 host module (requires user-supplied SDK)" OFF)
option(AXF_ENABLE_ASIO "Enable ASIO backend (requires the Steinberg ASIO SDK)" OFF)
set(AXF_ASIO_SDK_ROOT "" CACHE PATH "Steinberg ASIO SDK root containing common/asio.h")

if(AXF_ENABLE_ASIO)
  if(NOT AXF_ASIO_SDK_ROOT AND DEFINED ENV{AXF_ASIO_SDK_ROOT})
    set(AXF_ASIO_SDK_ROOT "$ENV{AXF_ASIO_SDK_ROOT}")
  endif()
  if(WIN32)
    if(NOT EXISTS "${AXF_ASIO_SDK_ROOT}/common/asio.h")
      message(FATAL_ERROR
        "AXF_ENABLE_ASIO=ON requires AXF_ASIO_SDK_ROOT (or AXF_ASIO_SDK_ROOT env) "
        "pointing at the Steinberg ASIO SDK. The SDK is never vendored into this repo.")
    endif()
  else()
    message(STATUS "ASIO backend is Windows-only; disabling AXF_ENABLE_ASIO here")
    set(AXF_ENABLE_ASIO OFF CACHE BOOL "" FORCE)
  endif()
endif()

find_package(Threads REQUIRED)

if(MSVC)
  set(AXF_WARNING_FLAGS /W4 /permissive-)
  if(AXF_WERROR)
    list(APPEND AXF_WARNING_FLAGS /WX)
  endif()
else()
  set(AXF_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow)
  if(AXF_WERROR)
    list(APPEND AXF_WARNING_FLAGS -Werror)
  endif()
endif()

function(axf_add_module MODULE_NAME MODULE_DIR)
  string(TOLOWER "${MODULE_NAME}" MODULE_LOWER)
  string(TOUPPER "${MODULE_NAME}" MODULE_UPPER)
  add_library(axf_${MODULE_LOWER} STATIC ${ARGN})
  add_library(AXF::${MODULE_NAME} ALIAS axf_${MODULE_LOWER})
  target_compile_features(axf_${MODULE_LOWER} PUBLIC cxx_std_20)
  target_include_directories(axf_${MODULE_LOWER}
    PUBLIC
      $<BUILD_INTERFACE:${MODULE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE src)
  target_compile_definitions(axf_${MODULE_LOWER} PRIVATE AXF_${MODULE_UPPER}_EXPORTS)
  target_compile_options(axf_${MODULE_LOWER} PRIVATE ${AXF_WARNING_FLAGS})
  set_target_properties(axf_${MODULE_LOWER} PROPERTIES
    EXPORT_NAME ${MODULE_NAME}
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
  install(TARGETS axf_${MODULE_LOWER} EXPORT AXFTargets
    ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
  install(DIRECTORY ${MODULE_DIR}/include/ DESTINATION include)
endfunction()

add_subdirectory(AXF)

if(AXF_BUILD_APP)
  add_subdirectory(AudioX)
endif()

if(AXF_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  add_subdirectory(Tests)
endif()

message(STATUS "AudioX ${PROJECT_VERSION} | tests=${AXF_BUILD_TESTS} app=${AXF_BUILD_APP} werror=${AXF_WERROR}")

cmake_minimum_required(VERSION 3.20)
project(PresenceOFX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal macOS binary" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

add_library(PresenceOFX MODULE
  src/PresenceOFX.cpp
  src/PresenceCPU.cpp
)

target_include_directories(PresenceOFX PRIVATE src)

target_compile_options(PresenceOFX PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Werror=return-type>
)

if(APPLE)
  set_target_properties(PresenceOFX PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    BUNDLE TRUE
    BUNDLE_EXTENSION "ofx.bundle"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist.in
  )
else()
  set_target_properties(PresenceOFX PROPERTIES PREFIX "" SUFFIX ".ofx")
endif()

add_executable(presence_model_tests tests/presence_model_tests.cpp src/PresenceCPU.cpp)
target_include_directories(presence_model_tests PRIVATE src)

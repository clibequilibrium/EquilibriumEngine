cmake_minimum_required(VERSION 3.1)
project(equilibrium LANGUAGES C CXX)

set(CMAKE_COLOR_MAKEFILE OFF)
set(CMAKE_DEBUG_POSTFIX "")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE})
set(PROJECT_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2-config.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/assimp-config.cmake)

# Since we linkg BGFX statically we also want to export symbols so BGFX is no
# included twice
add_compile_definitions(PUBLIC BGFX_SHARED_LIB_BUILD=1)

if(CMAKE_HOST_UNIX AND NOT APPLE)
  set(LINUX TRUE)
elseif(CMAKE_HOST_WIN32)
  set(WINDOWS TRUE)
endif()

if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

if(WINDOWS)
  set(SHADERS_COMPILER "3rdparty/bgfx/bin/windows/shaderc.exe")
  set(TEXTURE_COMPILER "3rdparty/bgfx/bin/windows/texturec.exe")
  set(GEOMETRY_COMPILER "3rdparty/bgfx/bin/windows/geometryc.exe")
elseif(LINUX)
  set(SHADERS_COMPILER "3rdparty/bgfx/bin/linux/shaderc")
  set(TEXTURE_COMPILER "3rdparty/bgfx/bin/linux/texturec")
  set(GEOMETRY_COMPILER "3rdparty/bgfx/bin/windows/geometryc")
else()
  message(FATAL_ERROR "Current platform is not supported.")
endif()

add_subdirectory(equilibrium)
add_subdirectory(editor)
add_subdirectory(launcher)

add_subdirectory(3rdparty/bgfx)
add_subdirectory(3rdparty/cimgui)
add_subdirectory(3rdparty/cglm)
add_subdirectory(3rdparty/flecs)
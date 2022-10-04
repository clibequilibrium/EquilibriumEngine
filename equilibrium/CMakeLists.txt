cmake_minimum_required(VERSION 3.1)
project(equilibrium LANGUAGES C CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/shader_compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/geometry_compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/texture_compiler.cmake)

file(GLOB_RECURSE SRC *.c *.cpp)
file(GLOB_RECURSE HEADERS *.h)

set(EQUILIBRIUM_SOURCES ${SRC} ${HEADERS})

add_library(
  ${PROJECT_NAME} SHARED
  ${EQUILIBRIUM_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/cgltf/mikktspace.c)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/bgfx/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/cimgui/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/flecs/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/cglm/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/cr/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/cgltf/
         ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/x-watcher/
         ${SDL2_INCLUDE_DIRS}
         ${ASSIMP_INCLUDE_DIRS}
         ${CMAKE_CURRENT_SOURCE_DIR}/components
         ${CMAKE_CURRENT_SOURCE_DIR}/systems)

target_link_libraries(
  ${PROJECT_NAME}
  ${SDL2_LIBRARIES}
  bgfx
  bx
  cimgui
  cglm
  ${ASSIMP_LIBRARIES}
  flecs)

add_subdirectory(engine-simulation)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${PROJECT_WORKING_DIRECTORY}/3rdparty/SDL2/lib/x64/SDL2.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${PROJECT_WORKING_DIRECTORY}/3rdparty/assimp/lib/assimp.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>)
cmake_minimum_required(VERSION 3.0)

project(flecs)
file(GLOB IMGUI_SOURCES flecs.c)

add_library(flecs SHARED ${IMGUI_SOURCES})
target_include_directories(flecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
  target_link_libraries(flecs ws2_32)
endif()

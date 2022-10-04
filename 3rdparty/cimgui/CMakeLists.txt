cmake_minimum_required(VERSION 3.0)

project(cimgui)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp)
  set(TABLES_SOURCE "imgui/imgui_tables.cpp")
else()
  set(TABLES_SOURCE "")
endif()

# general settings
file(
  GLOB
  IMGUI_SOURCES
  cimgui.cpp
  ImGuizmo.cpp
  cimguizmo.cpp
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_impl_sdl.cpp
  ${TABLES_SOURCE})

set(IMGUI_STATIC
    "yes"
    CACHE STRING "Build as a static library")
set(IMGUI_FREETYPE
    "no"
    CACHE STRING "Build with freetype library")
set(IMGUI_LIBRARIES)

if(IMGUI_FREETYPE)
  find_package(freetype REQUIRED PATHS ${FREETYPE_PATH})
  list(APPEND IMGUI_LIBRARIES freetype)
  list(APPEND IMGUI_SOURCES imgui/misc/freetype/imgui_freetype.cpp)
  add_definitions("-DCIMGUI_FREETYPE=1")
endif(IMGUI_FREETYPE)

# add library and link
if(IMGUI_STATIC)
  add_library(cimgui STATIC ${IMGUI_SOURCES})
else(IMGUI_STATIC)
  add_library(cimgui SHARED ${IMGUI_SOURCES})
endif(IMGUI_STATIC)

target_compile_definitions(cimgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
if(WIN32)
  target_compile_definitions(
    cimgui PUBLIC IMGUI_IMPL_API=extern\t\"C\"\t__declspec\(dllexport\))
  list(APPEND IMGUI_LIBRARIES imm32)
else(WIN32)
  target_compile_definitions(cimgui PUBLIC IMGUI_IMPL_API=extern\t\"C\"\t)
endif(WIN32)

target_include_directories(
  cimgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS}
                ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

set_target_properties(cimgui PROPERTIES PREFIX "")
target_link_libraries(cimgui ${IMGUI_LIBRARIES} ${SDL2_LIBRARIES})

# install
install(
  TARGETS cimgui
  RUNTIME DESTINATION .
  LIBRARY DESTINATION .
  ARCHIVE DESTINATION .)

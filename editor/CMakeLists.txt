cmake_minimum_required(VERSION 3.1)
project(editor LANGUAGES C CXX)

file(GLOB_RECURSE SRC *.c *.cpp)
file(GLOB_RECURSE HEADERS *.h)

set(EQUILIBRIUM_SOURCES ${SRC} ${HEADERS})
add_library(${PROJECT_NAME} SHARED ${EQUILIBRIUM_SOURCES})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/components
         ${CMAKE_CURRENT_SOURCE_DIR}/systems)

target_link_libraries(${PROJECT_NAME} equilibrium)

cmake_minimum_required(VERSION 3.1)
project(engine-simulation LANGUAGES C CXX)

file(GLOB_RECURSE SRC *.c *.cpp)
file(GLOB_RECURSE HEADERS *.h)

set(EQUILIBRIUM_SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} SHARED ${EQUILIBRIUM_SOURCES})
target_link_libraries(${PROJECT_NAME} equilibrium)

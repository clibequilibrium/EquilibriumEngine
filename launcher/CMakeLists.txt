cmake_minimum_required(VERSION 3.1)
project(launcher LANGUAGES C CXX)

add_executable(${PROJECT_NAME} launcher.cc)

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/cr/)

target_link_libraries(${PROJECT_NAME} dbghelp equilibrium)
add_subdirectory(sandbox)

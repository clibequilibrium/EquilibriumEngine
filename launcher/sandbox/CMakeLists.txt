project(sandbox LANGUAGES C CXX)

file(GLOB_RECURSE SRC *.c *.cpp)
file(GLOB_RECURSE HEADERS *.h)

set(PROJ_SRC ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} SHARED ${PROJ_SRC})
target_link_libraries(${PROJECT_NAME} equilibrium)

file(GLOB_RECURSE SHADERS_SRC shaders/*.sc)
file(GLOB_RECURSE MODELS_SRC models/*.gltf)
file(GLOB_RECURSE TEXTURES_SRC textures/*.tga textures/*.png textures/*.jpeg)

compile_shaders("${PROJECT_NAME}" "${SHADERS_SRC}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")

# compile_texture("${PROJECT_NAME}" "${TEXTURES_SRC}"
#                 "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/textures")

# compile_geometry("${PROJECT_NAME}" "${MODELS_SRC}"
#                  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models")

# Copy sample models to the build directory
file(COPY models DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
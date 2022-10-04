function(compile_texture TARGET MODELS MODEL_OUTPUT_DIR)

  foreach(MODEL_PATH ${MODELS})

    get_filename_component(TEXTURE_NAME "${MODEL_PATH}" NAME_WE)
    get_filename_component(GEOMETRY_FILE "${MODEL_PATH}" ABSOLUTE)

    set(TEXTURE_OUTPUT_PATH ${MODEL_OUTPUT_DIR}/${TEXTURE_NAME})
    file(MAKE_DIRECTORY ${MODEL_OUTPUT_DIR})

    if(EXISTS "${TEXTURE_OUTPUT_PATH}")
      continue()
    endif()

    if(TEXTURE_NAME MATCHES "normal")
      set(NORMAL_ARG "--normalmap")
    endif()

    message("Compiling texture: ${TEXTURE_NAME}")
    execute_process(
      COMMAND "${TEXTURE_COMPILER}" "-f" "${MODEL_PATH}" "-o"
              "${TEXTURE_OUTPUT_PATH}.dds" "-t" "BC3" " ${NORMAL_ARG}"
      WORKING_DIRECTORY "${PROJECT_WORKING_DIRECTORY}")

    # Make sure our build depends on this output.
    set_source_files_properties("${TEXTURE_OUTPUT_PATH}" PROPERTIES GENERATED
                                                                    TRUE)
    target_sources(${TARGET} PUBLIC "${TEXTURE_OUTPUT_PATH}")
  endforeach()

endfunction(compile_texture)

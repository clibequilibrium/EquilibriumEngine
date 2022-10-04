function(compile_geometry TARGET MODELS MODEL_OUTPUT_DIR)

  foreach(MODEL_PATH ${MODELS})

    get_filename_component(GEOMETRY_NAME "${MODEL_PATH}" NAME_WE)
    get_filename_component(GEOMETRY_FILE "${MODEL_PATH}" ABSOLUTE)

    set(GEOMETRY_OUTPUT_PATH ${MODEL_OUTPUT_DIR}/${GEOMETRY_NAME}.bin)
    file(MAKE_DIRECTORY ${MODEL_OUTPUT_DIR})

    if(EXISTS "${GEOMETRY_OUTPUT_PATH}")
      continue()
    endif()

    message("Compiling geometry: ${GEOMETRY_NAME}")
    execute_process(
      COMMAND "${GEOMETRY_COMPILER}" "-f" "${MODEL_PATH}" "-o"
              "${GEOMETRY_OUTPUT_PATH}" "--tangent" "--ccw"
      WORKING_DIRECTORY "${PROJECT_WORKING_DIRECTORY}")
  
    # Make sure our build depends on this output.
    set_source_files_properties("${GEOMETRY_OUTPUT_PATH}" PROPERTIES GENERATED
                                                                     TRUE)
    target_sources(${TARGET} PUBLIC "${GEOMETRY_OUTPUT_PATH}")
  endforeach()

endfunction(compile_geometry)

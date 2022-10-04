function(compile_shaders TARGET SHADERS SHADER_OUTPUT_DIR)

  if(WINDOWS)
    set(SHADER_OUTPUT_DIR ${SHADER_OUTPUT_DIR}/dx11)
  endif()

  if(LINUX)
    set(SHADER_OUTPUT_DIR ${SHADER_OUTPUT_DIR}/spirv)
  endif()

  foreach(SHADER_PATH ${SHADERS})

    get_filename_component(SHADER_NAME "${SHADER_PATH}" NAME_WE)
    get_filename_component(SHADER_FILE "${SHADER_PATH}" ABSOLUTE)

    if(SHADER_NAME MATCHES "^varying")
      continue()
    endif()

    set(CURRENT_OUTPUT_PATH ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.bin)
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

    set(GLSL_VERSION 430)
    set(GLSL_COMPUTE_VERSION 430)

    # DX9/11 shaders can only be compiled on Windows
    set(SHADER_PLATFORMS glsl spirv)

    if(WINDOWS)
      set(DX_MODEL 5_0)
      set(SHADER_PLATFORMS ${SHADER_PLATFORMS} dx11)
    endif()

    if(SHADER_NAME MATCHES "^vs_")
      set(SHADER_TYPE VERTEX)
      set(SHADER_PROFILE "vs_${DX_MODEL}")
    elseif(SHADER_NAME MATCHES "^fs_")
      set(SHADER_TYPE FRAGMENT)
      set(SHADER_PROFILE "ps_${DX_MODEL}")
    elseif(SHADER_NAME MATCHES "^cs_")
      set(SHADER_TYPE COMPUTE)
      set(SHADER_PROFILE "cs_${DX_MODEL}")
    endif()

    if(LINUX)
      set(SHADER_PROFILE spirv)
    endif()

    add_custom_command(
      OUTPUT ${CURRENT_OUTPUT_PATH}
      COMMAND
        ${SHADERS_COMPILER} -i
        "${PROJECT_WORKING_DIRECTORY}/3rdparty/bgfx/bgfx/src/" -i
        "${PROJECT_WORKING_DIRECTORY}/3rdparty/bgfx/bgfx/examples/common/"
        --type ${SHADER_TYPE} --platform ${SHADER_PLATFORMS} -f ${SHADER_PATH}
        -o "${CURRENT_OUTPUT_PATH}" -p "${SHADER_PROFILE}" --verbose
      DEPENDS ${SHADER_PATH}
      IMPLICIT_DEPENDS C ${SHADER_PATH}
      VERBATIM
      COMMENT "Compiling shader: ${SHADER_NAME}"
      WORKING_DIRECTORY ${PROJECT_WORKING_DIRECTORY})

    # Make sure our build depends on this output.
    set_source_files_properties(${CURRENT_OUTPUT_PATH} PROPERTIES GENERATED
                                                                  TRUE)
    target_sources(${TARGET} PRIVATE ${CURRENT_OUTPUT_PATH})
  endforeach()

endfunction(compile_shaders)

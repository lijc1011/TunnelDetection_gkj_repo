# Keep the prebuilt DLL tied to its source, import library and build configuration.
function(tszarr_artifact_command output mode dll implib config)
    set(${output}
        "${CMAKE_COMMAND}"
        "-DTSZARR_MODE=${mode}"
        "-DTSZARR_DLL=${dll}"
        "-DTSZARR_IMPLIB=${implib}"
        "-DTSZARR_SOURCE_DIR=${PROJECT_SOURCE_DIR}/src/tszarr"
        "-DTSZARR_CONFIG=${config}"
        "-DTSZARR_COMPILER=${CMAKE_CXX_COMPILER_ID}"
        "-DTSZARR_MSVC_VERSION=${MSVC_VERSION}"
        "-DTSZARR_TOOLSET=${CMAKE_VS_PLATFORM_TOOLSET}"
        "-DTSZARR_POINTER_SIZE=${CMAKE_SIZEOF_VOID_P}"
        -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CheckTsZarrBridge.cmake"
        PARENT_SCOPE)
endfunction()

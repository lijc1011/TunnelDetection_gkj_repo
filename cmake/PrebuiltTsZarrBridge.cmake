# Daily application builds keep the storage implementation outside the build tree.
# The original source build remains available through the local preset.
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Prebuilt TsZarrBridge requires the matching Windows x64 MSVC build.")
endif()

set(_tszarr_configs ${CMAKE_CONFIGURATION_TYPES})
if(NOT _tszarr_configs)
    if(NOT CMAKE_BUILD_TYPE)
        message(FATAL_ERROR "Set CMAKE_BUILD_TYPE to the prebuilt TsZarrBridge configuration.")
    endif()
    set(_tszarr_configs "${CMAKE_BUILD_TYPE}")
endif()

add_library(TsZarrBridge SHARED IMPORTED GLOBAL)
set_target_properties(TsZarrBridge PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src"
    IMPORTED_CONFIGURATIONS "${_tszarr_configs}"
)

set(_tszarr_available_configs "")
set(_tszarr_check_errors "")
foreach(_tszarr_config IN LISTS _tszarr_configs)
    string(TOUPPER "${_tszarr_config}" _tszarr_config_upper)
    set(_tszarr_dll "${TUNNEL_TSBRIDGE_PREBUILT_DIR}/${_tszarr_config}/TsZarrBridge.dll")
    set(_tszarr_lib "${TUNNEL_TSBRIDGE_PREBUILT_DIR}/${_tszarr_config}/TsZarrBridge.lib")
    # Define every configuration explicitly: never fall back from Debug to Release.
    set_target_properties(TsZarrBridge PROPERTIES
        "IMPORTED_LOCATION_${_tszarr_config_upper}" "${_tszarr_dll}"
        "IMPORTED_IMPLIB_${_tszarr_config_upper}" "${_tszarr_lib}"
    )
    if(EXISTS "${_tszarr_dll}" AND EXISTS "${_tszarr_lib}")
        tszarr_artifact_command(_tszarr_check_command CHECK
            "${_tszarr_dll}" "${_tszarr_lib}" "${_tszarr_config}")
        execute_process(COMMAND ${_tszarr_check_command}
            RESULT_VARIABLE _tszarr_check_result
            OUTPUT_VARIABLE _tszarr_check_output
            ERROR_VARIABLE _tszarr_check_error)
        if(_tszarr_check_result EQUAL 0)
            message(STATUS "Verified prebuilt TsZarrBridge (${_tszarr_config})")
            list(APPEND _tszarr_available_configs "${_tszarr_config}")
        else()
            # Rebuilding Debug must not require rebuilding an unused Release DLL.
            # The selected configuration is checked again before every build.
            message(STATUS "Prebuilt TsZarrBridge (${_tszarr_config}) needs a FULL rebuild")
            string(APPEND _tszarr_check_errors "\n${_tszarr_check_error}")
        endif()
    endif()
endforeach()

if(NOT _tszarr_available_configs)
    message(FATAL_ERROR
        "No verified prebuilt TsZarrBridge DLL/LIB found in ${TUNNEL_TSBRIDGE_PREBUILT_DIR}. "
        "Run tools/cmake.ps1 --preset local, then "
        "tools/cmake.ps1 --build --preset local-debug (or local-release) first."
        "${_tszarr_check_errors}")
endif()
tszarr_artifact_command(_tszarr_check_command CHECK
    "$<TARGET_FILE:TsZarrBridge>" "$<TARGET_LINKER_FILE:TsZarrBridge>" "$<CONFIG>")
add_custom_target(CheckTsZarrBridge
    COMMAND ${_tszarr_check_command}
    COMMENT "Checking prebuilt TsZarrBridge source and configuration"
    VERBATIM
)
message(STATUS "Using prebuilt TsZarrBridge (${_tszarr_available_configs}); TensorStore configuration skipped")

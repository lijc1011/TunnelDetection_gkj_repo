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
        list(APPEND _tszarr_available_configs "${_tszarr_config}")
    endif()
endforeach()

if(NOT _tszarr_available_configs)
    message(FATAL_ERROR
        "No matching prebuilt TsZarrBridge DLL/LIB found in ${TUNNEL_TSBRIDGE_PREBUILT_DIR}. "
        "Run tools/cmake.ps1 --preset local, then "
        "tools/cmake.ps1 --build --preset local-debug (or local-release) first.")
endif()
message(STATUS "Using prebuilt TsZarrBridge (${_tszarr_available_configs}); TensorStore configuration skipped")

cmake_minimum_required(VERSION 3.16)

project(TunnelDetection LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/Qt6.11.1/6.11.1/msvc2022_64")

set(OpenCV_DIR "C:/dev/opencv-4.8.0/opencv/build/x64/vc16/lib" CACHE PATH "Path to OpenCVConfig.cmake")
set(Z5_INCLUDE_DIR "C:/zarr/z5-master/include" CACHE PATH "Path to z5 include directory")

find_package(Qt6 COMPONENTS Widgets SerialPort REQUIRED)
find_package(OpenCV 4.8.0 COMPONENTS world REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(zstd REQUIRED)
find_package(lz4 REQUIRED)
find_package(xtensor REQUIRED)
find_package(nlohmann_json REQUIRED)

set(TINYXML_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml" CACHE PATH "Path to tinyxml include")
add_library(tinyxml STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml/tinyxml.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml/tinyxmlparser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml/tinyxmlerror.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml/tinystr.cpp"
)
target_include_directories(tinyxml PUBLIC "${TINYXML_INCLUDE_DIR}")

set(ADVAN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Advan" CACHE PATH "Path to Advantech include")
add_library(advan INTERFACE)
target_include_directories(advan INTERFACE "${ADVAN_INCLUDE_DIR}")

set(DALSA_DIR "C:/Program Files/Teledyne DALSA/Sapera" CACHE PATH "Path to Teledyne DALSA Sapera SDK")
set(DALSA_LIB_PATH "${DALSA_DIR}/Lib/Win64/SapClassBasic.lib")
if(NOT EXISTS "${DALSA_LIB_PATH}")
    set(DALSA_LIB_PATH "${DALSA_DIR}/Lib/Win64/VS2015/SapClassBasic.lib")
endif()
if(NOT EXISTS "${DALSA_LIB_PATH}")
    message(FATAL_ERROR "Sapera library not found: ${DALSA_LIB_PATH}")
endif()

add_library(Dalsa::Sapera STATIC IMPORTED GLOBAL)
target_include_directories(Dalsa::Sapera INTERFACE
    "${DALSA_DIR}/Classes/Basic"
    "${DALSA_DIR}/Classes/Gui"
    "${DALSA_DIR}/Include"
)
set_target_properties(Dalsa::Sapera PROPERTIES IMPORTED_LOCATION "${DALSA_LIB_PATH}")

if(NOT EXISTS "${Z5_INCLUDE_DIR}/z5/z5.hxx")
    message(FATAL_ERROR "z5 include directory not found or invalid: ${Z5_INCLUDE_DIR}")
endif()

add_library(z5 INTERFACE)
target_include_directories(z5 INTERFACE "${Z5_INCLUDE_DIR}")
target_link_libraries(z5 INTERFACE
    xtensor
    nlohmann_json::nlohmann_json
    ZLIB::ZLIB
    BZip2::BZip2
    zstd::libzstd
    LZ4::lz4
)
target_compile_definitions(z5 INTERFACE
    WITH_ZLIB
    WITH_BZIP2
    WITH_ZSTD
    WITH_LZ4
)

aux_source_directory(./src PROJECT_SOURCES)

add_executable(${PROJECT_NAME}
    WIN32
    ${PROJECT_SOURCES}
    resources.qrc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /FS)
    target_compile_options(tinyxml PRIVATE /utf-8 /FS)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        Qt6::SerialPort
        ${OpenCV_LIBS}
        z5
        Dalsa::Sapera
        advan
        tinyxml
)

if(WIN32)
    if(TARGET opencv_world)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:opencv_world>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying OpenCV runtime"
        )
    endif()

    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "C:/Qt/Qt6.11.1/6.11.1/msvc2022_64/bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --compiler-runtime --force "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime"
        )
    endif()
endif()

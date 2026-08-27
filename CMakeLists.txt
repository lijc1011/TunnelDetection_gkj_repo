cmake_minimum_required(VERSION 3.24)

project(TunnelDetection LANGUAGES CXX)

set(TUNNEL_BUILD_ID "0.2.0-fieldtrial-20260826" CACHE STRING "Build identifier written to Zarr manifests")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# TensorStore 首次全量构建源文件极多，让 MSVC 并行编译同一工程内的翻译单元。
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/MP>)
endif()

# 旧 z5 (Zarr v2) 后端：默认关闭但保留，需要时 -DTUNNEL_USE_Z5=ON 即可回退。
option(TUNNEL_USE_Z5 "Enable the deprecated z5 (Zarr v2) backend" OFF)

# Machine-specific dependency locations belong in CMakeUserPresets.json.
set(Z5_INCLUDE_DIR "" CACHE PATH "Path to z5 include directory")

find_package(Qt6 COMPONENTS Widgets SerialPort REQUIRED)
find_package(OpenCV 4.8.0 COMPONENTS world REQUIRED)

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

set(DALSA_DIR "" CACHE PATH "Path to Teledyne DALSA Sapera SDK")
if(NOT DALSA_DIR)
    message(FATAL_ERROR "DALSA_DIR is required. Set it in CMakeUserPresets.json.")
endif()
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

# 海康 MVS SDK（网口/USB 工业相机）
set(HIK_MVS_DIR "" CACHE PATH "Path to HIKROBOT MVS SDK")
if(NOT HIK_MVS_DIR)
    message(FATAL_ERROR "HIK_MVS_DIR is required. Set it in CMakeUserPresets.json.")
endif()
set(HIK_MVS_LIB_PATH "${HIK_MVS_DIR}/Libraries/win64/MvCameraControl.lib")
if(NOT EXISTS "${HIK_MVS_LIB_PATH}")
    message(FATAL_ERROR "HIK MVS library not found: ${HIK_MVS_LIB_PATH}")
endif()

add_library(Hik::MvCameraControl STATIC IMPORTED GLOBAL)
target_include_directories(Hik::MvCameraControl INTERFACE
    "${HIK_MVS_DIR}/Includes"
)
set_target_properties(Hik::MvCameraControl PROPERTIES IMPORTED_LOCATION "${HIK_MVS_LIB_PATH}")

if(TUNNEL_USE_Z5)
    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(zstd REQUIRED)
    find_package(lz4 REQUIRED)
    find_package(xtensor REQUIRED)
    find_package(nlohmann_json REQUIRED)

    if(NOT EXISTS "${Z5_INCLUDE_DIR}/z5/z5.hxx")
        message(FATAL_ERROR "Z5_INCLUDE_DIR is missing or invalid: ${Z5_INCLUDE_DIR}")
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
endif()

aux_source_directory(./src PROJECT_SOURCES)

add_executable(${PROJECT_NAME}
    WIN32
    ${PROJECT_SOURCES}
    resources.qrc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    TUNNEL_BUILD_ID="${TUNNEL_BUILD_ID}"
)

# 无相机硬件也可运行的自动亮度算法回归测试。
add_executable(AutoExposureControllerSelfTest
    tools/AutoExposureControllerSelfTest.cpp
    src/AutoExposureController.cpp
)
target_include_directories(AutoExposureControllerSelfTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(MSVC)
    target_compile_options(AutoExposureControllerSelfTest PRIVATE /utf-8)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /FS)
    target_compile_options(tinyxml PRIVATE /utf-8 /FS)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        Qt6::SerialPort
        ${OpenCV_LIBS}
        Dalsa::Sapera
        Hik::MvCameraControl
        advan
        tinyxml
)

if(MSVC)
    # Keep Hikrobot MVS optional at runtime. DALSA-only deployments should not
    # fail process startup just because MVCameraControl.dll is not installed.
    target_link_options(${PROJECT_NAME} PRIVATE "/DELAYLOAD:MVCameraControl.dll")
    target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
endif()

if(TUNNEL_USE_Z5)
    target_link_libraries(${PROJECT_NAME} PRIVATE z5)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TUNNEL_Z5_ENABLED)
endif()

# ===== TensorStore (Zarr v3) 采集写盘后端 =====
# vendored 源码经 FetchContent 并入构建树；首次 configure 需要联网下载依赖源码
# （abseil/riegeli 等）且需要 Python >= 3.10（bazel_to_cmake 在配置期运行）。
set(TUNNEL_TENSORSTORE_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/TensorStore"
    CACHE PATH "Vendored TensorStore source directory")
if(NOT EXISTS "${TUNNEL_TENSORSTORE_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "TensorStore source not found: ${TUNNEL_TENSORSTORE_SOURCE_DIR}")
endif()

# 主工程的 Qt 自动生成只属于主 target（上面已创建）；关掉目录级开关，
# 避免给 TensorStore 的数百个第三方 target 挂上 moc/uic/rcc。
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

include(FetchContent)
FetchContent_Declare(tensorstore SOURCE_DIR "${TUNNEL_TENSORSTORE_SOURCE_DIR}")
FetchContent_MakeAvailable(tensorstore)

# 桥接 DLL：C ABI 窄接口，主程序只依赖它，不直接编译/链接 TensorStore。
add_library(TsZarrBridge SHARED
    src/tszarr/TsZarrBridge.cpp
    src/tszarr/TsZarrBridge.h
)
target_compile_definitions(TsZarrBridge PRIVATE TSZARR_BRIDGE_EXPORTS)
target_include_directories(TsZarrBridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(MSVC)
    # /EHsc 必须显式给出（vcpkg 工具链的默认 flags 不含它）：桥内 try/catch 依赖栈展开语义
    target_compile_options(TsZarrBridge PRIVATE /utf-8 /FS /EHsc)
endif()
target_link_libraries(TsZarrBridge PRIVATE
    # 只链 zarr3 + 本地文件 + bytes codec 的最小闭包；
    # 网络/云存储/图像编解码等驱动全部不进编译。
    tensorstore::tensorstore
    tensorstore::open
    tensorstore::resize_options
    tensorstore::context
    tensorstore::array
    tensorstore::index_space_dim_expression
    tensorstore::driver_zarr3_driver
    tensorstore::driver_zarr3_codec_bytes
    tensorstore::kvstore_file
)
foreach(optional_dependency IN ITEMS
        absl::status
        nlohmann_json::nlohmann_json
        nlohmann_json)
    if(TARGET ${optional_dependency})
        target_link_libraries(TsZarrBridge PRIVATE ${optional_dependency})
    endif()
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE TsZarrBridge)

# 写盘基准（控制台程序）：现方案 fwrite vs zarr 异步写，含读回校验。
add_executable(TsZarrBench src/tszarr/TsZarrBench.cpp)
if(MSVC)
    target_compile_options(TsZarrBench PRIVATE /utf-8 /FS /EHsc)
endif()
target_link_libraries(TsZarrBench PRIVATE TsZarrBridge)

add_executable(ZarrRulesSelfTest
    src/tszarr/ZarrRulesSelfTest.cpp
    src/ZarrSessionRules.cpp
    src/ZarrSessionRules.h
)
if(MSVC)
    target_compile_options(ZarrRulesSelfTest PRIVATE /utf-8 /FS /EHsc)
endif()
target_link_libraries(ZarrRulesSelfTest PRIVATE Qt6::Core)

if(WIN32)
    add_custom_command(TARGET ZarrRulesSelfTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ZarrRulesSelfTest>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config/tunzarr_rules.json"
                "$<TARGET_FILE_DIR:ZarrRulesSelfTest>/config/tunzarr_rules.json"
    )
endif()

if(WIN32)
    if(TARGET opencv_world)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:opencv_world>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying OpenCV runtime"
        )
    endif()

    set(QT_BIN_DIR "")
    if(TARGET Qt6::qmake)
        get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
    endif()
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${QT_BIN_DIR}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --compiler-runtime --force "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime"
        )
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config/tunzarr_rules.json"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config/tunzarr_rules.json"
        COMMENT "Copying TunZarr field rule configuration"
    )
endif()

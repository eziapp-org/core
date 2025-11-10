cmake_minimum_required(VERSION 3.31)

set(VCPKG_TARGET_TRIPLET "x64-windows-static")

project(Ezi)

execute_process(
    COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DGIT_HASH=\"${GIT_HASH}\")
add_definitions(-DEZI_VERSION=\"0.0.0\")


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/inc ${CMAKE_SOURCE_DIR}/extensions/inc)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/extensions/src/*.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(unofficial-webview2 CONFIG REQUIRED)
    add_executable(${PROJECT_NAME} WIN32 ${CPP_SOURCES} ezi.win.rc)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::webview2::webview2 nlohmann_json::nlohmann_json dwmapi zstd::libzstd)
    target_link_options(${PROJECT_NAME} PRIVATE
        "/MANIFEST:EMBED"
        "/MANIFESTINPUT:${CMAKE_SOURCE_DIR}/ezi.win.manifest"
    )
else()
    add_executable(${PROJECT_NAME} ${CPP_SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
endif()

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /source-charset:utf-8 /execution-charset:utf-8)
endif()
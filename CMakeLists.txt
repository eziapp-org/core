cmake_minimum_required(VERSION 3.31)

project(Ezi)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IS_WINDOWS FALSE)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(IS_WINDOWS TRUE)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/extensions/inc
)

file(
    GLOB_RECURSE
    CPP_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/extensions/src/*.cpp
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

if(IS_WINDOWS)
    find_package(unofficial-webview2 CONFIG REQUIRED)
endif()

if(MSVC)
    add_compile_options(/source-charset:utf-8 /execution-charset:utf-8)
endif()

execute_process(
    COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DGIT_HASH=\"${GIT_HASH}\")
add_definitions(-DEZI_VERSION=\"0.0.0\")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/dist")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/dist")

if(IS_WINDOWS)
    add_link_options(
        "/MANIFEST:NO"
        "/MANIFEST:EMBED"
        "/MANIFESTINPUT:${CMAKE_SOURCE_DIR}/ezi.win.manifest"
    )

    set(WIN_LIBS unofficial::webview2::webview2 nlohmann_json::nlohmann_json dwmapi zstd::libzstd)

    # local test build
    set(LOCAL_TARGET eziapp-local-winx64)
    add_executable(${LOCAL_TARGET} WIN32 ${CPP_SOURCES} ezi.win.rc)
    set_target_properties(${LOCAL_TARGET} PROPERTIES
        OUTPUT_NAME "$<IF:$<CONFIG:Debug>,${LOCAL_TARGET}-debug,${LOCAL_TARGET}-release>"
    )
    target_link_libraries(${LOCAL_TARGET} PRIVATE ${WIN_LIBS})

    # npm build release
    set(NPM_TARGET eziapp-npm-release-winx64)
    add_executable(${NPM_TARGET} WIN32 ${CPP_SOURCES})
    target_link_libraries(${NPM_TARGET} PRIVATE ${WIN_LIBS})

    # npm build debug
    set(NPM_DEBUG_TARGET eziapp-npm-dev-winx64)
    add_executable(${NPM_DEBUG_TARGET} WIN32 ${CPP_SOURCES})
    target_compile_definitions(${NPM_DEBUG_TARGET} PRIVATE NPM_DEV=1)
    target_link_libraries(${NPM_DEBUG_TARGET} PRIVATE ${WIN_LIBS})
endif()

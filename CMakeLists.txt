cmake_minimum_required(VERSION 3.20)
project(Prism VERSION 0.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PRISM_BUILD_TESTS   "Build unit tests"         ON)
option(PRISM_BUILD_APPS    "Build sample applications" ON)
option(PRISM_HEADLESS      "Headless build (no SDL2)"  OFF)
option(PRISM_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(PRISM_ENABLE_ASAN   "Enable AddressSanitizer"  OFF)
option(PRISM_ENABLE_UBSAN  "Enable UndefinedBehaviorSanitizer" OFF)
option(PRISM_ENABLE_TSAN   "Enable ThreadSanitizer"   OFF)

# ── Compiler flags helper ─────────────────────────────────────────────────────
function(prism_apply_warnings TARGET_NAME)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /permissive-)
        if(PRISM_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
        if(PRISM_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        endif()
    endif()

    if(PRISM_ENABLE_ASAN)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    endif()
    if(PRISM_ENABLE_UBSAN)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=undefined)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=undefined)
    endif()
    if(PRISM_ENABLE_TSAN)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=thread)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=thread)
    endif()
endfunction()

# ── Math (header-only interface library) ──────────────────────────────────────
add_library(prism_math INTERFACE)
target_include_directories(prism_math INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(prism_math INTERFACE cxx_std_17)

# ── Render library ────────────────────────────────────────────────────────────
add_library(prism_render STATIC
    src/render/font.cpp
    src/render/framebuffer.cpp
    src/render/rasterizer.cpp
    src/render/loader.cpp
    src/render/raster_opt.cpp
    src/render/tile_rasterizer.cpp
)
target_include_directories(prism_render PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(prism_render PUBLIC prism_math)
prism_apply_warnings(prism_render)

if(PRISM_HEADLESS)
    target_compile_definitions(prism_render PUBLIC PRISM_HEADLESS)
endif()

# ── Core library (window wrapper) ─────────────────────────────────────────────
add_library(prism_core STATIC
    src/core/dummy.cpp
    src/core/window.cpp
)
target_include_directories(prism_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(prism_core PUBLIC prism_render)
prism_apply_warnings(prism_core)

if(NOT PRISM_HEADLESS)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        target_link_libraries(prism_core PUBLIC SDL2::SDL2)
    else()
        include(FetchContent)
        FetchContent_Declare(
            SDL2
            URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.0/SDL2-2.30.0.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON  CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
        target_link_libraries(prism_core PUBLIC SDL2::SDL2-static)
    endif()
endif()

if(PRISM_HEADLESS)
    target_compile_definitions(prism_core PUBLIC PRISM_HEADLESS)
endif()

# ── GoogleTest ────────────────────────────────────────────────────────────────
if(PRISM_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()

# ── Applications ─────────────────────────────────────────────────────────────
if(PRISM_BUILD_APPS)
    add_subdirectory(apps)
endif()

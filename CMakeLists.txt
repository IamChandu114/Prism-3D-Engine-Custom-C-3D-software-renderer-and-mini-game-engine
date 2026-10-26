cmake_minimum_required(VERSION 3.20)
project(Prism VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PRISM_BUILD_TESTS "Build unit tests" ON)
option(PRISM_BUILD_APPS "Build sample applications and benchmarks" ON)
option(PRISM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(PRISM_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(PRISM_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Common compiler warnings
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
endfunction()

# Include directories
include_directories(include)

# Math library (interface / header-only)
add_library(prism_math INTERFACE)
target_include_directories(prism_math INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(prism_math INTERFACE cxx_std_17)

# Core library (for future engine components)
add_library(prism_core STATIC)
target_sources(prism_core PRIVATE
    src/core/dummy.cpp
)
target_include_directories(prism_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(prism_core PUBLIC prism_math)
prism_apply_warnings(prism_core)

# Fetch GoogleTest
if(PRISM_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # Prevent installing GoogleTest
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()

# Applications will be enabled in later stages
if(PRISM_BUILD_APPS)
    # add_subdirectory(apps)
endif()

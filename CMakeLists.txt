cmake_minimum_required(VERSION 3.31.6)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_compile_options(-fopenmp -Xclang -fno-pch-timestamp)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Azalea)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE MODULES CONFIGURE_DEPENDS
    "src/*.cppm"
    "include/*.cppm"
)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_sources(${PROJECT_NAME}
    PRIVATE
    FILE_SET CXX_MODULES TYPE CXX_MODULES
    FILES ${MODULES}
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

setup_geode_mod(${PROJECT_NAME})

find_package(Boost REQUIRED)
target_link_libraries(
    ${PROJECT_NAME}
    Boost::boost
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
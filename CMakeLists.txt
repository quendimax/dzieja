cmake_minimum_required(VERSION 3.13.4)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
    message(FATAL_ERROR "In-source builds are not allowed. "
            "Please create a directory and run cmake "
            "from there, passing the path to this source directory as the last argument. "
            "This process created the file `CMakeCache.txt' and the directory "
            "`CMakeFiles'. Please delete them.")
endif()

set(CMAKE_MODULE_PATH
    "${CMAKE_MODULE_PATH}"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
)

set(DZIEJA_VERSION_MAJOR 0)
set(DZIEJA_VERSION_MINOR 0)
set(DZIEJA_VERSION_PATCH 0)
set(DZIEJA_VERSION "${DZIEJA_VERSION_MAJOR}.${DZIEJA_VERSION_MINOR}.${DZIEJA_VERSION_PATCH}")
message(STATUS "Dzieja version: ${DZIEJA_VERSION}")

set(DZIEJA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DZIEJA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(BEFORE
    "${DZIEJA_BINARY_DIR}/include"
    "${DZIEJA_SOURCE_DIR}/include")

include(AddDzieja)

add_subdirectory(lib)
add_subdirectory(utils)

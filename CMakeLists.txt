cmake_minimum_required(VERSION 3.4.3)

# Make sure that our source directory is on the current cmake module path so that
# we can include cmake files from this directory.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(BACKEND_PACKAGE_STRING "${PACKAGE_STRING}")  # PACKAGE_STRING is set in LLVM CMakeLists.txt file

set(DZIEJA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DZIEJA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
    message(FATAL_ERROR "In-source builds are not allowed. "
            "Please create a directory and run cmake "
            "from there, passing the path to this source directory as the last argument. "
            "This process created the file `CMakeCache.txt' and the directory "
            "`CMakeFiles'. Please delete them.")
endif()

set(DZIEJA_VERSION_MAJOR 0)
set(DZIEJA_VERSION_MINOR 0)
set(DZIEJA_VERSION_PATCH 0)
set(DZIEJA_VERSION "${DZIEJA_VERSION_MAJOR}.${DZIEJA_VERSION_MINOR}.${DZIEJA_VERSION_PATCH}")
message(STATUS "Dzieja version: ${DZIEJA_VERSION}")

# Configure the Version.inc file.
configure_file(
        ${DZIEJA_SOURCE_DIR}/include/dzieja/Basic/Version.inc.in
        ${DZIEJA_BINARY_DIR}/include/dzieja/Basic/Version.inc)

include_directories(BEFORE
        ${DZIEJA_BINARY_DIR}/include
        ${DZIEJA_SOURCE_DIR}/include)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(AddDzieja)

if (NOT LLVM_INSTALL_TOOLCHAIN_ONLY)
    install(DIRECTORY include/dzieja
            DESTINATION include
            COMPONENT dzieja-headers
            FILES_MATCHING
            PATTERN "*.def"
            PATTERN "*.h"
            PATTERN "config.h" EXCLUDE
            PATTERN ".svn" EXCLUDE
            )

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dzieja
            DESTINATION include
            COMPONENT dzieja-headers
            FILES_MATCHING
            PATTERN "CMakeFiles" EXCLUDE
            PATTERN "*.inc"
            PATTERN "*.h"
            )

    # Installing the headers needs to depend on generating any public
    # tablegen'd headers.
    add_custom_target(dzieja-headers DEPENDS dzieja-tablegen-targets)
    set_target_properties(dzieja-headers PROPERTIES FOLDER "Misc")
    if(NOT LLVM_ENABLE_IDE)
        add_llvm_install_targets(install-dzieja-headers
                DEPENDS dzieja-headers
                COMPONENT dzieja-headers)
    endif()
endif()

add_subdirectory(lib)
add_subdirectory(utils)

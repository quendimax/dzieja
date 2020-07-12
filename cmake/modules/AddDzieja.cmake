macro(set_dzieja_windows_version_resource_properties name)
    if(DEFINED windows_resource_file)
        set_windows_version_resource_properties(${name} ${windows_resource_file}
                VERSION_MAJOR ${DZIEJA_VERSION_MAJOR}
                VERSION_MINOR ${DZIEJA_VERSION_MINOR}
                VERSION_PATCHLEVEL ${DZIEJA_VERSION_PATCH}
                VERSION_STRING "${DZIEJA_VERSION} (${BACKEND_PACKAGE_STRING})"
                PRODUCT_NAME "dzieja")
    endif()
endmacro()

macro(add_dzieja_subdirectory name)
    add_llvm_subdirectory(DZIEJA TOOL ${name})
endmacro()

macro(add_dzieja_library name)
    cmake_parse_arguments(ARG
            "SHARED;INSTALL_WITH_TOOLCHAIN"
            ""
            "ADDITIONAL_HEADERS"
            ${ARGN})
    set(srcs)
    if(MSVC_IDE OR XCODE)
        # Add public headers
        file(RELATIVE_PATH lib_path
                ${DZIEJA_SOURCE_DIR}/lib/
                ${CMAKE_CURRENT_SOURCE_DIR}
                )
        if(NOT lib_path MATCHES "^[.][.]")
            file( GLOB_RECURSE headers
                    ${DZIEJA_SOURCE_DIR}/include/dzieja/${lib_path}/*.h
                    ${DZIEJA_SOURCE_DIR}/include/dzieja/${lib_path}/*.def
                    )
            set_source_files_properties(${headers} PROPERTIES HEADER_FILE_ONLY ON)

            file( GLOB_RECURSE tds
                    ${DZIEJA_SOURCE_DIR}/include/dzieja/${lib_path}/*.td
                    )
            source_group("TableGen descriptions" FILES ${tds})
            set_source_files_properties(${tds}} PROPERTIES HEADER_FILE_ONLY ON)

            if(headers OR tds)
                set(srcs ${headers} ${tds})
            endif()
        endif()
    endif(MSVC_IDE OR XCODE)
    if(srcs OR ARG_ADDITIONAL_HEADERS)
        set(srcs
                ADDITIONAL_HEADERS
                ${srcs}
                ${ARG_ADDITIONAL_HEADERS} # It may contain unparsed unknown args.
                )
    endif()
    if(ARG_SHARED)
        set(LIBTYPE SHARED)
    else()
        # llvm_add_library ignores BUILD_SHARED_LIBS if STATIC is explicitly set,
        # so we need to handle it here.
        if(BUILD_SHARED_LIBS)
            set(LIBTYPE SHARED)
        else()
            set(LIBTYPE STATIC)
        endif()
        if(NOT XCODE)
            # The Xcode generator doesn't handle object libraries correctly.
            list(APPEND LIBTYPE OBJECT)
        endif()
        set_property(GLOBAL APPEND PROPERTY CLANG_STATIC_LIBS ${name})
    endif()
    llvm_add_library(${name} ${LIBTYPE} ${ARG_UNPARSED_ARGUMENTS} ${srcs})

    if(TARGET ${name})
        target_link_libraries(${name} INTERFACE ${LLVM_COMMON_LIBS})

        if (NOT LLVM_INSTALL_TOOLCHAIN_ONLY OR ARG_INSTALL_WITH_TOOLCHAIN)
            set(export_to_dziejatargets)
            if(${name} IN_LIST LLVM_DISTRIBUTION_COMPONENTS OR
                    "dzieja-libraries" IN_LIST LLVM_DISTRIBUTION_COMPONENTS OR
                    NOT LLVM_DISTRIBUTION_COMPONENTS)
                set(export_to_dziejatargets EXPORT DziejaTargets)
                set_property(GLOBAL PROPERTY DZIEJA_HAS_EXPORTS True)
            endif()

            install(TARGETS ${name}
                    COMPONENT ${name}
                    ${export_to_dziejatargets}
                    LIBRARY DESTINATION lib${LLVM_LIBDIR_SUFFIX}
                    ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX}
                    RUNTIME DESTINATION bin)

            if (NOT LLVM_ENABLE_IDE)
                add_llvm_install_targets(install-${name}
                        DEPENDS ${name}
                        COMPONENT ${name})
            endif()

            set_property(GLOBAL APPEND PROPERTY DZIEJA_LIBS ${name})
        endif()
        set_property(GLOBAL APPEND PROPERTY DZIEJA_EXPORTS ${name})
    else()
        # Add empty "phony" target
        add_custom_target(${name})
    endif()

    set_target_properties(${name} PROPERTIES FOLDER "Dzieja libraries")
    set_dzieja_windows_version_resource_properties(${name})
endmacro(add_dzieja_library)

macro(add_dzieja_executable name)
    add_llvm_executable( ${name} ${ARGN} )
    set_target_properties(${name} PROPERTIES FOLDER "Dzieja executables")
    set_dzieja_windows_version_resource_properties(${name})
endmacro(add_dzieja_executable)

macro(add_dzieja_tool name)
    if (NOT DZIEJA_BUILD_TOOLS)
        set(EXCLUDE_FROM_ALL ON)
    endif()

    add_dzieja_executable(${name} ${ARGN})
    add_dependencies(${name} dzieja-resource-headers)

    if (DZIEJA_BUILD_TOOLS)
        set(export_to_dziejatargets)
        if(${name} IN_LIST LLVM_DISTRIBUTION_COMPONENTS OR
                NOT LLVM_DISTRIBUTION_COMPONENTS)
            set(export_to_dziejatargets EXPORT DziejaTargets)
            set_property(GLOBAL PROPERTY DZIEJA_HAS_EXPORTS True)
        endif()

        install(TARGETS ${name}
                ${export_to_dziejatargets}
                RUNTIME DESTINATION bin
                COMPONENT ${name})

        if(NOT LLVM_ENABLE_IDE)
            add_llvm_install_targets(install-${name}
                    DEPENDS ${name}
                    COMPONENT ${name})
        endif()
        set_property(GLOBAL APPEND PROPERTY DZIEJA_EXPORTS ${name})
    endif()
endmacro()

macro(add_dzieja_symlink name dest)
    add_llvm_tool_symlink(${name} ${dest} ALWAYS_GENERATE)
    # Always generate install targets
    llvm_install_symlink(${name} ${dest} ALWAYS_GENERATE)
endmacro()

function(dzieja_target_link_libraries target type)
    target_link_libraries(${target} ${type} ${ARGN})
endfunction()

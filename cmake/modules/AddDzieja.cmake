macro(add_dzieja_library name)
    llvm_add_library(${name} ${ARGN})
    set_target_properties(${name} PROPERTIES FOLDER "Dzieja libraries")
endmacro()

macro(add_dzieja_executable name)
    add_llvm_executable( ${name} ${ARGN} )
    set_target_properties(${name} PROPERTIES FOLDER "Dzieja executables")
endmacro()

set(LLVM_LINK_COMPONENTS
    Support
)

add_llvm_executable(dzieja-lexgen
    FiniteAutomaton.cpp
    FiniteAutomaton.h
    main.cpp
)

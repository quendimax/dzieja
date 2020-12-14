set(LLVM_LINK_COMPONENTS
    Support
)

add_dzieja_executable(dzieja-lexgen
    FiniteAutomaton.cpp
    FiniteAutomaton.h
    main.cpp
)

add_executable(lexgen
    FiniteAutomaton.cpp
    FiniteAutomaton.h
    main.cpp
)

target_link_libraries(lexgen
    LLVMSupport
)

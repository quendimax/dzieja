set(LLVM_LINK_COMPONENTS
    Support
)

add_dzieja_executable(dzieja-lexer
    main.cpp
)

target_link_libraries(dzieja-lexer
    PRIVATE
        dziejaLex
)

set(LLVM_LINK_COMPONENTS
    Support
)

set(DFA_IMPL_FILE "${DZIEJA_BINARY_DIR}/include/dzieja/Basic/LexDFAImpl.inc")

add_llvm_library(dziejaLex
    Lexer.cpp
    ${DFA_IMPL_FILE}
)

add_custom_command(
    OUTPUT ${DFA_IMPL_FILE}
    COMMAND dzieja-lexgen -gen-via-switch -o ${DFA_IMPL_FILE}
    DEPENDS ${DZIEJA_SOURCE_DIR}/include/dzieja/Basic/TokenKinds.def
    DEPENDS dzieja-lexgen
)

set(LEX_DFA_FILE "${DZIEJA_BINARY_DIR}/include/dzieja/Basic/LexDFAImpl.inc")

set(INCLUDE_DIR "${DZIEJA_SOURCE_DIR}/include/dzieja/Lex")

add_dzieja_library(dziejaLex
    "${INCLUDE_DIR}/Lexer.h"
    "${INCLUDE_DIR}/Token.h"
    Lexer.cpp
    "${LEX_DFA_FILE}"

    LINK_COMPONENTS Support
)

target_link_libraries(dziejaLex
    PUBLIC 
        dziejaBasic   # needed for Token::getName implementation via tok::getTokenName
)

add_custom_command(
    OUTPUT "${LEX_DFA_FILE}"
    COMMAND dzieja-lexgen -gen-via-table -o "${LEX_DFA_FILE}"
    DEPENDS dzieja-lexgen
)

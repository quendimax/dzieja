set(INCLUDE_DIR "${DZIEJA_SOURCE_DIR}/include/dzieja/Basic")

add_dzieja_library(dziejaBasic
    "${INCLUDE_DIR}/TokenKinds.h"
    TokenKinds.cpp

    LINK_COMPONENTS Support # for the llvm_unreachable, i.e. debug mode
)

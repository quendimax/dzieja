#ifndef DZIEJA_LEX_TOKEN_H
#define DZIEJA_LEX_TOKEN_H

#include "dzieja/Basic/TokenKinds.h"


namespace dzieja {

/// Contains information about a lexed token.
class Token {
    friend class Lexer;

    /// Pointers on the beginning of token in source buffer.
    const char *BufferPtr;

    /// Length of lexed token.
    // TODO: consider if it is needed to use size_t type
    unsigned Len;

    tok::TokenKind Kind = tok::unknown;

public:
    tok::TokenKind getKind() const { return Kind; }
    void setKind(tok::TokenKind kind) { Kind = kind; }
};

} // namespace dzieja


#endif // DZIEJA_LEX_TOKEN_H

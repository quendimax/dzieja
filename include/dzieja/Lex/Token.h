#ifndef DZIEJA_LEX_TOKEN_H
#define DZIEJA_LEX_TOKEN_H

#include "dzieja/Basic/TokenKinds.h"

namespace dzieja {

/// Contains information about a lexed token.
class Token {
    /// Points on the beginning of token in source buffer.
    const char *BufferPtr;

    /// Length of lexed token.
    // TODO: consider if it is needed to use size_t type
    unsigned Len;

    tok::TokenKind Kind = tok::unknown;

public:
    tok::TokenKind getKind() const { return Kind; }
    void setKind(tok::TokenKind kind) { Kind = kind; }

    bool is(tok::TokenKind kind) const { return Kind == kind; }
    bool isOneOf(tok::TokenKind kind) const { return is(kind); }

    template<typename... Kinds>
    bool isOneOf(tok::TokenKind K, Kinds... Ks) const
    {
        return is(K) || isOneOf(Ks...);
    }

    const char *getName() const { return tok::getTokenName(Kind); }

    const char *getBufferPtr() const { return BufferPtr; }
    void setBufferPtr(const char *bufferPtr) { BufferPtr = bufferPtr; }

    unsigned getLength() const { return Len; }
    void setLength(unsigned length) { Len = length; }
};

} // namespace dzieja

#endif // DZIEJA_LEX_TOKEN_H

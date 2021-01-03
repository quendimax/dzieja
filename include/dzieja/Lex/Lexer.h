#ifndef DZIEJA_LEX_LEXER_H
#define DZIEJA_LEX_LEXER_H

namespace llvm {
class MemoryBuffer;
}

namespace dzieja {

class Token;

class Lexer {
    const char *BufferStart;
    const char *BufferEnd;
    const char *BufferPtr;

    /// If this mode is enabled \p lex method returns \c comment tokens too.
    bool InCommentRetentionMode = false;

public:
    Lexer(const char *bufferStart, const char *bufferPtr, const char *bufferEnd);
    explicit Lexer(const llvm::MemoryBuffer *inputFile);

    Lexer(const Lexer &) = delete;
    Lexer &operator=(const Lexer &) = delete;

    /// Reads next token from an input buffer. Depending on the settings it can skip comment tokens.
    void lex(Token &result);

    void enableCommentRetentionMode() { InCommentRetentionMode = true; }
    void disableCommentRetentionMode() { InCommentRetentionMode = false; }
    bool inCommentRetentionMode() const { return InCommentRetentionMode; }

private:
    /// Reads next token from an input buffer.
    ///
    /// It reads every token includeing comments and gaps. \p lex method decides which token must be
    /// returned to the client code.
    void lexInternal(Token &result);
};

} // namespace dzieja

#endif // DZIEJA_LEX_LEXER_H

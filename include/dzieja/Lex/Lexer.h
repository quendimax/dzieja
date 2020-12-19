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

public:
    Lexer(const char *bufferStart, const char *bufferPtr, const char *bufferEnd);
    explicit Lexer(const llvm::MemoryBuffer *inputFile);

    Lexer(const Lexer &) = delete;
    Lexer &operator=(const Lexer &) = delete;

    void lex(Token &result);
};

} // namespace dzieja

#endif // DZIEJA_LEX_LEXER_H

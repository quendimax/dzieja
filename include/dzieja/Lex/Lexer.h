#ifndef DZIEJA_LEX_LEXER_H
#define DZIEJA_LEX_LEXER_H

namespace llvm {
class MemoryBuffer;
}

namespace dzieja {

class Token;

class Lexer
{
    const char *BufferStart;
    const char *BufferEnd;
    const char *BufferPtr;

public:
    Lexer(const char *BufStart, const char *BufPtr, const char *BufEnd);
    explicit Lexer(const llvm::MemoryBuffer *InputFile);

    Lexer(const Lexer &) = delete;
    Lexer &operator=(const Lexer &) = delete;

    void Lex(Token &Result);
};

} // namespace dzieja

#endif // DZIEJA_LEX_LEXER_H

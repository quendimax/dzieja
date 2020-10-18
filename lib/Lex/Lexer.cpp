#include "dzieja/Lex/Lexer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/MemoryBuffer.h"
#include <cassert>

using namespace llvm;

namespace dzieja {

Lexer::Lexer(const char *BufStart, const char *BufPtr, const char *BufEnd)
    : BufferStart(BufStart), BufferEnd(BufEnd), BufferPtr(BufPtr)
{
    assert(BufEnd[0] == '\0' && "expected null at the end of the buffer");

    // Check whether we have a BOM in the beginning of the buffer
    if (BufferStart == BufferPtr) {
        StringRef Buffer(BufferStart, BufferEnd - BufferStart);
        size_t BOMLength = StringSwitch<size_t>(Buffer)
            .StartsWith("\xEF\xBB\xBF", 3)
            .Default(0);
        BufferStart += BOMLength;
    }
}

Lexer::Lexer(const MemoryBuffer *InputFile)
    : Lexer(InputFile->getBufferStart(), InputFile->getBufferStart(), InputFile->getBufferEnd()) {}

void Lexer::Lex(Token &Result) {}

} // namespace dzieja

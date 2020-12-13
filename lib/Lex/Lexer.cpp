#include "dzieja/Lex/Lexer.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/MemoryBuffer.h>

// these two headers are needed for LexDFAImpl.inc
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace dzieja {

Lexer::Lexer(const char *BufStart, const char *BufPtr, const char *BufEnd)
    : BufferStart(BufStart), BufferEnd(BufEnd), BufferPtr(BufPtr)
{
    assert(BufEnd[0] == '\0' && "expected null at the end of the buffer");

    // Check whether we have a UTF-8 BOM in the beginning of the buffer
    if (BufferStart == BufferPtr) {
        StringRef Buffer(BufferStart, BufferEnd - BufferStart);
        if (Buffer.startswith("\xEF\xBB\xBF"))
            BufferStart += 3;
    }
}

Lexer::Lexer(const MemoryBuffer *InputFile)
    : Lexer(InputFile->getBufferStart(), InputFile->getBufferStart(), InputFile->getBufferEnd())
{
}

// This file is an implementation of DFA for lexer. It is consist of functions
// DFA_delta(stateID, symbol) and DFA_getKind(stateID), and a constant DFA_InvalidStateID.
// This file is generated with the dzieja-lexgen util from the dzieja/Basic/TokenKinds.def source.
#include "dzieja/Basic/LexDFAImpl.inc"

void Lexer::Lex(Token &Result) {}

} // namespace dzieja

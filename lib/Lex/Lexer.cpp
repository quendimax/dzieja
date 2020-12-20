#include "dzieja/Lex/Lexer.h"

#include "dzieja/Basic/TokenKinds.h"
#include "dzieja/Lex/Token.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/WithColor.h>

// these two headers are needed for LexDFAImpl.inc
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace dzieja {

Lexer::Lexer(const char *bufferStart, const char *bufferPtr, const char *bufferEnd)
    : BufferStart(bufferStart), BufferEnd(bufferEnd), BufferPtr(bufferPtr)
{
    assert(BufferEnd[0] == '\0' && "expected null at the end of the buffer");

    // Check whether we have a UTF-8 BOM in the beginning of the buffer
    if (BufferStart == BufferPtr) {
        StringRef buffer(BufferStart, BufferEnd - BufferStart);
        if (buffer.startswith("\xEF\xBB\xBF"))
            BufferStart += 3;
    }
}

void Lexer::lex(Token &result)
{
    do {
        lexInternal(result);
    } while (result.getKind() == tok::gap || result.getKind() == tok::comment);
}

// This file is an implementation of DFA for lexer. It is consist of functions
// DFA_delta(stateID, symbol) and DFA_getKind(stateID), and a constant DFA_InvalidStateID.
// This file is generated with the dzieja-lexgen util from the dzieja/Basic/TokenKinds.def source.
#include "dzieja/Basic/LexDFAImpl.inc"

void Lexer::lexInternal(Token &result)
{
    unsigned prevID = DFA_InvalidStateID;
    unsigned stateID = DFA_InvalidStateID;
    const char *tokStartPtr = BufferPtr;

    do {
        prevID = stateID;
        stateID = DFA_delta(stateID, *BufferPtr++);
    } while (stateID != DFA_InvalidStateID);

    if (DFA_getKind(prevID) == tok::unknown) {
        WithColor::error() << "unexpected symbol '" << *(BufferPtr - 1) << "'";
        std::exit(1);
    }

    result.BufferPtr = tokStartPtr;
    result.Len = BufferPtr - tokStartPtr;
    result.setKind((tok::TokenKind)DFA_getKind(prevID));
}

} // namespace dzieja

#include "dzieja/Basic/TokenKinds.h"
#include <llvm/Support/ErrorHandling.h>


using namespace dzieja;


static const char *const TokenNames[] = {
#define TOK(name) #name,
#define KEYWORD(name) #name,
#include "dzieja/Basic/TokenKinds.def"
    nullptr
};


const char *tok::getTokenName(tok::TokenKind kind)
{
    if (kind < tok::NUM_TOKENS)
        return TokenNames[kind];
    llvm_unreachable("unknown TokenKind");
}

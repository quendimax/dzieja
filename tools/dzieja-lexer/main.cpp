#include "dzieja/Basic/TokenKinds.h"
#include "dzieja/Lex/Lexer.h"
#include "dzieja/Lex/Token.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/WithColor.h>

#include <string>

using namespace llvm;
using namespace dzieja;

static cl::opt<std::string> Input(cl::Positional, cl::Required);
static cl::opt<bool> PrintTokenName("print-tok-name", cl::init(false),
                                    cl::desc("Print tokens' names separated with new line"));
static cl::opt<bool>
    PrintTokenSpelling("print-tok-spell", cl::init(false),
                       cl::desc("Print tokens' spellings separated with new line"));
static cl::opt<int> Repeat("repeat", cl::init(1), cl::desc("Repeat lexing of a file N times"));

int main(int argc, const char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    auto buffer = llvm::MemoryBuffer::getFile(Input);
    if (!buffer) {
        WithColor::error(llvm::errs(), "dzieja-lexer") << buffer.getError().message();
        return 1;
    }

    for (int i = 0; i < Repeat; ++i) {
        Lexer L(buffer.get().get());
        Token T;
        do {
            L.lex(T);
            if (PrintTokenName)
                llvm::outs() << T.getName() << "\n";
            if (PrintTokenSpelling)
                llvm::outs() << T.getSpelling() << "\n";
        } while (!T.is(dzieja::tok::eof));
    }

    return 0;
}

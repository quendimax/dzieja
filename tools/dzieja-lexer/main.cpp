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

int main(int argc, const char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    auto buffer = llvm::MemoryBuffer::getFile(Input);
    if (!buffer) {
        WithColor::error(llvm::errs(), "dzieja-lexer") << buffer.getError().message();
        return 1;
    }

    Lexer L(buffer.get().get());
    Token T;
    do {
        L.lex(T);
    } while (!T.is(dzieja::tok::eof));

    if (!T.is(tok::gap)) {
        llvm::outs() << "'" << StringRef(T.getBufferPtr(), T.getLength()) << "' - " << T.getName()
                     << "\n";
    }
    else {
        llvm::outs() << "'";
        llvm::outs().write_escaped(StringRef(T.getBufferPtr(), T.getLength()))
            << "' - " << T.getName() << "\n";
    }

    return 0;
}

#include "FiniteAutomaton.h"
#include "dzieja/Basic/TokenKinds.h"
#include <llvm/Support/CommandLine.h>
#include <string>

using namespace dzieja;
using namespace llvm;

cl::opt<std::string> Output("o", cl::desc("Specify output filename"), cl::init("dfa.inc"),
                            cl::value_desc("filename"));

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    NFA nfa;
#define TOKEN(name, str) nfa.parseRawString(str, tok::name);
#define TOKEN_REGEX(name, regex) nfa.parseRegex(regex, tok::name);
#include "dzieja/Basic/TokenKinds.def"

    NFA dfa = nfa.buildDFA();

    nfa.print(llvm::outs());
    llvm::outs() << "\n";
    dfa.print(llvm::outs());

    if (!dfa.generateCppImpl(Output.c_str()))
        return 1;
    return 0;
}

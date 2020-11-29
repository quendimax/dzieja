#include "FiniteAutomaton.h"
#include "dzieja/Basic/TokenKinds.h"
#include <llvm/Support/CommandLine.h>
#include <string>

using namespace dzieja;
using namespace llvm;


static cl::opt<std::string> Output("o", cl::desc("Specify output filename"), cl::init("a.inc"),
                                   cl::value_desc("filename"));
static cl::opt<NFA::GeneratingMode>
    GenMode(cl::init(NFA::GM_Table),
            cl::values(clEnumValN(NFA::GM_Table, "gen-via-table",
                                  "Generate the function via transitive table"),
                       clEnumValN(NFA::GM_Switch, "gen-via-switch",
                                  "Generate the function via switch-case control flow")),
            cl::desc("Specify mode of transitive (delta) function generating"));


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

    if (!dfa.generateCppImpl(Output.c_str(), GenMode))
        return 1;
    return 0;
}

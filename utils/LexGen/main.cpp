#include "FiniteAutomaton.h"
#include "dzieja/Basic/TokenKinds.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include <string>

using namespace dzieja;
using namespace llvm;


static cl::opt<std::string> Output("o", cl::desc("Specify output filename."), cl::init("a.inc"),
                                   cl::value_desc("filename"));
static cl::opt<bool> NoMinimization("no-minimization", cl::init(false),
                                    cl::desc("Don't apply any minimization algorithm for DFA."));
static cl::opt<bool> Verbose("v", cl::init(false),
                             cl::desc("Print some information about a DFA building process."));
static cl::opt<NFA::GeneratingMode>
    GenMode(cl::init(NFA::GM_Table),
            cl::desc("Specify mode of transitive (delta) function generating:"),
            cl::values(clEnumValN(NFA::GM_Table, "gen-via-table",
                                  "Generate the function via transitive table (default)."),
                       clEnumValN(NFA::GM_Switch, "gen-via-switch",
                                  "Generate the function via switch-case control flow.")));

static const char *Overview =
    "The program generates an inc-file with functions implementing DFA for\n"
    "          lexical analyze of text.\n";

NFA buildNFA()
{
    NFA nfa;
#define TOKEN(name, str) nfa.parseRawString(str, tok::name);
#define TOKEN_REGEX(name, regex) nfa.parseRegex(regex, tok::name);
#include "dzieja/Basic/TokenKinds.def"

    if (Verbose)
        llvm::errs() << "NFA has " << nfa.getNumStates() << " states.\n";

#define DEBUG_TYPE "nfa"
    LLVM_DEBUG(nfa.print(llvm::errs()) << "\n");
#undef DEBUG_TYPE

    return nfa;
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv, Overview);

    NFA nfa = buildNFA();
    NFA dfa = nfa.buildDFA();
    if (Verbose)
        llvm::errs() << "DFA has " << dfa.getNumStates() << " states.\n";

#define DEBUG_TYPE "dfa"
    LLVM_DEBUG(dfa.print(llvm::errs()) << "\n");
#undef DEBUG_TYPE

    if (NoMinimization) {
        if (!dfa.generateCppImpl(Output.c_str(), GenMode))
            return 1;
        return 0;
    }

    NFA minDfa = dfa.buildMinimizedDFA();
    if (Verbose)
        llvm::errs() << "minDFA has " << minDfa.getNumStates() << " states.\n";

#define DEBUG_TYPE "min-dfa"
    LLVM_DEBUG(minDfa.print(llvm::errs()) << "\n");
#undef DEBUG_TYPE

    if (!minDfa.generateCppImpl(Output.c_str(), GenMode))
        return 1;
    return 0;
}

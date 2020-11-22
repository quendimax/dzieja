#include "FiniteAutomaton.h"

int main()
{
    NFA nfa;
    nfa.parseRawString("for");
    nfa.parseRawString("free");
    nfa.parseRawString("free");
    nfa.parseRawString("from");
    NFA dfa = nfa.buildDFA();

    nfa.print(llvm::outs());
    llvm::outs() << "\n";
    dfa.print(llvm::outs());

    dfa.generateCppImpl("/home/quendimax/dfa.inc");

    return 0;
}

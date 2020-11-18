#include "FiniteAutomaton.h"

int main()
{
    NFA nfa;
    nfa.parseRawString("for");
    nfa.parseRawString("free");
    NFA dfa = nfa.buildDFA();

    nfa.print(llvm::outs());
    llvm::outs() << "\n";
    dfa.print(llvm::outs());

    return 0;
}

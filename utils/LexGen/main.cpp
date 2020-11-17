#include "FiniteAutomaton.h"

int main()
{
    NFA nfa;
    nfa.parseRawString("for");
    nfa.parseRawString("free");
    NFA dfa = nfa.buildDFA();

    return 0;
}

#include "FiniteAutomaton.h"
#include "dzieja/Basic/TokenKinds.h"

using namespace dzieja;

int main()
{
    NFA nfa;

#define TOKEN(name, str) nfa.parseRawString(str, tok::name);
#define TOKEN_REGEX(name, regex) nfa.parseRegex(regex, tok::name);
#include "dzieja/Basic/TokenKinds.def"

    NFA dfa = nfa.buildDFA();

    nfa.print(llvm::outs());
    llvm::outs() << "\n";
    dfa.print(llvm::outs());

    dfa.generateCppImpl("dfa.inc");

    return 0;
}

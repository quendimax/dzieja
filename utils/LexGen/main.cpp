#include "FiniteAutomaton.h"

int main()
{
    auto q0 = State::makeState();
    auto q1 = State::makeState(true);
    q0->connectTo(q1, 'a');
    ConvNFAtoDFA converter(q0);
    auto dfaQ0 = converter.convert();
    return dfaQ0->getValue();
}

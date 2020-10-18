#include "NFA.h"

int main()
{
    auto q0 = State::makeState();
    auto q1 = State::makeState(true);
    q0->connectTo(q1, 'a');
    return 0;
}
#include "NFA.h"
#include <memory>

using namespace std;

State *State::makeState(bool isTerminal)
{
    static int ValueCounter = 0;
    static vector<unique_ptr<State>> stateStorage;
    stateStorage.push_back(unique_ptr<State>(new State(ValueCounter++, isTerminal)));
    return stateStorage.back().get();
}

void State::connectTo(State *state, int symbol)
{
    edges.push_back(Edge(symbol, state));
}
#include "NFA.h"
#include <functional>
#include <memory>

using namespace std;

State *State::makeState(bool isTerminal)
{
    static int ValueCounter = 0;
    static vector<State> stateStorage;
    stateStorage.push_back(State(ValueCounter++, isTerminal));
    return &stateStorage.back();
}

void State::connectTo(const State *state, int symbol)
{
    edges.push_back(Edge(symbol, state));
}

set<const State *> State::getEspClosure() const
{
    set<const State *> closure;
    function<void (const State *state)> finder;
    finder = [&closure, &finder](const State *state) {
        if (closure.contains(state))
            return;
        closure.insert(state);
        for (const Edge &edge : state->getEdges()) {
            if (edge.getSymbol() == Edge::Epsilon)
                finder(edge.getState());
        }
    };
    finder(this);
    return closure;
}

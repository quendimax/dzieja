#include "FiniteAutomaton.h"
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <unordered_set>

using namespace std;

State *State::makeState(bool isTerminal)
{
    static int ValueCounter = 0;
    static vector<unique_ptr<State>> stateStorage;
    auto newState = new State(ValueCounter++, isTerminal);
    stateStorage.push_back(unique_ptr<State>(newState));
    return stateStorage.back().get();
}

void State::connectTo(const State *state, int symbol)
{
    edges.push_back(Edge(symbol, state));
}

StateSet State::getEspClosure() const
{
    StateSet closure;
    function<void(const State *state)> finder;
    finder = [&closure, &finder](const State *state) {
        if (closure.contains(state))
            return;
        closure.insert(state);
        for (const Edge &edge : state->getEdges()) {
            if (edge.getSymbol() == Edge::Epsilon)
                finder(edge.getTarget());
        }
    };
    finder(this);
    return closure;
}

// This function looks for new state set, an equivalent of the next DNA state for the edge`symbol`.
static StateSet findDFAState(const StateSet &states, int symbol)
{
    StateSet newStates;
    for (const auto &state : states)
        for (const auto &edge : state->getEdges())
            if (edge.getSymbol() == symbol) {
                auto closure = edge.getTarget()->getEspClosure();
                newStates.insert(closure.begin(), closure.end());
            }
    return newStates;
}

const State *ConvNFAtoDFA::convert()
{
    StateSet setQ0 = q0->getEspClosure();
    return _convert(setQ0);
}

State *ConvNFAtoDFA::_convert(const StateSet &set)
{
    if (convTable.contains(set))
        return convTable.at(set);

    auto newState = State::makeState();
    for (const auto state : set) {
        if (state->isTerminal()) {
            newState->setTerminal();
            break;
        }
    }
    convTable[set] = newState;

    std::unordered_set<int> symbols;
    for (auto state : set)
        for (const Edge &edge : state->getEdges())
            symbols.insert(edge.getSymbol());

    for (auto symbol : symbols) {
        auto targetSet = findDFAState(set, symbol);
        assert(!targetSet.empty() && "target set musn't be empty");
        auto targetState = _convert(targetSet);
        newState->connectTo(targetState, symbol);
    }
    return newState;
}

#include "FiniteAutomaton.h"

#include <llvm/ADT/STLExtras.h>
#include <cassert>
#include <functional>
#include <memory>

using namespace llvm;
using namespace std;

StateSet State::getEspClosure() const
{
    StateSet closure;
    function<void(const State *state)> finder;
    finder = [&closure, &finder](const State *state) {
        if (is_contained(closure, state))
            return;
        closure.insert(state);
        for (const Edge &edge : state->getEdges())
            if (edge.getSymbol() == Edge::Epsilon)
                finder(edge.getTarget());
    };
    finder(this);
    return closure;
}

State *NFA::makeState(bool terminal)
{
    auto newState = new State(terminal);
    storage.push_back(unique_ptr<State>(newState));
    return storage.back().get();
}

void NFA::parseRawString(const char *str)
{
    auto curState = makeState();
    Q0->connectTo(curState, Edge::Epsilon);
    while (*str++) {
        auto newState = makeState();
        curState->connectTo(newState, *str);
        curState = newState;
    }
    curState->setTerminal();
}

void NFA::parseRegex(const char *regex)
{
    // TODO: add parsing of regex
}

/// This function looks for new state set, an equivalent of the next DNA state for the edge
/// `symbol`.
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

/// Builds new NFA that satisfies the DNA requirements.
///
/// Every state of new DNA has only one edge for one symbol.
NFA NFA::buildDFA() const
{
    map<const StateSet, State *> convTable;
    NFA dfa;
    dfa.storage.pop_back(); // by default NFA instance contane start state
    StateSet setQ0 = getStartState()->getEspClosure();
    function<State *(const StateSet &set)> convert;

    convert = [&](const StateSet &set) {
        auto iter = convTable.find(set);
        if (iter != convTable.end())
            return iter->second;

        auto newState = dfa.makeState();
        for (const auto state : set) {
            if (state->isTerminal()) {
                newState->setTerminal();
                break;
            }
        }
        convTable[set] = newState;

        SmallSet<Symbol, 8> symbols;
        for (auto state : set)
            for (const Edge &edge : state->getEdges())
                symbols.insert(edge.getSymbol());

        for (auto symbol : symbols) {
            auto targetSet = findDFAState(set, symbol);
            assert(!targetSet.empty() && "target set musn't be empty");
            auto targetState = convert(targetSet);
            newState->connectTo(targetState, symbol);
        }
        return newState;
    };
    dfa.Q0 = convert(setQ0);
    dfa.isDFA = true;
    return dfa;
}

void NFA::print(raw_ostream &out) const {
    for (const auto &state: storage) {
        out << "State(" << state.get() << ")\n";
        for (const auto &edge: state->getEdges()) {
            out << " |- ";
            if (edge.getSymbol() == Edge::Epsilon)
                out << "Eps";
            else
                out << (char)edge.getSymbol();
            out << " - State(" << edge.getTarget() << ")\n";
        }
    }
}

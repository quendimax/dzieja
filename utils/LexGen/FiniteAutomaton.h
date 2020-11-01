#ifndef DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H
#define DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <vector>

class State;

using StateSet = std::set<const State *>;

class Edge {
    const State *state;
    int symbol;

public:
    enum { Epsilon = std::numeric_limits<int>::min() };

    Edge(int symbol, const State *state = nullptr) : state(state), symbol(symbol) {}
    bool isEpsilon() const { return symbol == Epsilon; }
    int getSymbol() const { return symbol; }
    const State *getTarget() const { return state; }
};

class State {
    std::vector<Edge> edges;
    int value;
    bool terminal;

    State(int value, bool terminal) : value(value), terminal(terminal) {}
    State(const State &) = delete;
    State &operator=(const State &) = delete;

public:
    static State *makeState(bool isTerminal = false);

    int getValue() const { return value; }
    bool isTerminal() const { return terminal; }
    void setTerminal() { terminal = true; }
    const std::vector<Edge> &getEdges() const { return edges; }
    void connectTo(const State *state, int symbol);
    StateSet getEspClosure() const;
};

class ConvNFAtoDFA {
    State *q0;
    std::map<const StateSet, State *> convTable;

public:
    ConvNFAtoDFA(State *q0) : q0(q0) {}

    /// returns q0 of original NFA which will be converted
    const State *getOriginalQ0() const { return q0; }

    /// returns q0 of new converted DFA
    const State *convert();

private:
    State *_convert(const StateSet &set);
};

#endif // DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

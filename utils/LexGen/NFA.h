#ifndef DZIEJA_UTILS_LEXGEN_NFA_H
#define DZIEJA_UTILS_LEXGEN_NFA_H

#include <limits>
#include <iostream>
#include <vector>
#include <set>

class State;

class Edge {
    const State *state;
    int symbol;

public:
    enum { Epsilon = std::numeric_limits<int>::min() };

    Edge(int symbol, const State *state = nullptr) : state(state), symbol(symbol) {}
    bool isEpsilon() const { return symbol == Epsilon; }
    int getSymbol() const { return symbol; }
    const State *getState() const { return state; }
};

class State {
    std::vector<Edge> edges;
    int value;
    bool terminal;

    State(int value, bool terminal) : value(value), terminal(terminal) {}

public:
    static State *makeState(bool isTerminal = false);

    int getValue() const { return value; }
    bool isTerminal() const { return terminal; }
    const std::vector<Edge> &getEdges() const { return edges; }
    void connectTo(const State *state, int symbol);
    std::set<const State *> getEspClosure() const;
};

#endif // DZIEJA_UTILS_LEXGEN_NFA_H

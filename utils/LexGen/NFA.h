#ifndef DZIEJA_UTILS_LEXGEN_NFA_H
#define DZIEJA_UTILS_LEXGEN_NFA_H

#include <limits>
#include <vector>
#include <iostream>

class State;

class Edge {
    State *state;
    int symbol;

public:
    enum { Epsilon = std::numeric_limits<int>::min() };

    Edge(int symbol, State *state = nullptr) : state(state), symbol(symbol) {}
    bool isEpsilon() const { return symbol == Epsilon; }
    int getSymbol() const { return symbol; }
    void setSymbol(int symbol) { this->symbol = symbol; }
    State *getState() const { return state; }
};

class State {
    std::vector<Edge> edges;
    int value;
    bool terminal;

    State() = delete;
    State(int value, bool terminal) : value(value), terminal(terminal) {}

public:
    static State *makeState(bool isTerminal = false);

    int getValue() const { return value; }
    bool isTerminal() const { return terminal; }
    const std::vector<Edge> &getEdges() const { return edges; }
    void connectTo(State *state, int symbol);

    ~State() { std::cout << "Bye\n"; }
};

#endif // DZIEJA_UTILS_LEXGEN_NFA_H

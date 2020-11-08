#ifndef DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H
#define DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

#include <limits>
#include <map>
#include <set>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>

class State;

using Symbol = int;
using StateSet = std::set<const State *>;


class Edge {
    const State *target;
    Symbol symbol;

public:
    enum { Epsilon = std::numeric_limits<Symbol>::min() };

    Edge(Symbol symbol, const State *target = nullptr) : target(target), symbol(symbol) {}
    bool isEpsilon() const { return symbol == Epsilon; }
    Symbol getSymbol() const { return symbol; }
    const State *getTarget() const { return target; }
};


class State {
    llvm::SmallVector<Edge, 0> edges;
    bool terminal;

public:
    State(bool terminal) : terminal(terminal) {}

    bool isTerminal() const { return terminal; }
    void setTerminal() { terminal = true; }
    const llvm::SmallVectorImpl<Edge> &getEdges() const { return edges; }
    void connectTo(const State *state, Symbol symbol) { edges.push_back(Edge(symbol, state)); }
    StateSet getEspClosure() const;

private:
    State(const State &) = delete;
    State &operator=(const State &) = delete;
};


class NFA {
    llvm::SmallVector<std::unique_ptr<State>, 0> storage;
    State *Q0 = nullptr;
    bool isDFA = false;

public:
    NFA() = default;

    /// receives Q0 state — the start state of the finite automoton
    const State *getStartState() const { return Q0; }

    void parseRegex(const char *str);
    NFA buildDFA() const;

private:
    NFA(const NFA &) = delete;
    NFA &operator=(const NFA &) = delete;
    NFA(NFA &&) = default;
    NFA &operator=(NFA &&) = default;

    State *makeState(bool isTerminal = false);
};

#endif // DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

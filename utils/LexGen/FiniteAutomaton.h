///
/// \file
/// This file contains the declaration of structure parts of graph representation of NFA: states and
/// edges, and the wrapper for theam — the NFA class.
///

#ifndef DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H
#define DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <limits>
#include <map>

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


/// The NFA class is a graph representation of epsilon-NFA.
///
/// It can build new graph DFA, that is contained inside new instance of the NFA class. That new DFA
/// can be converted into C-implementation of the delta-function which is used in the dziejaLex
/// library.
/// 
/// The start state of the NFA is built by default. New states are built with method parseRegex, and
/// are joined to the start state with epsilone-edge.
class NFA {
    llvm::SmallVector<std::unique_ptr<State>, 1> storage;
    State *Q0;
    bool isDFA = false;

public:
    NFA() : Q0(makeState()) {}

    /// receives Q0 state — the start state of the finite automoton
    const State *getStartState() const { return Q0; }

    void parseRegex(const char *str);

    /// Builts new NFA instance that meets the DFA requirements
    NFA buildDFA() const;

private:
    NFA(const NFA &) = delete;
    NFA &operator=(const NFA &) = delete;
    NFA(NFA &&) = default;
    NFA &operator=(NFA &&) = default;

    State *makeState(bool isTerminal = false);
};

#endif // DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

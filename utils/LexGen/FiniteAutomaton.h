//-----------------------------------------------------------------------------------*- C++ -*----//
///
/// This file contains the declaration of structure parts of graph representation of NFA: states and
/// edges, and the wrapper for theam — the NFA class.
///
/// Recommended references:
/// - [1] Andrew W. Appel. Modern Compiler Implementation in C (good for begginers)
/// - [2] John E. Hopcroft, Rajeev Motwani, Jeffrey D. Ullman. Introduction to Automata Theory,
/// Languages, and Computation. (has more details)
///
//------------------------------------------------------------------------------------------------//

#ifndef DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H
#define DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <limits>
#include <map>
#include <memory>

class State;

using Symbol = int; /// it is enough 8 bits for symbols, but to express \c epsilon, we use \c int
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


/// State for epsilone-NFA.
///
/// It means that State instance can have some edges for every symbol includeing \c epsilon.
class State {
    llvm::SmallVector<Edge, 0> edges;
    bool terminal;

public:
    State(bool terminal) : terminal(terminal) {}

    bool isTerminal() const { return terminal; }
    void setTerminal(bool value = true) { terminal = value; }
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

    /// The method interprete any special character as usual, i.e. it builds simple chain of states.
    /// From "for" we get (1) --f-> (2) --o-> (3) --r-> (4) where (4) is terminal state.
    void parseRawString(const char *str);
    void parseRegex(const char *regex);

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

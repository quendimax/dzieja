//-----------------------------------------------------------------------------------*- C++ -*----//
///
/// \file
/// This file contains the declaration of structure parts of graph representation of NFA: states and
/// edges, and the wrapper for theam — the NFA class.
///
/// Recommended references:
/// - [1] Andrew W. Appel. Modern Compiler Implementation in C. (good for begginers)
/// - [2] John E. Hopcroft, Rajeev Motwani, Jeffrey D. Ullman. Introduction to Automata Theory,
/// Languages, and Computation. (has more details)
///
//------------------------------------------------------------------------------------------------//

#ifndef DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H
#define DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

#include "dzieja/Basic/TokenKinds.h"

#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <limits>
#include <memory>
#include <set>


class State;

using StateID = unsigned int;

/// it is enough 8 bits for a symbols, but to express \c Epsilon, we use \c int.
using Symbol = unsigned int;

using StateSet = std::set<const State *>;


class Edge {
    const State *target;
    Symbol symbol;

public:
    enum : Symbol { Epsilon = std::numeric_limits<Symbol>::max() };

    Edge(Symbol symbol, const State *target = nullptr) : target(target), symbol(symbol)
    {
        assert((symbol == (unsigned char)symbol || symbol == Epsilon)
               && "the symbol must be either a char instance or the Epsilon");
    }
    bool isEpsilon() const { return symbol == Epsilon; }
    Symbol getSymbol() const { return symbol; }
    const State *getTarget() const { return target; }
};


/// State for epsilone-NFA.
///
/// As eNFA state the \p State can have multiple edges for every symbol including \c Epsilon.
///
/// \p kind is a marker of if the state is terminal. Non \c tok::unknown kind means that the state
/// is terminal.
class State {
    llvm::SmallVector<Edge, 0> edges;
    StateID id;
    dzieja::tok::TokenKind kind;

public:
    State(StateID id, dzieja::tok::TokenKind kind = dzieja::tok::unknown) : id(id), kind(kind) {}

    StateID getID() const { return id; }
    bool isTerminal() const { return kind != dzieja::tok::unknown; }
    dzieja::tok::TokenKind getKind() const { return kind; }
    void setKind(dzieja::tok::TokenKind kind) { this->kind = kind; }
    const llvm::SmallVectorImpl<Edge> &getEdges() const { return edges; }
    void connectTo(const State *state, Symbol symbol) { edges.push_back(Edge(symbol, state)); }
    StateSet getEspClosure() const;

private:
    State(const State &) = delete;
    State &operator=(const State &) = delete;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &out, const State &state);


/// The NFA class is a graph representation of epsilon-NFA.
///
/// It can build new graph DFA, that is contained inside new instance of the NFA class. That new DFA
/// can be converted into C-implementation of the delta-function which is used in the \c dziejaLex
/// library.
///
/// The start state of the NFA is built by default. New states are built with methods \p parseRegex
/// and \p parseRawString, and are joined to the start state with epsilone-edge. For instance,
/// from two keywords \c for and \c free we will get the following NFA graph:
/// \code
///       Eps->(2)-'f'->(3)-'o'->(4)-'r'->[5]
///      /
///   (1)-Eps->(6)-'f'->(7)-'r'->(8)-'e'->(9)-'e'->[10]
/// \endcode
/// where (n) ‐ usual state, [n] ‐ terminal state.
///
/// After building of eNFA, you can create new \p NFA meeting DFA requirenments and then generate
/// the DFA implementation via cpp-functions in a source file using \p generateCppImpl method.
class NFA {
    llvm::SmallVector<std::unique_ptr<State>, 0> storage;
    State *Q0;
    bool isDFA = false;

public:
    NFA() : Q0(makeState()) {}
    NFA(NFA &&) = default;
    NFA &operator=(NFA &&) = default;

    /// Receives Q0 state — the start state of the finite automaton.
    const State *getStartState() const { return Q0; }

    size_t getNumStates() const { return storage.size(); }

    /// Builds an NFA-graph from raw string without interpreting special characters.
    ///
    /// It interprets any special character as usual one, i.e. it builds a simple chain of states.
    /// For example, from string \c "f\r" we'll get the following states chain:
    /// \code
    ///   (1)-'f'->(2)-'\'->(3)-'r'->[4]
    /// \endcode
    /// where [4] is terminal state.
    void parseRawString(const char *str, dzieja::tok::TokenKind kind);
    void parseRegex(const char *regex, dzieja::tok::TokenKind kind);

    /// Builds new NFA instance that meets the DFA requirements.
    ///
    /// Every state of new DNA can't have edges with indentical symbols.
    NFA buildDFA() const;

    /// Generates '\p filename' source file which contains transitive funciton and terminal
    /// function in order to pass through the \c NFA.
    bool generateCppImpl(llvm::StringRef filename) const;

    void print(llvm::raw_ostream &) const;

private:
    NFA(const NFA &) = delete;
    NFA &operator=(const NFA &) = delete;

    State *makeState(dzieja::tok::TokenKind kind = dzieja::tok::unknown);


    enum { TransTableRowSize = 128 }; // now ASCII char is supported only
    using TransitiveTable = llvm::SmallVector<llvm::SmallVector<StateID, TransTableRowSize>, 0>;

    TransitiveTable buildTransitiveTable() const;
    void printTransitiveTable(const TransitiveTable &, llvm::raw_ostream &, int indent = 0) const;
    void printKindTable(llvm::raw_ostream &, int indent = 0) const;

    void printHeadComment(llvm::raw_ostream &) const;
    /// Prints transitive function implemented via transitive table.
    void printTransTableFunction(llvm::raw_ostream &) const;
    void printTerminalFunction(llvm::raw_ostream &) const;
};


#endif // DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

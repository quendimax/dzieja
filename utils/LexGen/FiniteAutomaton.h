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
#include <utility>


namespace dzieja {

class State;

using StateID = unsigned int;

/// it is enough 8 bits for a symbols, but to express \c Epsilon, we use \c int.
using Symbol = unsigned int;

enum : Symbol { Epsilon = std::numeric_limits<Symbol>::max() };

using StateSet = std::set<const State *>;


class Edge {
    const State *target;
    Symbol symbol;

public:
    Edge(Symbol symbol, const State *target = nullptr) : target(target), symbol(symbol)
    {
        assert((symbol == (0x7f & symbol) || symbol == Epsilon)
               && "the symbol must be either an ASCII character or the Epsilon");
    }

    /// This constructor needs in order to get correct converting from signed char to \p Symbol.
    Edge(char symbol, const State *target = nullptr) : Edge((Symbol)(unsigned char)symbol, target)
    {
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
    tok::TokenKind kind;

public:
    State(StateID id, tok::TokenKind kind = tok::unknown) : id(id), kind(kind) {}

    StateID getID() const { return id; }
    bool isTerminal() const { return kind != tok::unknown; }
    tok::TokenKind getKind() const { return kind; }
    void setKind(tok::TokenKind kind) { this->kind = kind; }
    const llvm::SmallVectorImpl<Edge> &getEdges() const { return edges; }
    void connectTo(const State *state, Symbol symbol) { edges.push_back(Edge(symbol, state)); }
    void connectTo(const State *state, char symbol) { edges.push_back(Edge(symbol, state)); }
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
    /// Specifies the mode of transitive function implementation.
    enum GeneratingMode {
        GM_Table, /// Generate delta-func via transitive table
        GM_Switch /// Generate delta-func via switch-case control flow
    };

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
    void parseRawString(const char *str, tok::TokenKind kind);

    /// Builds an NFE-graph from the regular excpression subset.
    ///
    ///
    void parseRegex(const char *regex, tok::TokenKind kind);

private:
    /// Specifies start and last (quasi-terminal) state of the part of an NFA
    using SubAutomaton = std::pair<State *, State *>;

    SubAutomaton parseSequence(const char *&expr);
    char parseSymbolCodePoint(const char *&expr);
    SubAutomaton parseSymbol(const char *&expr);
    SubAutomaton parseSquareSymbol(const char *&expr);
    SubAutomaton parseParen(const char *&expr);
    SubAutomaton parseSquare(const char *&expr);
    SubAutomaton parseQualifier(const char *&expr, SubAutomaton);
    SubAutomaton parseQuestion(const char *&expr, SubAutomaton);
    SubAutomaton parseStar(const char *&expr, SubAutomaton);
    SubAutomaton parsePlus(const char *&expr, SubAutomaton);

    SubAutomaton cloneSubAutomaton(SubAutomaton autom);

public:
    /// Builds new NFA instance that meets the DFA requirements.
    ///
    /// Every state of new DNA can't have edges with indentical symbols.
    NFA buildDFA() const;

    /// Generates '\p filename' source file which contains transitive funciton and terminal
    /// function in order to pass through the \c NFA.
    bool generateCppImpl(llvm::StringRef filename, GeneratingMode mode) const;

    void print(llvm::raw_ostream &) const;

private:
    NFA(const NFA &) = delete;
    NFA &operator=(const NFA &) = delete;

    State *makeState(tok::TokenKind kind = tok::unknown);

    enum { TransTableRowSize = 128 }; // now ASCII char is supported only
    using TransitiveTable = llvm::SmallVector<llvm::SmallVector<StateID, TransTableRowSize>, 0>;

    TransitiveTable buildTransitiveTable() const;
    void printTransitiveTable(const TransitiveTable &, llvm::raw_ostream &, int indent = 0) const;
    void printKindTable(llvm::raw_ostream &, int indent = 0) const;

    void printHeadComment(llvm::raw_ostream &, llvm::StringRef end = "") const;
    void printInvalidStateConstant(llvm::raw_ostream &, llvm::StringRef end = "") const;

    /// Prints transitive function implemented via transitive table.
    void printTransTableFunction(llvm::raw_ostream &, llvm::StringRef end = "") const;

    /// Prints transitive function implemented via transitive table.
    void printTransSwitchFunction(llvm::raw_ostream &, llvm::StringRef end = "") const;

    /// Prints function returning TokenKind of given state.
    ///
    /// If the kind is \c tok::unknown it means that the state is not terminal, otherwise it is
    /// terminal and marks the end of the parsed token.
    void printTerminalFunction(llvm::raw_ostream &, llvm::StringRef end = "") const;
};

} // namespace dzieja


#endif // DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

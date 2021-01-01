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

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ConvertUTF.h>
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

constexpr Symbol Epsilon = std::numeric_limits<Symbol>::max();
constexpr Symbol MaxSymbolValue = std::numeric_limits<unsigned char>::max();

using StateSet = std::set<const State *>;


class Edge {
    const State *Target;
    Symbol Sym;

public:
    Edge(Symbol symbol, const State *target = nullptr) : Target(target), Sym(symbol)
    {
        assert((symbol == (MaxSymbolValue & symbol) || symbol == Epsilon)
               && "the symbol must be either an ASCII character or the Epsilon");
    }

    /// This constructor needs in order to get correct converting from signed char to \p Symbol.
    Edge(char symbol, const State *target = nullptr) : Edge((Symbol)(unsigned char)symbol, target)
    {
    }

    bool isEpsilon() const { return Sym == Epsilon; }
    Symbol getSymbol() const { return Sym; }
    const State *getTarget() const { return Target; }
};


/// State for epsilone-NFA.
///
/// As eNFA state the \p State can have multiple edges for every symbol including \c Epsilon.
///
/// \p kind is a marker of if the state is terminal. Non \c tok::unknown kind means that the state
/// is terminal.
class State {
    llvm::SmallVector<Edge, 0> Edges;
    StateID ID;
    tok::TokenKind Kind;

public:
    State(StateID id, tok::TokenKind kind = tok::unknown) : ID(id), Kind(kind) {}

    StateID getID() const { return ID; }
    bool isTerminal() const { return Kind != tok::unknown; }
    tok::TokenKind getKind() const { return Kind; }
    void setKind(tok::TokenKind kind) { Kind = kind; }
    const llvm::SmallVectorImpl<Edge> &getEdges() const { return Edges; }
    void connectTo(const State *state, Symbol symbol) { Edges.push_back(Edge(symbol, state)); }
    void connectTo(const State *state, char symbol) { Edges.push_back(Edge(symbol, state)); }
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
    llvm::SmallVector<std::unique_ptr<State>, 0> Storage;
    State *Q0;
    bool IsDFA = false;

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

    size_t getNumStates() const { return Storage.size(); }

    /// Builds an NFA-graph from a raw string without interpreting special characters.
    void parseRawString(const char *str, tok::TokenKind kind);

    /// Builds an NFA-graph from the regular excpression subset.
    ///
    /// For detailed description look at `utils/LexGen/README.md`.
    void parseRegex(const char *regex, tok::TokenKind kind);

private:
    /// Specifies start and last (quasi-terminal) state of the part of an NFA
    using SubAutomaton = std::pair<State *, State *>;

    SubAutomaton parseSequence(const char *&expr);
    SubAutomaton makeSubAutomFromCodePoint(llvm::UTF32 codePoint, SubAutomaton);
    SubAutomaton parseSymbol(const char *&expr);
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

    /// Builds new NFA instance that meets the minimized DFA requirements.
    NFA buildMinimizedDFA() const;

    /// Generates '\p filename' source file which contains transitive funciton and terminal
    /// function in order to pass through the \c NFA.
    bool generateCppImpl(llvm::StringRef filename, GeneratingMode mode) const;

    llvm::raw_ostream &print(llvm::raw_ostream &) const;

private:
    NFA(const NFA &) = delete;
    NFA &operator=(const NFA &) = delete;

    State *makeState(tok::TokenKind kind = tok::unknown);

    /// Builds distinguishable/equivalent table. It works for DFA only!
    llvm::SmallVector<llvm::BitVector, 0> buildDistinguishTable(bool distinguishKinds) const;

    /// Prints the distinguishable table. It's used as debug information only.
    void dumpDistinguishTable(const llvm::SmallVector<llvm::BitVector, 0> &distingTable,
                              llvm::raw_ostream &out) const;

    enum { TransTableRowSize = 256 };
    using TransitiveTable = llvm::SmallVector<llvm::SmallVector<StateID, TransTableRowSize>, 0>;

    TransitiveTable buildTransitiveTable() const;
    TransitiveTable buildReverseTransitiveTable() const;
    void printTransitiveTable(const TransitiveTable &, llvm::raw_ostream &, int indent = 0) const;
    void printKindTable(llvm::raw_ostream &, int indent = 0) const;

    void printHeadComment(llvm::raw_ostream &, llvm::StringRef end = "") const;
    void printInvalidStateConstant(llvm::raw_ostream &, llvm::StringRef end = "") const;

    /// Prints transitive function implemented via transitive table.
    void printTransTableFunction(llvm::raw_ostream &, llvm::StringRef end = "") const;

    /// Prints transitive function implemented via switch control flow.
    void printTransSwitchFunction(llvm::raw_ostream &, llvm::StringRef end = "") const;

    /// Prints function returning TokenKind of given state.
    ///
    /// If the kind is \c tok::unknown it means that the state is not terminal, otherwise it is
    /// terminal and marks the end of the parsed token.
    void printTerminalFunction(llvm::raw_ostream &, llvm::StringRef end = "") const;
};

} // namespace dzieja


#endif // DZIEJA_UTILS_LEXGEN_FINITEAUTOMATON_H

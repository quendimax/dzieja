#include "FiniteAutomaton.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <functional>
#include <map>
#include <memory>


using namespace llvm;
using namespace std;

namespace dzieja {

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

raw_ostream &operator<<(raw_ostream &out, const State &state)
{
    out << "State";
    out << (state.isTerminal() ? "[" : "(");
    out << state.getID();
    out << (state.isTerminal() ? "]" : ")");
    return out;
}

State *NFA::makeState(tok::TokenKind kind)
{
    auto newState = new State((StateID)storage.size(), kind);
    storage.push_back(unique_ptr<State>(newState));
    return storage.back().get();
}

void NFA::parseRawString(const char *str, tok::TokenKind kind)
{
    auto curState = makeState();
    Q0->connectTo(curState, Edge::Epsilon);
    for (; *str; str++) {
        auto newState = makeState();
        curState->connectTo(newState, *str);
        curState = newState;
    }
    curState->setKind(kind);
}

void NFA::parseRegex(const char *regex, tok::TokenKind kind)
{
    // TODO: add parsing of regex
}

/// This function looks for new state set, an equivalent of the next DNA state for the edge \p
/// symbol.
static StateSet findDFAState(const StateSet &states, Symbol symbol)
{
    StateSet newStates;
    for (const auto &state : states) {
        for (const auto &edge : state->getEdges()) {
            if (edge.getSymbol() == symbol) {
                auto closure = edge.getTarget()->getEspClosure();
                newStates.insert(closure.begin(), closure.end());
            }
        }
    }
    return newStates;
}

NFA NFA::buildDFA() const
{
    map<const StateSet, State *> convTable;
    NFA dfa;
    dfa.storage.pop_back(); // by default NFA contains the start state, but here we don't need it
    StateSet setQ0 = getStartState()->getEspClosure();
    function<State *(const StateSet &set)> convert;

    convert = [&](const StateSet &set) {
        auto iter = convTable.find(set);
        if (iter != convTable.end())
            return iter->second;

        auto newState = dfa.makeState();
        StateID minID = this->getNumStates();
        for (const auto state : set) {
            if (state->isTerminal() && state->getID() < minID) {
                // lesser ID means that the state was defined earlier and has higher priority
                minID = state->getID();
                newState->setKind(state->getKind());
            }
        }
        convTable[set] = newState;

        SmallSet<Symbol, 8> symbols;
        for (auto state : set)
            for (const Edge &edge : state->getEdges())
                if (!edge.isEpsilon())
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

bool NFA::generateCppImpl(StringRef filename) const
{
    if (!isDFA) {
        WithColor::error() << "you are trying generate trasitive table for non DNA";
        return false;
    }
    if (storage.size() > std::numeric_limits<unsigned short>::max()) {
        WithColor::error() << "number of state is too big for `unsigned short` cell of table";
        return false;
    }
    error_code EC;
    raw_fd_ostream out(filename, EC);
    if (EC) {
        WithColor::error() << EC.message() << "\n";
        return false;
    }

    printHeadComment(out, "\n");
    printInvalidStateConstant(out, "\n\n");
    printTransTableFunction(out, "\n\n");
    printTerminalFunction(out, "\n");

    return true;
}

void NFA::print(raw_ostream &out) const
{
    for (const auto &state : storage) {
        out << *state << "\n";
        for (const auto &edge : state->getEdges()) {
            out << " |- ";
            if (edge.isEpsilon())
                out << "Eps";
            else
                out << (char)edge.getSymbol();
            out << " - " << *edge.getTarget() << "\n";
        }
    }
}

NFA::TransitiveTable NFA::buildTransitiveTable() const
{
    assert(isDFA && "It's expected that the NFA meets DNA requirements");

    TransitiveTable transTable;
    transTable.resize(storage.size());

    const size_t INVALID_ID = storage.size();
    for (auto &row : transTable)
        row.resize(TransTableRowSize, INVALID_ID);

    for (StateID id = 0; id < storage.size(); id++) {
        const auto *state = storage[id].get();
        for (const auto &edge : state->getEdges()) {
            transTable[id][edge.getSymbol()] = edge.getTarget()->getID();
        }
    }
    return transTable;
}

/// Returns type required for containing of \p size states plus invalid one.
static const char *getTypeBySize(size_t size)
{
    if (size <= 0xffu)
        return "uint8_t";
    if (size <= 0xffffu)
        return "uint16_t";
    if (size <= 0xffffffffu)
        return "uint32_t";
    llvm_unreachable("Number of states is too big. Now only uint32_t is supported");
}

void NFA::printTransitiveTable(const TransitiveTable &table, raw_ostream &out, int indent) const
{
    SmallString<16> indention;
    for (int i = 0; i < indent; i++)
        indention += ' ';

    const char *typeStr = getTypeBySize(storage.size());
    out << indention << "static const " << typeStr;
    out << " TransitiveTable[" << storage.size() << "][" << TransTableRowSize << "] = {\n";
    for (size_t i = 0; i < table.size(); i++) {
        const auto &row = table[i];
        out << indention << "    {";
        for (size_t j = 0; j < row.size(); j++)
            out << row[j] << "u" << (j + 1 == row.size() ? "" : ", ");
        out << "}" << (i + 1 == table.size() ? "\n" : ",\n");
    }
    out << indention << "};\n";
}

void NFA::printKindTable(raw_ostream &out, int indent) const
{
    SmallString<16> indention;
    for (int i = 0; i < indent; i++)
        indention += ' ';

    out << indention << "static const unsigned short KindTable[" << storage.size() << "] = {\n";
    out << indention << "    ";
    for (size_t i = 0; i < storage.size(); i++) {
        const auto *state = storage[i].get();
        out << state->getKind() << "u";
        out << (i + 1 == storage.size() ? "\n" : ", ");
    }
    out << indention << "};\n";
}

void NFA::printHeadComment(raw_ostream &out, llvm::StringRef end) const
{
    out << "//\n";
    out << "// This file is automatically generated with the lexgen tool.\n";
    out << "// So, don't change this file because it will be automatically updated.\n";
    out << "// For more details look at 'dzieja/Basic/TokenKind.def' file.\n";
    out << "//\n";
    out << "// !WARNING: In order to use generated code include <stdint.h> header before!\n";
    out << "//\n" << end;
}

void NFA::printInvalidStateConstant(llvm::raw_ostream &out, llvm::StringRef end) const
{
    out << "enum { DFA_InvalidStateID = " << storage.size() << "u };";
    out << end;
}

void NFA::printTransTableFunction(raw_ostream &out, llvm::StringRef end) const
{
    const char *typeStr = getTypeBySize(storage.size());
    out << "static inline " << typeStr << " DFA_delta(unsigned stateID, unsigned symbol)\n";
    out << "{\n";
    auto transTable = buildTransitiveTable();
    printTransitiveTable(transTable, out, 4);
    out << "    return TransitiveTable[stateID][symbol];\n";
    out << "}" << end;
}

void NFA::printTerminalFunction(raw_ostream &out, llvm::StringRef end) const
{
    out << "static inline unsigned short DFA_getKind(unsigned stateID)\n";
    out << "{\n";
    printKindTable(out, 4);
    out << "    return KindTable[stateID];\n";
    out << "}" << end;
}

} // namespace dzieja

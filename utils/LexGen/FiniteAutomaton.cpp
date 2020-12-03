#include "FiniteAutomaton.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <map>
#include <memory>


using namespace llvm;
using namespace std;

namespace dzieja {

StateSet State::getEspClosure() const
{
    StateSet closure;
    auto finder = [&closure](const State *state) {
        auto finderImpl = [&closure](const State *state, auto &finderRef) {
            if (is_contained(closure, state))
                return;
            closure.insert(state);
            for (const Edge &edge : state->getEdges())
                if (edge.getSymbol() == Edge::Epsilon)
                    finderRef(edge.getTarget(), finderRef);
        };
        finderImpl(state, finderImpl);
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
    auto *newState = new State((StateID)storage.size(), kind);
    storage.push_back(unique_ptr<State>(newState));
    return storage.back().get();
}

void NFA::parseRawString(const char *str, tok::TokenKind kind)
{
    auto *curState = Q0;
    for (; *str; str++) {
        auto *newState = makeState();
        curState->connectTo(newState, *str);
        curState = newState;
    }
    curState->setKind(kind);
}

void NFA::parseRegex(const char *expr, tok::TokenKind kind)
{
    SubAutomaton sub = parseSequence(expr);
    Q0->connectTo(sub.first, Edge::Epsilon);
    sub.second->setKind(kind);
    if (*expr == ')')
        llvm::report_fatal_error("unexpected close paren ')' without previous open one");

    assert(*expr == '\0' && "parsing must be finished with zero character");
}

NFA::SubAutomaton NFA::parseSequence(const char *&expr)
{
    auto *firstState = makeState();
    auto *lastState = firstState;
    SubAutomaton curAutom;
    SmallVector<State *, 8> lastStates;
    for (;;) {
        switch (*expr) {
        case '\0':
        case ')':
            lastStates.push_back(lastState);
            goto finish;
        case '|':
            lastStates.push_back(lastState);
            lastState = firstState;
            ++expr;
            continue;
        case '(':
            curAutom = parseParen(expr);
            break;
        default:
            curAutom = parseSymbol(expr);
            break;
        }
        curAutom = parseQualifier(expr, curAutom);
        lastState->connectTo(curAutom.first, Edge::Epsilon);
        lastState = curAutom.second;
    }
finish:
    auto *finishState = makeState();
    for (auto *state : lastStates)
        state->connectTo(finishState, Edge::Epsilon);
    return {firstState, finishState};
}

NFA::SubAutomaton NFA::parseSymbol(const char *&expr)
{
    char c = *expr;
    if (*expr == '\\') {
        ++expr;
        switch (*expr) {
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;
        case '0':
            c = '\0';
            break;
        // clang-format off
        case '(': case ')': case '[': case ']': case '-': case '\\':
        case '^': case '|': case '+': case '*': case '?':
            c = *expr;
            break;
        // clang-format on
        default:
            SmallString<40> errorMsg;
            errorMsg += "unsupported escaped character '\\";
            errorMsg += *expr;
            errorMsg += "'";
            llvm::report_fatal_error(errorMsg);
        }
    }
    auto *first = makeState();
    auto *last = makeState();
    first->connectTo(last, c);
    ++expr; // set the pointor to the next symbol after the processed one
    return {first, last};
}

NFA::SubAutomaton NFA::parseParen(const char *&expr)
{
    assert(*expr == '(' && "open paren '(' is exptected");
    ++expr;
    auto autom = parseSequence(expr);
    if (*expr != ')')
        llvm::report_fatal_error("close paren ')' is expected!");
    ++expr;
    return autom;
}

NFA::SubAutomaton NFA::parseQualifier(const char *&expr, SubAutomaton autom)
{
    switch (*expr) {
    case '?':
        autom = parseQuestion(expr, autom);
        break;
    case '*':
        autom = parseStar(expr, autom);
        break;
    case '+':
        autom = parsePlus(expr, autom);
        break;
    default:
        return autom;
    }
    if (*expr == '?' || *expr == '*' || *expr == '+')
        llvm::report_fatal_error("qualifier mustn't be after another qualifier");
    return autom;
}

NFA::SubAutomaton NFA::parseQuestion(const char *&expr, SubAutomaton autom)
{
    autom.first->connectTo(autom.second, Edge::Epsilon);
    ++expr;
    return autom;
}

NFA::SubAutomaton NFA::parseStar(const char *&expr, SubAutomaton autom)
{
    auto *startState = makeState();
    auto *lastState = makeState();
    startState->connectTo(lastState, Edge::Epsilon);
    autom.second->connectTo(lastState, Edge::Epsilon);
    lastState->connectTo(autom.first, Edge::Epsilon);
    ++expr;
    return {startState, lastState};
}

NFA::SubAutomaton NFA::parsePlus(const char *&expr, SubAutomaton autom)
{
    ++expr;
    return autom;
}

NFA::SubAutomaton NFA::cloneSubAutomaton(SubAutomaton autom)
{
    return autom;
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

    auto convert = [&](const StateSet &set) {
        auto convertImpl = [&](const StateSet &set, auto &convertRef) {
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
                auto targetState = convertRef(targetSet, convertRef);
                newState->connectTo(targetState, symbol);
            }
            return newState;
        };
        return convertImpl(set, convertImpl);
    };
    dfa.Q0 = convert(setQ0);
    dfa.isDFA = true;
    return dfa;
}

bool NFA::generateCppImpl(StringRef filename, NFA::GeneratingMode mode) const
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
    if (mode == GM_Table)
        printTransTableFunction(out, "\n\n");
    else if (mode == GM_Switch)
        printTransSwitchFunction(out, "\n\n");
    else
        llvm_unreachable("Unknown mode of transitive function generating.");
    printTerminalFunction(out, "\n");

    return true;
}

void NFA::print(raw_ostream &out) const
{
    for (const auto &state : storage) {
        out << *state << "\n";
        for (const auto &edge : state->getEdges()) {
            out << " |- '";
            if (edge.isEpsilon()) {
                out << "Eps";
            }
            else {
                char c = edge.getSymbol();
                out.write_escaped(StringRef(&c, 1));
            }
            out << "' - " << *edge.getTarget() << "\n";
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

    const char *typeStr = getTypeBySize(table.size());
    out << indention << "static const " << typeStr;
    out << " const TransitiveTable[" << table.size() << "][" << TransTableRowSize << "] = {\n";
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

    out << indention << "static const unsigned short const KindTable[";
    out << storage.size() << "] = {\n";
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
    out << "//\n"
           "// This file is automatically generated with the lexgen tool.\n"
           "// So, don't change this file because it will be automatically updated.\n"
           "// For more details look at 'dzieja/Basic/TokenKind.def' file.\n"
           "//\n"
           "// * NOTE: in order to use this generated code include "
           "<stdint.h> and <assert.h> headers before!\n"
           "//\n";
    out << end;
}

void NFA::printInvalidStateConstant(llvm::raw_ostream &out, llvm::StringRef end) const
{
    out << "enum { DFA_InvalidStateID = " << storage.size() << "u };";
    out << end;
}

void NFA::printTransTableFunction(raw_ostream &out, llvm::StringRef end) const
{
    out << "static inline unsigned DFA_delta(unsigned stateID, char symbol)\n";
    out << "{\n";
    auto transTable = buildTransitiveTable();
    printTransitiveTable(transTable, out, 4);
    out << "    return TransitiveTable[stateID][(unsigned char)symbol];\n";
    out << "}" << end;
}

void NFA::printTransSwitchFunction(llvm::raw_ostream &out, llvm::StringRef end) const
{
    out << "static inline unsigned short DFA_delta(unsigned stateID, char symbol)\n";
    out << "{\n";
    out << "    unsigned char usymbol = symbol;\n";
    out << "    switch (stateID) {\n";
    auto transTable = buildTransitiveTable();
    const size_t InvalidID = storage.size();
    for (size_t id = 0; id < transTable.size(); ++id) {
        out << "    case " << id << "u:\n";
        out << "        switch (usymbol) {\n";
        const auto &row = transTable[id];
        for (unsigned ch = 0; ch < row.size(); ++ch)
            if (row[ch] != InvalidID)
                out << "        case " << ch << "u: return " << row[ch] << "u;\n";
        out << "        default: return " << InvalidID << "u;\n";
        out << "        }\n";
    }
    out << "    default:\n";
    out << "        assert(0 && \"Unknown state ID is detected!\");\n";
    out << "    }\n";
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

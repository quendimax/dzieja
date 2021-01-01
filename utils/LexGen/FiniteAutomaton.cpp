#include "FiniteAutomaton.h"

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <cctype>
#include <map>

#define UNI_SUR_HIGH_START (UTF32)0xD800
#define UNI_SUR_LOW_END (UTF32)0xDFFF

using namespace llvm;
using namespace std;

namespace dzieja {

static auto &error()
{
    return WithColor::error(llvm::errs(), "dzieja-lexgen");
}

static bool areDistinguishable(const SmallVector<BitVector, 0> &table, StateID leftID,
                               StateID rightID)
{
    // the same states are distinguishable cause the table's initialized with false by default
    if (leftID <= rightID)
        return table[leftID][rightID];
    else
        return table[rightID][leftID];
}

StateSet State::getEspClosure() const
{
    StateSet closure;
    auto finder = [&closure](const State *state) {
        auto finderImpl = [&closure](const State *state, auto &finderRef) {
            if (is_contained(closure, state))
                return;
            closure.insert(state);
            for (const Edge &edge : state->getEdges())
                if (edge.getSymbol() == Epsilon)
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
    if (state.isTerminal())
        out << ":" << state.getKind();
    out << (state.isTerminal() ? "]" : ")");
    return out;
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
    Q0->connectTo(sub.first, Epsilon);
    sub.second->setKind(kind);
    if (*expr == ')') {
        error() << "unexpected close paren ')' without previous open one\n";
        std::exit(1);
    }
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
        case '[':
            curAutom = parseSquare(expr);
            break;
        default:
            curAutom = parseSymbol(expr);
            break;
        }
        curAutom = parseQualifier(expr, curAutom);
        lastState->connectTo(curAutom.first, Epsilon);
        lastState = curAutom.second;
    }
finish:
    auto *finishState = makeState();
    for (auto *state : lastStates)
        state->connectTo(finishState, Epsilon);
    return {firstState, finishState};
}

/// Parse an escape sequence specifying a unicode symbol.
///
/// The escape sequence has two forms: \uxxxx and \Uxxxxxx, where \c x is a hex digit. After parsing
/// \c expr points to the last character of parsed sequence (not after it!).
static UTF32 parseUnicodeEscape(const char *&expr)
{
    assert(*expr == 'u' || *expr == 'U');

    char marker = *expr;
    int numOfDigits = marker == 'u' ? 4 : 6;
    UTF32 codePoint = 0;
    for (int i = 0; i < numOfDigits; i++) {
        int c = (unsigned char)*++expr;
        if (std::isxdigit(c)) {
            codePoint <<= 4;
            if (std::isdigit(c))
                codePoint |= c - '0';
            else
                codePoint |= std::toupper(c) - 'A' + 10;
        }
        else {
            error() << "after \\" << marker << " " << numOfDigits << " hex digits are expected\n";
            std::exit(1);
        }
    }
    // I don't add ++expr because it is in outer parseSymbolCodePoint function.
    return codePoint;
}

/// Parse a symbol and returns its unicode point. It doesn't create \p SubAutomaton unlike
/// \p parseSymbol method.
static UTF32 parseSymbolCodePoint(const char *&expr)
{
    UTF32 codePoint;
    if (*expr & 0x80) { // is not an ASCII char
        unsigned numBytes = llvm::getNumBytesForUTF8((UTF8)*expr);
        const UTF8 **source = (const UTF8 **)&expr;
        auto result = convertUTF8Sequence(source, *source + numBytes, &codePoint, strictConversion);
        if (result != conversionOK) {
            auto &err = error() << "source sequence '";
            err.write_escaped(StringRef(expr, numBytes)) << "' is malformed UTF8 symbol\n";
            std::exit(1);
        }
    }
    else { // is an ASCII char
        codePoint = (unsigned char)*expr;
        if (*expr == '\\') {
            ++expr;
            switch (*expr) {
            case 'n':
                codePoint = (unsigned char)'\n';
                break;
            case 'r':
                codePoint = (unsigned char)'\r';
                break;
            case 't':
                codePoint = (unsigned char)'\t';
                break;
            case 'v':
                codePoint = (unsigned char)'\v';
                break;
            case '0':
                codePoint = (unsigned char)'\0';
                break;
            case 'u':
            case 'U':
                codePoint = parseUnicodeEscape(expr);
                break;
            // clang-format off
            case '(': case ')': case '[': case ']': case '-': case '\\':
            case '^': case '|': case '+': case '*': case '?':
                codePoint = (unsigned char)*expr;
                break;
            // clang-format on
            default:
                auto &err = error() << "unsupported escaped character '";
                err.write_escaped(StringRef(expr, 1), true) << "'\n";
                std::exit(1);
            }
        }
        ++expr;
    }
    return codePoint;
}

NFA::SubAutomaton NFA::makeSubAutomFromCodePoint(UTF32 codePoint)
{
    SmallString<UNI_MAX_UTF8_BYTES_PER_CODE_POINT> u8seq;
    u8seq.set_size(UNI_MAX_UTF8_BYTES_PER_CODE_POINT);
    char *ptr = u8seq.data();
    if (!ConvertCodePointToUTF8(codePoint, ptr)) {
        auto &err = error() << "can't convert code point ";
        err.write_hex(codePoint) << " into UTF8 sequence\n";
        std::exit(1);
    }
    unsigned size = ptr - u8seq.data();
    auto *first = makeState();
    auto *last = first;
    for (unsigned i = 0; i < size; i++) {
        auto *newOne = makeState();
        last->connectTo(newOne, u8seq[i]);
        last = newOne;
    }
    return {first, last};
}

NFA::SubAutomaton NFA::parseSymbol(const char *&expr)
{
    UTF32 codePoint = parseSymbolCodePoint(expr);
    return makeSubAutomFromCodePoint(codePoint);
}

NFA::SubAutomaton NFA::parseParen(const char *&expr)
{
    assert(*expr == '(' && "open paren '(' is exptected");
    ++expr;
    auto autom = parseSequence(expr);
    if (*expr != ')') {
        error() << "close paren ')' is expected!\n";
        std::exit(1);
    }
    ++expr;
    return autom;
}

NFA::SubAutomaton NFA::parseSquare(const char *&expr)
{
    assert(*expr == '[' && "open paren '[' is exptected");

    const char *startSource = ++expr;
    bool isNegative = false;
    if (*expr == '^') {
        isNegative = true;
        ++expr;
    }
    llvm::BitVector unicodeMarkers; // marks if an element must be include in the range of chars
    unicodeMarkers.resize(UNI_MAX_LEGAL_UTF32 + 1, isNegative);

    while (*expr != ']') {
        UTF32 firstPoint = parseSymbolCodePoint(expr);
        unicodeMarkers[firstPoint] = !isNegative;

        if (*expr == '-') {
            ++expr;
            UTF32 secondPoint = parseSymbolCodePoint(expr);
            if (secondPoint < firstPoint) {
                auto &err = error() << "character range in [";
                err << StringRef(startSource, expr - startSource) << "] is not consecutive\n";
                std::exit(1);
            }
            for (Symbol c = firstPoint; c <= secondPoint; c++)
                unicodeMarkers[c] = !isNegative;
        }
    }
    ++expr;

    auto *firstState = makeState();
    auto *lastState = makeState();
    for (UTF32 c = 0; c <= UNI_MAX_LEGAL_UTF32; c++)
        if ((c < UNI_SUR_HIGH_START || UNI_SUR_LOW_END < c) && unicodeMarkers[c]) {
            if (c > 127) {
                auto autom = makeSubAutomFromCodePoint(c);
                firstState->connectTo(autom.first, Epsilon);
                autom.second->connectTo(lastState, Epsilon);
            }
            else { // c is ASCII compatible
                firstState->connectTo(lastState, c);
            }
        }
    return {firstState, lastState};
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
    if (*expr == '?' || *expr == '*' || *expr == '+') {
        error() << "qualifier mustn't be after another qualifier\n";
        std::exit(1);
    }
    return autom;
}

NFA::SubAutomaton NFA::parseQuestion(const char *&expr, SubAutomaton autom)
{
    autom.first->connectTo(autom.second, Epsilon);
    ++expr;
    return autom;
}

NFA::SubAutomaton NFA::parseStar(const char *&expr, SubAutomaton autom)
{
    auto *startState = makeState();
    auto *lastState = makeState();
    startState->connectTo(lastState, Epsilon);
    autom.second->connectTo(lastState, Epsilon);
    lastState->connectTo(autom.first, Epsilon);
    ++expr;
    return {startState, lastState};
}

NFA::SubAutomaton NFA::parsePlus(const char *&expr, SubAutomaton autom)
{
    auto copyAutom = cloneSubAutomaton(autom);
    auto starAutom = parseStar(expr, copyAutom); // this method's already advanced expr
    starAutom.second->connectTo(copyAutom.first, Epsilon);
    return {starAutom.first, copyAutom.second};
}

NFA::SubAutomaton NFA::cloneSubAutomaton(SubAutomaton autom)
{
    DenseMap<const State *, State *> map;
    auto rec = [&map, this](const State *state) {
        auto recImpl = [&map, this](const State *state, auto &recRef) {
            if (map.find(state) != map.end())
                return;
            auto *newState = makeState();
            map[state] = newState;
            for (auto &edge : state->getEdges())
                recRef(edge.getTarget(), recRef);
        };
        recImpl(state, recImpl);
    };
    rec(autom.first);

    for (auto &item : map) {
        auto *origState = item.first;
        auto *newState = item.second;
        for (auto &edge : origState->getEdges()) {
            auto *newTargetState = map[edge.getTarget()];
            newState->connectTo(newTargetState, edge.getSymbol());
        }
    }
    return {map[autom.first], map[autom.second]};
}

/// This function looks for new state set, an equivalent of the next DFA state for the edge
/// \p symbol.
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
    std::map<const StateSet, State *> convTable;
    NFA dfa;
    dfa.Storage.pop_back(); // by default NFA contains the start state, but here we don't need it
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

            SmallSet<Symbol, 1> symbols; // in generated DFA there are many one-edge-states
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
    dfa.IsDFA = true;
    return dfa;
}

NFA NFA::buildMinimizedDFA() const
{
    if (!IsDFA) {
        error() << "can't build minimized DFA from NFA!\n";
        std::exit(1);
    }

    auto distingTable = buildDistinguishTable(/*distinguishKinds=*/true);

#define DEBUG_TYPE "disting-table"
    LLVM_DEBUG(dumpDistinguishTable(distingTable, llvm::errs()));
#undef DEBUG_TYPE

    NFA minDfa;
    minDfa.Storage.pop_back();

    // Build groups of states
    DenseMap<const State *, State *> old2new;
    DenseMap<State *, StateSet> new2old;
    BitVector checkedStates;
    checkedStates.resize(Storage.size(), false);
    for (StateID id = 0, e = Storage.size(); id < e; id++) {
        if (checkedStates[id])
            continue;

        StateSet group;
        group.insert(Storage[id].get());
        checkedStates[id] = true;
        for (StateID nextID = 0; nextID < e; nextID++) {
            if (checkedStates[nextID])
                continue;

            if (!areDistinguishable(distingTable, id, nextID)) {
                group.insert(Storage[nextID].get());
                checkedStates[nextID] = true;
            }
        }

        State *newState = minDfa.makeState();
        for (const auto *state : group) {
            // expected only one kind of all the group
            if (state->isTerminal())
                newState->setKind(state->getKind());
            old2new[state] = newState;
            new2old[newState] = group;
        }
    }

    // Build edges between new states
    for (auto &item : new2old) {
        State *newState = item.first;
        auto &group = item.second;

        BitVector addedSymbols;
        addedSymbols.resize(256, false);
        for (const auto *oldState : group) {
            for (const auto &edge : oldState->getEdges()) {
                if (addedSymbols[edge.getSymbol()])
                    break;

                const State *target = old2new[edge.getTarget()];
                newState->connectTo(target, edge.getSymbol());
                addedSymbols[edge.getSymbol()] = true;
            }
        }
    }

    return minDfa;
}

bool NFA::generateCppImpl(StringRef filename, NFA::GeneratingMode mode) const
{
    if (!IsDFA) {
        error() << "you are trying generate trasitive table for non DFA\n";
        return false;
    }
    if (Storage.size() > std::numeric_limits<unsigned short>::max()) {
        error() << "number of states is too big for `unsigned short` cell of table\n";
        return false;
    }
    error_code EC;
    raw_fd_ostream out(filename, EC);
    if (EC) {
        error() << EC.message() << "\n";
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

raw_ostream &NFA::print(raw_ostream &out) const
{
    for (const auto &state : Storage) {
        out << *state << "\n";
        for (const auto &edge : state->getEdges()) {
            out << " |- '";
            if (edge.isEpsilon()) {
                out << "Eps";
            }
            else {
                char c = edge.getSymbol();
                bool useHexEscapes = c & 0x80 ? false : true;
                out.write_escaped(StringRef(&c, 1), useHexEscapes);
            }
            out << "' - " << *edge.getTarget() << "\n";
        }
    }
    return out;
}

State *NFA::makeState(tok::TokenKind kind)
{
    auto *newState = new State((StateID)Storage.size(), kind);
    Storage.push_back(unique_ptr<State>(newState));
    return Storage.back().get();
}

SmallVector<BitVector, 0> NFA::buildDistinguishTable(bool distinguishKinds) const
{
    assert(IsDFA && "can't make equivalent table for non DFA");

    // equivalent table initialization
    SmallVector<BitVector, 0> distinTable;
    distinTable.resize(Storage.size());
    for (size_t i = 0, e = Storage.size(); i < e; i++)
        distinTable[i].resize(e, false);

    for (size_t i = 0, e = Storage.size(); i < e; i++) {
        for (size_t j = i + 1; j < e; j++) {
            auto *st1 = Storage[i].get();
            auto *st2 = Storage[j].get();
            if ((st1->isTerminal() && !st2->isTerminal())
                || (!st1->isTerminal() && st2->isTerminal())) {
                distinTable[i][j] = true;
            }
            else if (distinguishKinds) {
                if (st1->isTerminal() && st2->isTerminal() && st1->getKind() != st2->getKind())
                    // we need to distinguish key words and identifier for example
                    distinTable[i][j] = true;
            }
        }
    }

    const StateID InvalidID = Storage.size();
    auto reverseTable = buildReverseTransitiveTable();
    bool isUpdated;
    do {
        isUpdated = false;
        for (StateID i = 0, e = Storage.size(); i < e; i++) {
            for (StateID j = i + 1; j < e; j++) {
                if (distinTable[i][j])
                    continue;

                for (Symbol c = 0u; c <= MaxSymbolValue; c++) {
                    StateID nextState1 = reverseTable[i][c];
                    StateID nextState2 = reverseTable[j][c];
                    if (nextState1 == InvalidID && nextState2 == InvalidID)
                        continue;

                    if ((nextState1 != InvalidID && nextState2 == InvalidID)
                        || (nextState1 == InvalidID && nextState2 != InvalidID)
                        || areDistinguishable(distinTable, nextState1, nextState2)) {
                        distinTable[i][j] = true;
                        isUpdated = true;
                        break;
                    }
                }
            }
        }
    } while (isUpdated);

    return distinTable;
}

void NFA::dumpDistinguishTable(const SmallVector<BitVector, 0> &distingTable,
                               raw_ostream &out) const
{
    out << "Distiguish table:\n";
    for (size_t i = 0, e = Storage.size(); i < e; i++) {
        for (size_t j = 0; j < e; j++)
            if (i < j)
                out << (distingTable[i][j] ? "1" : ".") << (j + 1 == e ? "\n" : " ");
            else
                out << "  " << (j + 1 == e ? "\n" : "");
    }
}

NFA::TransitiveTable NFA::buildTransitiveTable() const
{
    assert(IsDFA && "It's expected that the NFA meets DFA requirements");

    TransitiveTable transTable;
    transTable.resize(Storage.size());

    const size_t INVALID_ID = Storage.size();
    for (auto &row : transTable)
        row.resize(TransTableRowSize, INVALID_ID);

    for (StateID id = 0; id < Storage.size(); id++) {
        const auto *state = Storage[id].get();
        for (const auto &edge : state->getEdges())
            transTable[id][edge.getSymbol()] = edge.getTarget()->getID();
    }
    return transTable;
}

NFA::TransitiveTable NFA::buildReverseTransitiveTable() const
{
    assert(IsDFA && "It's expected that the NFA meets DFA requirements");

    TransitiveTable table;
    table.resize(Storage.size());

    const size_t InvalidID = Storage.size();
    for (auto &row : table)
        row.resize(TransTableRowSize, InvalidID);

    for (StateID id = 0; id < Storage.size(); id++) {
        const auto *state = Storage[id].get();
        for (const auto &edge : state->getEdges())
            table[edge.getTarget()->getID()][edge.getSymbol()] = id;
    }
    return table;
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
    out << " TransitiveTable[" << table.size() << "][" << TransTableRowSize << "] = {\n";
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

    out << indention << "static const unsigned short KindTable[";
    out << Storage.size() << "] = {\n";
    out << indention << "    ";
    for (size_t i = 0; i < Storage.size(); i++) {
        const auto *state = Storage[i].get();
        out << state->getKind() << "u";
        out << (i + 1 == Storage.size() ? "\n" : ", ");
    }
    out << indention << "};\n";
}

void NFA::printHeadComment(raw_ostream &out, StringRef end) const
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

void NFA::printInvalidStateConstant(raw_ostream &out, StringRef end) const
{
    out << "enum {\n";
    out << "    DFA_StartStateID = " << Q0->getID() << "u,\n";
    out << "    DFA_InvalidStateID = " << Storage.size() << "u\n";
    out << "};";
    out << end;
}

void NFA::printTransTableFunction(raw_ostream &out, StringRef end) const
{
    out << "static inline unsigned DFA_delta(unsigned stateID, char symbol)\n";
    out << "{\n";
    auto transTable = buildTransitiveTable();
    printTransitiveTable(transTable, out, 4);
    out << "    return TransitiveTable[stateID][(unsigned char)symbol];\n";
    out << "}" << end;
}

void NFA::printTransSwitchFunction(raw_ostream &out, StringRef end) const
{
    out << "static inline unsigned DFA_delta(unsigned stateID, char symbol)\n";
    out << "{\n";
    out << "    unsigned char usymbol = symbol;\n\n";
    out << "#ifdef _MSC_VER\n";
    out << "#pragma warning(push)\n";
    // disable VS warning about a switch with the only default branch
    out << "#pragma warning(disable : 4065)\n";
    out << "#endif\n";
    out << "    switch (stateID) {\n";
    auto transTable = buildTransitiveTable();
    const size_t InvalidID = Storage.size();
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
    out << "#ifdef _MSC_VER\n";
    out << "#pragma warning(pop)\n";
    out << "#endif\n\n";
    out << "    return DFA_InvalidStateID;\n";
    out << "}" << end;
}

void NFA::printTerminalFunction(raw_ostream &out, StringRef end) const
{
    out << "static inline unsigned short DFA_getKind(unsigned stateID)\n";
    out << "{\n";
    printKindTable(out, 4);
    out << "    return KindTable[stateID];\n";
    out << "}" << end;
}

} // namespace dzieja

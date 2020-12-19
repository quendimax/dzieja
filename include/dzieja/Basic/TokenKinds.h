//-----------------------------------------------------------------------------------*- C++ -*----//
///
/// \file
/// The file contains token enumeration and auxilliary functions
///
//------------------------------------------------------------------------------------------------//

#ifndef DZIEJA_BASIC_TOKENKINDS_H
#define DZIEJA_BASIC_TOKENKINDS_H

namespace dzieja {

namespace tok {

enum TokenKind : unsigned short {
#define TOK(name) name,
#include "dzieja/Basic/TokenKinds.def"
    NUM_TOKENS
};

/// Returns name of token.
///
/// For an identifier returns identifier itself. For a keyword returns keyword without \c kw_
/// prefix. For a punctuator returns punctuator's name as its enum variable name.
const char *getTokenName(TokenKind kind);

} // namespace tok

} // namespace dzieja

#endif // DZIEJA_BASIC_TOKENKINDS_H

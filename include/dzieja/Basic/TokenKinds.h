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

} // namespace tok

} // namespace dzieja

#endif // DZIEJA_BASIC_TOKENKINDS_H

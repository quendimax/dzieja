# `dzieja-lexgen`

## Introduction

`dzieja-lexgen` is used for generating `.inc`-file containing an implementation
of DFA that `dziejaLex` library uses.

You can't change logic of generated DFA running `dzieja-lexgen`. It is
configured before build of the tool through specifying of
`include/Basic/TokenKinds.def` file. In the file you can set tokens via either a
raw string (with `TOKEN` macro) or a regular expression (with `TOKEN_REGEX`
macro).

## DFA Implementation

`dzieja-lexgen` generates DFA implementation in `.inc`-file by means of the
following functions and constants:

- `unsigned DFA_delta(unsigned statusID, char symbol)` is __δ__-function of
  being made DFA. It gets the current state id and input symbol, and returns the
  next state (if it exists) or the invalid state (look at `DFA_InvaidStateID`
  below).

- `unsigned short DFA_getKind(unsigned stateID)` returns a kind of token that
  corresponding the specified `stateID`. If the state is not terminal state, the
  function returns `tok::unknown` kind.

- `DFA_InvalidStateID` is ID of non-existent symbol.

- `DFA_StartStateID` speaks for itself ;)

`dzieja-lexgen` can generate a DFA in two different ways:

The first, activated with `-gen-via-table` option, is a table `NxM` where `N` is
number of states of the DFA, and `M` is number of possible symbols (in our case
`M` is 256).

The second, activated with `-gen-via-switch` option, is single outer
`switch-case` construction where every case corresponds every DFA state, and
that cases contain inner `switch-case`s which cases correspond every possible
symbol.

## Supported regular expression subset

`dzieja-lexgen` supports narrow subset of common used regex.

### Sequence of arbitrary characters

Characters laying in a row are translated into a chain of states with edges
corresponding symbols of the sequence.

For example, from string `bar` we'll get the following states chain:

    (1)-'b'->(2)-'a'->(3)-'r'->[4]

where the 4th state is terminal.

### Logical OR (`|`) in a sequence

If a sequence of characters is divided with `|` character, it means that a
parsed token can corresponding either the first part or the second one.

### Parentheses

A character sequence within parentheses is considered as a separate regex. It
means that following qualifiers (`*`, `+`, etc.) will affect to overall
sub-regex, but not to the last character only.

### Square brackets

Characters between `[` and `]` brackets specify a list of symbols that can be
corresponded to current input character. There you can specify a range of
characters by means of `-` symbol between the first and the last symbols in the
desirable range. For example, regex `[a-cABC]` corresponds to any of `a`, `b`,
`c`, `A`, `B`, `C`.

If just after open `[` the parser sees `^`, it will reverse list of acceptable
characters, i.e. a regex `[^abc]` will be interpreted as any character except of
`a`, `b`, and `c`.

### Qualifiers

A qualifier added after either a symbol or a sub-regex (formed with parentheses)
determines how many times that symbol/sub-regex can be repeated in input stream.

- `?` means the symbol/sub-regex can be present once or never.
- `*` means the symbol/sub-regex can be present any number of times including
  zero.
- `+` means the symbol/sub-regex can be present at least once.

### Escaped characters

The next escaped characters are supported:

- `\n` - Line Feed, New Line
- `\r` - Carriage Return
- `\t` - Horizontal Tabular
- `\v` - Vertical Tabular
- `\0` - Null
- `\]` - it makes sense only inside square brackets.
- `(`, `)`, `[`, `-`, `\`, `|`, `^`, `+`, `*`, `?`.

If just after open `[` the `^` character is set, it means that range of allowed
characters is inverted, i.e. on the current position can be any character except
of listed ones.

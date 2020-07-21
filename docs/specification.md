# Specification of Dzieja Programming Language

## 1. Terms and definitions

**argument**  
(function call expression) expression in the comma-separated list bounded by the
parentheses.

**implementation defined behavior**  
some behaviors that are not defined by this document, and they have to be
defined by implementation documents.

**loose type**  
type, which members are reference variables.

**reference variable**  
variable that contains of address to an object.

**signature**  
function name, parameter type list.

**solid type**  
type that hasn't got reference members.

## 2. Lexical conventions

### 2.1. Translation units

The text of the program is kept in _source files_. The text of every source file
must be encoded into UTF-8 format either with BOM or without. BOM, if it is, is
ignored, i.e. it is interpreted like white-space characters.

Characters are divided into:

- basic character set,
- white-space character set,
- unsupported character set.

The following characters belong to the _basic character set_:  
`a b c d e f g h i j k l m n o p q r s t u v w x y z`  
`A B C D E F G H I J K L M N O P Q R S T U V W X Y Z`  
`0 1 2 3 4 5 6 7 8 9`  
`_ { } [ ] ( ) ; : . * + - / = ,`

The characters belonging to _white-space character set_:

- Space, `U+0020`,
- LF, Line Feed, `U+000A`,
- CR, Carriage Return, `U+000D`,
- HT, Horizontal Tabulation, `U+0009`,
- VT, Vertical Tabulation, `U+000B`,
- Byte Order Mark in the beginning of a text, `U+FEFF`.

Any other characters belong to _unsupported character set_. What happens if that
kind of characters is encounters is implementation defined.

### 2.2. Translation phases

1. The source file is decomposed into _tokens_ and sequences of white-space
   characters.

End of line is Unicode point `U+000A` _New Line_ (resides in _Other, Control_
category).

---
title: Lexical structure
group: Language
order: 10
summary: Comments, identifiers, keywords, number literals and statement separators.
---

This page covers the tokens: comments, identifiers, keywords, number and
boolean literals, and statement separators. String literals have their own
page, [Strings](page:language/strings).

## Comments

`//` starts a comment that runs to the end of the line. `/* ... */` runs
until the next `*/`, across any number of lines. It does not nest: the first
`*/` closes the comment, even when an inner `/*` came after the outer one.

{{example:language/lexical-comments}}

## Identifiers and keywords

An identifier starts with a letter or `_`, followed by any number of
letters, digits or `_`. Quirrel is case sensitive, so `squad`, `Squad` and
`SQUAD` are three different names.

These words are reserved and cannot be used as an identifier. Each links to
the section that explains it.

{{keywords}}

Two entries in this list are special:

- `not` only appears directly before `in`, as the compound operator `x not
  in t`. There is no standalone `not expr`; write `!expr` for that.
- `clone`, `switch`, `case` and `default` are reserved only while the
  language feature they belong to is turned on. A
  [compiler directive](page:language/directives) can turn the feature on or
  off for a unit. When the feature is off, the word parses as an ordinary
  identifier: `#forbid-clone-operator` lets you declare
  `function clone(x) { ... }`.

`async` and `await` mark an asynchronous function and wait for a value in
it. See [async and await](page:language/async).

## Numbers

An integer is decimal (`30`) or hexadecimal (`0x1E`, up to 16 hex digits).
A leading `0` before another digit is a compile error; octal literals are
not supported. A character between single quotes, such as `'A'`, is also an
integer literal. Its value is the one byte between the quotes, or the one
byte that an escape such as `'\n'` produces. Anything that is not exactly
one byte is an error.

A float needs a fractional part, an exponent, or both: `1.5`, `1.5e3`,
`1e-3`. There is no bare leading dot: write `0.5`, not `.5`.

An underscore may appear between digits anywhere in a number, decimal, hex
or float. It has no effect on the value: `123_456`, `0xFF_00`, `1_000.25`
are the same values as without the underscores.

{{example:language/lexical-numbers}}

## true, false, null

`true` and `false` are the two `bool` values. `null` is its own type with
one value. It is not `0`, `false`, or an empty string. See
[Values and types](page:language/types) for how equality treats it.

## __FILE__ and __LINE__

`__FILE__` and `__LINE__` are keywords, not variables. The compiler replaces
each with a literal at the position where it appears: `__FILE__` with the
path being compiled, and `__LINE__` with the number of that line. The
substitution happens where the token is in the source, so `__LINE__` inside
a function reports the line where it is written, not the line of the call.

{{example:language/lexical-fileline}}

## Statement separators

A newline ends a statement. `;` also ends a statement, and lets two
statements share one line. Both work only at a point where the current
statement can end. A trailing operator, an open `(`, or a trailing `,`
keeps the parser inside the same statement, and it continues onto the next
line to find the rest of the expression.

{{example:language/lexical-statements}}

## # is a directive, not a comment

A line starting with `#` is not a comment. It is a
[compiler directive](page:language/directives), and it changes how the whole
file compiles. Do not confuse it with `//`.

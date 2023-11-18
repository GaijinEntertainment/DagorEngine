# RFCs

This document tracks unimplemented RFCs.

## Deprecate extends keyword

Use Python style for extends
  let Bar = class {}
  let Baz = class(Bar) //(instead of Baz = class extends Bar

**Status**: Implemented

## Introduce way to call builtin table methods

to avoid ambiguity with table fields {values = @() print("values")}.values()
Introduce operator '.$' (as well as ?.$ )
  ```
    print({keys = 1}.$keys()?[0]) //will print 'keys', not 'null'

**Status**: Implemented.

## Deprecate delete operator

Use rawdelete instead

**Status**: Implemented, as optional behavior specified with pragma #forbid-delete-operator #allow-delete-operator.

## Deprecate clone operator and replace it with .clone method

**Status**: Implemented, as optional behavior currently. Can be specified by #forbid-clone-operator or #allow-clone-operator

## Add is_frozen method to array and table

**Status**: Implemented

## Add for and foreach in ranges

Syntax like:
  ```
  for (range:integer)
  for (start_of_range:integer, end_of_range:integer)
  foreach (i in range:integer)
  ```

Will allow to make code faster and safer than with for(local i=0; i<range; i++)

**Status**: Implemented.

## NaN-tagging

**Status**: Needs implementation and detailed rfc

## Incremental GC

**Status**: Needs implementation and detailed rfc

## Replace let with const

Replace 'let' for immutables with 'const' keyword

**Status**: Needs implementation and detailed rfc

## Remove :: operator

Use getroottable() or write your own wrapper for shorter syntax

**Status**: Needs implementation and detailed rfc

## Expression folding

**Status**: Needs implementation and detailed rfc

## Insert-ordered tables (like in Python)

**Status**: Needs implementation and detailed rfc

## Automatically assign names to unnamed functions

like `let foo = function() {}` == `let foo = function foo() {}`
or {foo = function()} == {foo = function foo()}

**Status**: Implemented.

## Spread operator

like https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax

**Status**: Needs implementation and detailed rfc

## AST optimizations

- loop unrolling
- filter + map folding, dead-code elimation
- Hoisting immutable variables

**Status**: Needs detailed rfc

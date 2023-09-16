# RFCs

This document tracks unimplemented RFCs.

## Deprecate extends keyword

Use Python style for extends
  let Bar = class {}
  let Baz = class(Bar) //(instead of Baz = class extends Bar

**Status**: Needs implementation and detailed rfc

## Introduce way to call table methods

to avoid ambiguity with table fields {values = @() print("values")}.values()

Suggestions:

  - with 'prefix'-style, operator .$ ( print({keys = 1}.$keys()) //'keys', not 1)
  - '::' (if and whe nwe remove :: operator) table::values()
  - '->'
  - '|>' (betterr keep for pipe)
  - some other way, for example with like getMethod({}.values)({})

**Status**: Needs implementation and detailed rfc

## Deprecate delete operator

Use rawdelete instead

**Status**: Needs implementation and detailed rfc

## Deprecate clone operator and replace it with .clone method

**Status**: Needs implementation and detailed rfc

## Add is_freezed method to array and table

**Status**: Needs implementation and detailed rfc

## Add for and foreach in ranges

Syntax like:
  ```
  for (range:integer)
  for (start_of_range:integer, end_of_range:integer)
  foreach (i in range:integer)
  ```

Will allow to make code faster and safer than with for(local i=0; i<range; i++)

**Status**: Needs implementation and detailed rfc

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

**Status**: Needs implementation and detailed rfc

## Spread operator

like https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax

**Status**: Needs implementation and detailed rfc

## AST optimizations

- loop unrolling
- filter + map folding, dead-code elimation
- Hoisting immutable variables

**Status**: Needs detailed rfc

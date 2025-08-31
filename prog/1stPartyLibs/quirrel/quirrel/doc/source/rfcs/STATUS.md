# RFCs

This document tracks RFCs, both implemented and not implemented (implemented RFC should eventually go way from here).


## Deprecate clone operator and replace it with .clone method

**Status**: Implemented, as optional behavior currently. Can be specified by #forbid-clone-operator or #allow-clone-operator


## Replace let with const

Replace 'let' for immutables with 'const' keyword
or make them aliases.

`const` and `let` for simple types works the same from coder PoV.
`let` and `const` declarations for simple types (integer, float, string, null, boolean) can be optimized (we do know in compile time what is it).
For other types const should work as 'binding' (like `let` do now).

**Status**: Waiting for implementation


## Add .hasindex(<index>) for instances, classes, tables, arrays and strings.

Should behave as 'in' operator, but exists only for types that can have 'index', and should have correct check of arguments type
(arrays and strings can have only integer as index)

**Status**: Waiting for implementation

## Add .hasvalue(<value>) for tables and arrays.

The same as 'contains' for arrays. To make it more consistent with findvalue() and findindex() and hasindex()

**Status**: Waiting for implementation


## assignments in 'if' (let and local)

```
  if (let a = foo()){
    println(a)
  }
```

equals to (except scope of visibility of 'a'):

```
  let a = foo()
  if (a) {
    println(a)
  }
```

This is sometimes much less verbose and safer to use. The same as warlus operator in python, but we do not need it in Quirrel, cause we have expicit declaration of variables

**Status**: Waiting for implementation


## Add for and foreach in ranges

Syntax like:
  ```
  for (range:integer)
  for (start_of_range:integer, end_of_range:integer)
  foreach (i in range:integer)
  ```

Will allow to make code faster and safer than with for(local i=0; i<range; i++)

**Status**: Needs implementation


## Return back _inherited and _newmember metamethods for classes, or implement some other way to validate children classes

**Status**: Not going to be implemented. _lock method is much more powerful and useful.

## Keyworded arguments for function calls, like Python

  `function foo(a=1, b=2){}`
  `foo(b=3) //the same as foo(1, 3), but much safer`

Should work almost the same as kwarg() function from functools.nut do or how python allow function calls.
As quirrel function has in it's infos information about default arguments, it can be called.

**Status**: Needs implementation and detailed rfc


## Forward declaration for bindings

```
  local a
  function b(){
    a()
  }
  a = function(){}
  let a
```

The idea is to be able convert local variable into binding, make it safer to use, but still allow to have forward declarations

**Status**: Needs implementation and detailed rfc


## Destructors in classes

Special functions that would be called just before garbage collections, like __del__ in Python

```
  let cache = {}

  let class = Foo {
    constuctor(){
      cache[this] <- true
    }
    destructor(){
      cache.$delete(this)
    }
  }
```

**Status**: Needs implementation and detailed rfc


## delete operator for bindings and locals, or unbind global function

Clear namespace, like 'del' operator in Python

  function(){
    let a = heavy_function()
    if (a) {
     let {field} = a
     del a // (or unbind(a)) will clear namespace from 'a'
     let a = do_something_heavy(field)
   }
  }

**Status**: Needs implementation and detailed rfc


## Keyworded optional arguments for function calls, like Python

  function bar(***){
    foreach(k, v in kvargs)
      println($"{k}={v}")
  }

  bar(a=1, b=2, c=3)

**Status**: Needs implementation and detailed rfc


## Remove :: operator

Use getroottable() or write your own wrapper for shorter syntax

**Status**: Needs implementation and detailed rfc

## NaN-tagging

**Status**: Needs implementation and detailed rfc

## Incremental GC

**Status**: Needs implementation and detailed rfc

## Expression folding

**Status**: Needs implementation and detailed rfc

## Insert-ordered tables (like in Python)

**Status**: Needs implementation and detailed rfc

## Spread operator

like https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax

**Status**: Needs implementation and detailed rfc

## AST optimizations

- loop unrolling
- filter + map folding, dead-code elimation
+ Hoisting immutable variables  **implemented**

**Status**: Needs detailed rfc

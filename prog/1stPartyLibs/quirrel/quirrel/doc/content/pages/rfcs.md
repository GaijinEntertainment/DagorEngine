---
title: RFCs
group: Guides
order: 96
summary: How a change to the language is proposed, and the current proposals.
layout: rfcs
---

Every change to Quirrel syntax or semantics, and every new function in a core
library, goes through an RFC. Once code depends on a change it is very hard to
reverse, so it is worth discussing first.

## Why an RFC

Whenever Quirrel changes its syntax or semantics (including behavior of builtin
libraries), we need to consider many implications of the changes.

Whenever new syntax is introduced, we need to ask:

- Is it backwards compatible?
- Is it easy for machines and humans to parse?
- Does it follow 'Zen of Quirrel' (based on Zen of Python?)
- Does it create grammar ambiguities for current and future syntax?
- Is it stylistically coherent with the rest of the language?
- Does it present challenges with editor integration like autocomplete?
- Will it affect performance?

For changes in semantics, we need to ask:

- Is behavior easy to understand and non-surprising?
- Can it be implemented performantly today?
- Is it compatible with type checking and other forms of static analysis?

For new standard library functions, we need to ask:

- Is the new functionality used/useful often enough in existing code?
- Does the standard library implementation carry important performance
  benefits that can't be achieved in user code?
- Is the behavior general and unambiguous, as opposed to solving a problem /
  providing an interface that's too specific?
- Is the function interface amenable to type checking / linting?

In addition to these questions, we also need to consider that every addition
carries a cost. Too many features result in a language that is harder to
learn, harder to implement with consistent quality, slower, etc. Any language
is greater than the sum of its parts, and features often have non-intuitive
interactions with each other.

Since reversing these decisions is very costly and can be impossible due to
backwards compatibility, all user facing changes to the Quirrel language and
core libraries must go through an RFC process.

## The process

There is no special process for RFC review at the moment.

When an RFC gets merged, the feature *can* be implemented; however, there is no
set timeline for that implementation. In some cases implementation may land in
a matter of days after an RFC is merged, in some it may take months.

To avoid permanently stale RFCs, in rare cases the Quirrel team can *remove* a
previously merged RFC when the landscape has changed enough for the feature to
need further discussion.

When an RFC is implemented, its entry below gets "**Status**: Implemented" and
a link to the page that documents the feature. The entry stays until nothing
about the feature is optional any more, so that the same idea is not proposed
twice.

## Implemented

### Deprecate the clone operator and replace it with a .clone method

Every type has a [clone](sym:types.Table.clone) method, but `clone` is a
keyword, so `x.clone()` does not parse by default. `#forbid-clone-operator`
turns the keyword into a plain identifier, which makes the method reachable;
`#allow-clone-operator` turns the operator back on. See
[Compiler directives](page:language/directives#delete-and-clone) and
[clone](page:language/operators#clone).

**Status**: Implemented as optional behavior. The operator is on by default.

### Assignments in if (let and local)

`if (let x = expr)` tests `x` for truth, and `if (let x = expr; cond)` tests a
separate condition. The binding is visible in the whole `if` / `else` chain.
See [Control flow](page:language/control-flow#if-else).

**Status**: Implemented.

### Add .hasindex(index) for instances, classes, tables, arrays and strings

Behaves as the `in` operator, but exists only on the types that have an index,
and checks the argument type: an array or a string accepts a number only.
[table](sym:types.Table.hasindex), [array](sym:types.Array.hasindex),
[string](sym:types.String.hasindex), [class](sym:types.Class.hasindex) and
[instance](sym:types.Instance.hasindex) have it.

**Status**: Implemented.

### Add .hasvalue(value) for tables and arrays

On an array it is the same as `contains`. The name matches `findvalue`,
`findindex` and `hasindex`. [table](sym:types.Table.hasvalue) and
[array](sym:types.Array.hasvalue) have it.

**Status**: Implemented.

### Forward declaration for bindings

The proposal was to declare a `local`, assign it, then turn it into a binding
with a trailing `let`:

```
local a
function b() {
  a()
}
a = function() {}
let a
```

The implemented form is the other way round. `let name` with no initializer
forward-declares the binding, and exactly one later assignment in the same
scope defines it. A read before the definition, a missing definition and a
second assignment are compile errors. See
[Bindings and constants](page:language/bindings#forward-declaration).

**Status**: Implemented, with a different spelling than proposed.

### Remove the :: operator

`#forbid-root-table`, which `#strict` also sets, makes `::name` a compile
error; `#allow-root-table` turns it back on. Code that needs the root table
calls [getroottable](sym:getroottable). See
[Compiler directives](page:language/directives#root-table-access).

**Status**: Implemented as optional behavior. The operator is on by default.

## Partly implemented

### Replace let with const

Replace `let` for immutables with `const`, or make them aliases. For a simple
type (integer, float, string, null, boolean) a `let` and a `const` look the
same to the coder and can be resolved at compile time; for other types `const`
would act as a binding, as `let` does now.

The two are still distinct, and they are not equally cheap. A `let` lives in a
register. A `const` is substituted at each use, which costs an extra
instruction for some operators. An experiment that compiled a literal `let` as
a scoped const, off by default, was abandoned. What did land is a wider
`const`. The initializer may be any expression the compiler can evaluate: a
table or array of constants, a call to a `pure` function, or a function that
captures nothing. `const expr` is also accepted inline inside an expression.
See
[Bindings and constants](page:language/bindings#const-and-global-const).

**Status**: Open. The alias is not implemented.

### Keyword arguments for function calls, like Python

Two proposals, merged here. Name an argument at the call:

```
function foo(a = 1, b = 2) {}
foo(b = 3) // the same as foo(1, 3), but much safer
```

And collect the keyword arguments a function did not declare, as Python's
`**kwargs` does:

```
function bar(***) {
  foreach (k, v in kvargs)
    println($"{k}={v}")
}

bar(a = 1, b = 2, c = 3)
```

Neither is syntax today. `foo(b = 3)` is the compile error `'=' inside
'function argument' is forbidden`, and `***` does not parse. Destructuring in
the parameter list covers the first proposal. `function foo({ a = 1, b = 2 })`
is called as `foo({ b = 3 })`. A missing key takes its default, a key without a
default must be present, and an extra key is ignored. See
[Destructuring](page:language/destructuring) and
[Functions](page:language/functions).

**Status**: Covered by parameter destructuring. A separate syntax needs a
detailed RFC that states what destructuring does not give.

### Spread operator

Like [JavaScript's spread syntax](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax).
`...expr` in a table or an array literal copies the source into the literal
that is being built, a `const` initializer included. A table literal takes a
table, a class or an instance; an array literal takes an array; a `null`
source adds nothing. See
[Tables and arrays](page:language/containers#spread).

JavaScript also spreads at a call and collects a rest element when it
destructures. Neither is syntax here. `f(...args)` needs an argument count
that the `_OP_CALL` instruction holds at compile time, and a rest element
needs a runtime slice of the source. Today
[acall](sym:types.Function.acall) passes an array as the argument list, and
[slice](sym:types.Array.slice) takes the tail of an array.

**Status**: Implemented for table and array literals. The call case and the
rest element in destructuring each need a detailed RFC.

### Compiler optimizations

Expression folding and the AST optimizations, merged here.

- Constant folding: done. A `const` initializer and any compound constant
  expression fold to one load at the AST level, and the bytecode peephole
  optimizer then folds constant arithmetic, jump chains and empty jumps.
  `#disable-optimizer` turns the peephole pass off. See
  [Compiler directives](page:language/directives#the-optimizer).
- Hoisting: closure hoisting, behind the `-optCH` compile option
  (`CO_CLOSURE_HOISTING_OPT`), moves an inner function out of the loop or
  function that creates it when the move is safe, so the closure is not built
  again on every pass.
- Hoisting a loop-invariant value, loop unrolling, `filter` + `map` fusion and
  dead-code elimination: not done.

**Status**: Partly implemented. The rest needs a detailed RFC.

## Open

### for and foreach over a range

```
for (range:integer)
for (start_of_range:integer, end_of_range:integer)
foreach (i in range:integer)
```

Faster and safer than `for (local i = 0; i < range; i++)`. Today `for` is
C-style only, and `foreach` over an integer throws `cannot iterate integer`.

**Status**: Needs implementation.

### Destructors in classes

A method called just before the garbage collector frees the instance, like
`__del__` in Python:

```
let cache = {}

class Foo {
  constructor() {
    cache[this] <- true
  }
  destructor() {
    cache.$rawdelete(this)
  }
}
```

**Status**: Needs implementation and a detailed RFC.

### delete operator for bindings and locals, or an unbind function

Clear a name from the scope, like `del` in Python:

```
function f() {
  let a = heavy_function()
  if (a) {
    let { field } = a
    del a // (or unbind(a)) clears 'a' from the scope
    let a = do_something_heavy(field)
  }
}
```

**Status**: Needs implementation and a detailed RFC.

### NaN-tagging

Pack the type tag into the unused bits of a NaN, so that a value fits in one
machine word. The VM still stores a value as a type tag, flags and a union.

**Status**: Needs implementation and a detailed RFC.

### Incremental GC

**Status**: Needs implementation and a detailed RFC.

### Insert-ordered tables (like in Python)

Iteration order still depends on the table's internal layout and is reseeded
on every run. See
[Tables and arrays](page:language/containers#iteration-order).

**Status**: Needs implementation and a detailed RFC.

## Rejected

### Return the _inherited and _newmember metamethods, or another way to validate child classes

`_lock` is the replacement. It runs once, when the class locks: at the first
instantiation, when another class inherits it, or on an explicit
[lock](sym:types.Class.lock). At that moment the class can still be modified
with [newmember](sym:types.Class.newmember). See
[Metamethods](page:language/metamethods#lock).

**Status**: Not going to be implemented.

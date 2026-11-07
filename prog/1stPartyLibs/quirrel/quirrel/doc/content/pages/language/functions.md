---
title: Functions
group: Language
order: 30
summary: Definitions, default and vararg parameters, calls and closures.
---

A function is a value. It can be stored in a table or an array, passed to
another function, and returned. It captures the bindings that are visible where
it is written.

## Declaring one

`function name(params) { ... }` binds a name in the current scope. A parameter may
have a default, and a trailing `...` collects the rest of the arguments into
`vargv`, an ordinary array. A default is evaluated once, when the closure is
created. A table or array default is one object that every call without that
argument shares, so do not mutate it.

Defaults may only come after the parameters without one, and a call must still
pass every parameter that has no default.

A parameter and the return value may also carry a declared type - see
[Type annotations](page:language/annotations).

{{example:language/functions-minimal}}

The same function written as a lambda takes the same arguments and returns the
same value. `@(params) expression` is a shorter form for a function with one
expression.

{{example:language/functions-basics}}

## Lambdas

`@(params) expression` is a function with one expression. The value of the
expression is the result. There is no `return`.

The body is an expression, so braces around it make a table literal, not a
statement block. `@() {}` is a function that returns an empty table, not a
function with an empty body. When the body needs statements, write `function`.

{{example:language/functions-lambdas}}

## this

A function called through a table or an instance receives that container as
`this`. A function taken out of its container and called on its own does not:
`this` is then the value that the caller supplies.
[bindenv](sym:types.Function.bindenv) attaches a fixed `this`.
[call](sym:types.Function.call) passes a `this` for one call.

{{example:language/functions-this-basic}}

A method called through the instance that owns it gets that instance as `this`.
A detached method gets the `this` that the caller supplies.

{{example:language/functions-this}}

## Its own name

In its body, a function can call itself by its own name:
`function fact(n) { return n <= 1 ? 1 : n * fact(n - 1) }`. A lambda bound by
`let` gets the name of the binding. A function nested in the body does not see
the name.

When a variable, a constant or an import outside the function has the same
name, a use of that name in the body is a compile error, as it could mean
either. Give the outer binding another name:

```nut
from "lobby.nut" import leaveQueue as leaveQueueImpl

let Lobby = class {
  function leaveQueue() {
    leaveQueueImpl(this.id)
  }
}
```

## Attributes

A declaration can carry attributes in brackets. They tell the compiler what a
call may assume: `const function [pure] armorAt(angle) { ... }`. See
[Function attributes](page:attributes).

## Edge cases

- A parameter list is checked at the call. Too few or too many arguments throws
  `wrong number of parameters`, and the message counts `this`, so a two parameter
  function reports "3 required".
- Only a function declared with `...` gets its own `vargv`. In a function without
  `...`, `vargv` is an ordinary captured name. It resolves to the nearest enclosing
  function with `...` (the top-level script always counts as one). It then holds
  the extra arguments of that outer function, not of the current call.
- A named function expression, `let update = function updateSquad() {...}`, names
  the closure for stack traces without binding `updateSquad` in the enclosing
  scope.
- `clone` on a function returns the same function. Functions are immutable, so
  there is nothing to copy.

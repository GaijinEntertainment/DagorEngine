---
params: [env]
see_also: [types.Function.call, types.Function.getfreevar]
---

Returns a copy of the closure with `env` statically bound as its `this`:
every future call to the copy uses `env`, no matter how it is called.

## Parameters

- `env` - a table, class or instance to bind as `this`

## Return value

A new closure. The original is unchanged.

## Errors

Throws `parameter 1 of 'bindenv' has an invalid type` when `env` is not a
table, class or instance - an array is rejected too, even though it is a
valid environment for other purposes in the language.

## Notes

The bound closure keeps only a weak reference to `env`. If nothing else keeps
a strong reference to it, `env` can be collected before the closure is ever
called, and `this` reads back as `null` inside the call:

```nut
let f = (function() { return this }).bindenv({tag = "temporary"})
f() // null: the table literal had no other owner
```

Give the environment a named local (or another strong owner) for as long as
the bound closure needs it.

The bound `this` keeps the frozen state of `env`: after
`f.bindenv(freeze(t))`, a write to `this` inside `f` is rejected.

## Example

{{example:types.Function.bindenv}}

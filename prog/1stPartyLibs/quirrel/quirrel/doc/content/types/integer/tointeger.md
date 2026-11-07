---
see_also: [types.Integer.tofloat, types.Integer.tochar, types.Float.tointeger, types.Bool.tointeger]
---

Returns the integer itself.

## Return value

`this`, unchanged. Provided so every numeric type answers the same
`tointeger`/`tofloat` pair without the caller checking which one it already
has.

## Notes

Takes no arguments, and that arity is fixed: unlike
[`types.String.tointeger`](sym:types.String.tointeger), which takes an
optional `base` for parsing digits out of text, the number-typed
`tointeger()` has nothing left to parse and was registered with no room for
an extra argument. For an integer `n`, `n.tointeger(16)` throws a
wrong-number-of-parameters error rather than silently ignoring the base. With a
constant receiver, such as `(5).tointeger(16)`, the compiler folds the call, so
the error fails the compile.

See [`types.Float.tointeger`](sym:types.Float.tointeger) for what happens
when the value being converted has a fractional part, or is too
large to fit in a 64-bit integer.

## Example

{{example:types.Integer.tointeger}}

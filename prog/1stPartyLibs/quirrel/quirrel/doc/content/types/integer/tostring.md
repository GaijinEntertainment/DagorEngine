---
see_also: [types.Integer.tointeger, types.Float.tostring, types.Bool.tostring, types.WeakRef.tostring]
---

Converts the integer to its decimal string form.

## Return value

The digits of the integer, with a leading `-` for a negative value. No
thousands separators, no leading zeros, no format specifiers - the same text
`print` shows for an integer.

## Notes

Takes no arguments: for an integer `n`, `n.tostring(1)` throws `wrong number
of parameters passed to native closure 'tostring' (2 passed, 1 required)`. With
a constant receiver, such as `(5).tostring(1)`, the compiler folds the call, so
the error fails the compile.

This calls the same VM conversion as `tostring()`, so every built-in type has
a `tostring` of its own; see [`types.Float.tostring`](sym:types.Float.tostring)
for what a fractional part does to the output and
[`types.Bool.tostring`](sym:types.Bool.tostring) for `true`/`false`.
`null` has no `tostring` method - see
[`types.Null.constructor`](sym:types.Null.constructor).

## Example

{{example:types.Integer.tostring}}

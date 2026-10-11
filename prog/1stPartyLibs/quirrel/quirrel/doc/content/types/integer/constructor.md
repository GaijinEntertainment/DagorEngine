---
see_also: [types.Integer.tointeger, types.Float.constructor, types.Bool.constructor, types.classof]
---

Converts `value` to an integer. This is what runs when `types.Integer` itself
is called, `types.Integer(value)`.

## Return value

`0` when called with no arguments. Otherwise `value` converted to an
integer: an integer is returned as is, a float is truncated toward zero the
same as [`tointeger`](sym:types.Integer.tointeger), a bool gives `0` or `1`,
and a string is parsed as a number in the given `base` (default `10`, so
`types.Integer("2A", 16)` reads `"2A"` in base 16).

## Errors

Throws `cannot convert to Integer` for a `table`, `array`, `null` or any
other type a number cannot come from, and `cannot convert string to Integer`
when the string does not parse in the given base.

## Notes

Takes zero to three arguments: `value` and `base` are both optional, but the
VM cannot show them by name here - this binding carries no declaration
string, so it dumps as the generic fallback
`(table|userdata|instance|class|null).constructor(...)`, the same
placeholder receiver described on
[`types.String.constructor`](sym:types.String.constructor). Calling it with
four or more arguments does not throw either; the extra ones are read but
never used.

`constructor` is an ordinary name, so `x.constructor` parses like any other
field access. [`clone`](sym:types.Integer.clone) is a reserved word; see its
Notes.

## Example

{{example:types.Integer.constructor}}

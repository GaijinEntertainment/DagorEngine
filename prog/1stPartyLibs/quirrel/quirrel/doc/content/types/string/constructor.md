---
params: [value]
see_also: [types.String.tostring, types.classof]
---

Converts `value` to a string. This is what runs when `types.String` itself is
called, `types.String(value)`, the same way `types.Integer(value)` converts to
an integer.

## Parameters

- `value` - the value to convert

## Return value

`value` converted to a string, the same conversion `tostring(value)` performs
for the built-in types (a number prints its digits, `true`/`false` print as
those words, `null` prints as `"null"`, and a table, array, class, instance,
function, generator, thread or weakref prints a description such as
`"(table : 0x...)"` unless its class defines `_tostring`).

## Errors

Throws `cannot convert to String` when the conversion fails. Every
built-in type converts successfully, so this is not reachable through
ordinary Quirrel code; it exists for a `_tostring` metamethod that itself
fails.

## Notes

Takes exactly one argument: calling `types.String()` with no argument, or with
two or more, throws a wrong-number-of-parameters error, because this binding
carries no declaration string and so cannot show `value` by name; the VM
dumps it as `(table|userdata|instance|class|null).constructor(arg1)`, a
generic fallback receiver used for every binding with no declared parameter
types, not the real receiver.

`constructor` is an ordinary name, so `x.constructor` reads like any other
field access. The same name declares a class's constructor method.
`types.String.clone` is a reserved word; see its Notes.

## Example

{{example:types.String.constructor}}

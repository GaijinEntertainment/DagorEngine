---
see_also: [op:typeof, getobjflags]
---

Returns the name of `obj`'s type.

## Parameters

- `obj` - any value

## Return value

One of `"null"`, `"bool"`, `"integer"`, `"float"`, `"string"`, `"table"`,
`"array"`, `"function"`, `"generator"`, `"userdata"`, `"thread"`, `"class"`,
`"instance"` or `"weakref"`.

## Notes

This is a function, while `typeof` is an operator: `type` can be stored in a
binding or passed as a callback, and `typeof` cannot. `typeof(v)` looks like the
same thing written differently, but those parentheses group the operand.

Ignores metamethods, unlike the `typeof` operator: `typeof` calls a class's
`_typeof` method when it defines one, so an instance can report any string it
likes there, while `type()` on the same instance always reports the plain
`"instance"`. Marked `pure` because the result depends only on `obj`, and
`fastcall` because it calls nothing back and pushes one value.

## Example

{{example:type}}

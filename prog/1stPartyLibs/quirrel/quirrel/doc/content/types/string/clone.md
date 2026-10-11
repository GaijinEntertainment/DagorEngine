---
see_also: [types.String.weakref, types.String.constructor]
---

Returns `str` itself.

## Return value

`str` unchanged. A Quirrel string is an immutable value, so cloning one has
nothing to copy: the VM's `Clone` falls through to its `default:` case, which
returns the same object instead of allocating a copy (contrast a table or an
array, which do allocate a new one).

## Notes

Takes no arguments.

`clone` is a reserved word (it is also the unary `clone x` operator), and the
parser only accepts an identifier after `.` and `.$`, so `str.clone()` and
`str.$clone()` both fail to compile with `expected 'IDENTIFIER'`. Call this
type method through a computed index instead, `str["clone"]()`, or write
`clone str`. With
[`#forbid-clone-operator`](page:language/directives#delete-and-clone) the
word is an ordinary identifier and `str.$clone()` compiles.
`str.constructor` needs no directive: `constructor` is an ordinary name.

## Example

{{example:types.String.clone}}

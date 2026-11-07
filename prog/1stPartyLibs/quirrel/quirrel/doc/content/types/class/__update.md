---
params: [other]
see_also: [types.Class.__merge, types.Class.newmember, types.Table.__update]
---

Copies every own member of each argument into the class it is called on, and
returns that same class. Unlike `__merge`, nothing new is allocated.

## Parameters

- `other` - a table, class, or instance to copy members from
- `...` - more tables, classes, or instances, copied in order

## Return value

The receiver itself, after the copy.

## Errors

Throws a parameter type error naming the argument position when a source is not
a table, class, or instance. Takes at least one argument beyond the receiver;
calling it with none throws `wrong number of parameters passed to native closure
'__update' (1 passed, at least 2 required)`. Since it mutates the receiver in
place, every rule for `newmember`/`rawset` on a class applies: a locked class
and a frozen reference reject any change.

## Notes

`Class.__update` and `table.__update` are the same native function; only the
receiver's type differs. A same-named member from a later argument overwrites
one from an earlier argument or from the receiver itself.

## Example

{{example:types.Class.__update}}

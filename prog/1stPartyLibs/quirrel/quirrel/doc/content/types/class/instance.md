---
see_also: [types.Class.call, types.Instance.getclass, types.Class.lock]
---

Creates a new instance of the class without running its constructor.

## Return value

A new instance whose fields hold the class's plain declared defaults - the same
values `ClassName()` would give them before the constructor runs, since
Quirrel never clones a field's default value per instance (see
`types.Class.newmember`'s Notes on default values, and the language reference
on class instances).

## Notes

Locks the class as a side effect, the same as calling `ClassName(...)` does: a
class becomes read-only the first time any instance of it exists, whichever way
that instance was made, so add every member before that.

Useful for a class whose constructor requires arguments a caller does not have
yet, or for tooling that wants a template instance to inspect without running
arbitrary constructor code.

## Example

{{example:types.Class.instance}}

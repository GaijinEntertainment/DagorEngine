---
params: [key, value]
see_also: [types.Class.rawget, types.Class.newmember, types.Class.lock, types.Class.is_frozen]
---

Creates or replaces a member of the class, and unlike `newmember`, wires `key`
and `value` to the roles their names suggest.

## Parameters

- `key` - the member name to create or replace
- `value` - the value to store there

## Return value

The class itself, so calls can be chained.

## Errors

On a locked class, throws `trying to modify a class that has already been
instantiated, inherited or is locked manually` for any key, not only a new
one, and for any value, a closure included. On a frozen
reference, throws `Cannot modify immutable object` - a different message from
the one `newmember` raises for the same condition, because this check runs
earlier, in `sqbaselib.cpp`, rather than in `SQVM::NewSlot`.

## Notes

On an unlocked class, `rawset` on an existing key overwrites that field's
*default* value for future instances; instances already created keep whatever
value they copied at construction. This is the same rule the language
reference states for `ClassName.field <- value` after declaration.

## Example

{{example:types.Class.rawset}}

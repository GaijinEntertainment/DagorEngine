---
see_also: [types.Class.instance, types.Class.newmember, types.Class.is_frozen]
---

Locks the class immediately, without creating an instance.

## Return value

The class itself.

## Errors

Propagates whatever a `_lock` metamethod throws, if the class defines one.

## Notes

A class locks automatically the first time it gains an instance, is used as a
base for another class, or has `instance()` called on it; `lock()` exists to
trigger that permanently-instantiated state on demand, without either. Locking
an already-locked class is a harmless no-op.

A locked class is read-only: any change throws `trying to modify a class that
has already been instantiated, inherited or is locked manually`. A `_lock`
metamethod runs before the lock, so it can still add members. If the class has
a base, the base is locked first, recursively.

## Example

{{example:types.Class.lock}}

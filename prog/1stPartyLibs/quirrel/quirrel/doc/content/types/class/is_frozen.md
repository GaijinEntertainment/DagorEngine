---
see_also: [types.Class.lock, types.Class.newmember, types.Class.rawset, freeze]
---

Reports whether this reference to the class was frozen with `freeze()`.

## Return value

`true` if the class is immutable, `false` otherwise.

## Notes

Takes no arguments. The immutable flag lives on the reference, not on the
class object: `freeze(SomeClass)` returns a new, frozen reference to the same
class, but a variable that already held that class keeps pointing at an
unfrozen reference, and so does every copy made from it before the freeze.
`SomeClass.is_frozen()` stays `false` unless the variable itself is reassigned
to the frozen result, as in `SomeClass = freeze(SomeClass)`.

A frozen class rejects `newmember` and `rawset` with `trying to modify
immutable 'class'` and `Cannot modify immutable object` respectively (the two
native paths report it with different text). `lock()` also rejects every
mutation, but through every reference and only from the moment the class
locks; a frozen reference rejects them on a class that is not locked yet too.

## Example

{{example:types.Class.is_frozen}}

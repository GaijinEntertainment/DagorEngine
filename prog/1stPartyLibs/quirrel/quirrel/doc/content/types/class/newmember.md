---
params: [key, value]
see_also: [types.Class.rawset, types.Class.lock, types.Class.is_frozen]
---

Adds a member to the class, optionally as a static one.

## Parameters

- `key` - name of the new slot
- `value` - value stored in it

## Return value

The class, so calls can be chained.

## Errors

Throws `wrong number of parameters passed to native closure 'newmember' (N
passed, at least 3 required)` when `value` is missing; both `key` and `value`
are required.

On a class locked by [lock](sym:types.Class.lock), or one already instantiated
or inherited from, adding or replacing any member, static or not, throws
`trying to modify a class that has already been instantiated, inherited or is
locked manually`.
On a frozen reference it throws `trying to modify immutable 'class'`.

## Notes

A third, boolean argument requests a static member: one value shared by every
instance rather than a per-instance default. The VM cannot report that
parameter, because this method is bound with a type mask, so it is absorbed
into the trailing `...` of the signature above.

Quirrel classes have no per-member attribute storage, so there is no
`attributes` argument, whatever older Squirrel-family references show.

## Example

{{example:types.Class.newmember}}

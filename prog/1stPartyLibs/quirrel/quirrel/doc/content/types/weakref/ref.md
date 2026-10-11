---
see_also: [types.WeakRef.constructor, types.WeakRef.weakref, types.Table.weakref]
---

Returns the value this `weakref` points at, or `null` once that value is
gone.

## Return value

The original value for as long as something else still holds a strong
reference to it. Once the last strong reference is dropped, `ref()` returns
`null` instead - it never throws for a dead reference.

## Notes

Takes no arguments; `wr.ref(1)` throws a wrong-number-of-parameters error.

The returned reference has the frozen state of the reference that made the
`weakref`: `freeze(t).weakref().ref()` is frozen and `t.weakref().ref()` is
not. Thus these two `weakref` values are different, and not `==`.

Dropping a variable's value does not always show up immediately: leaving a
`{ }` block does not, by itself, release a local's reference the moment
control leaves the block - the value's slot on the VM stack keeps holding it
until something later overwrites that same slot. Setting the variable to
`null` explicitly forces the release at a known point; relying on scope exit
alone for a "did it die yet" check is not reliable.

A `weakref` to a value that was never reference counted in the first place
(an `integer`, `float`, `bool` or `null`) never goes dead, because
`ref()` returns that same value every time - see
[`types.WeakRef.constructor`](sym:types.WeakRef.constructor).

## Example

{{example:types.WeakRef.ref}}

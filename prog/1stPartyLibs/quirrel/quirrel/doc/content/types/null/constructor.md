---
see_also: [types.Bool.constructor, types.Integer.constructor, type, types.classof]
---

Returns `null`. This is what runs when `types.Null` itself is called,
`types.Null()`.

## Return value

Always `null`, regardless of any arguments given.

## Notes

`null` is the only value in this class, so unlike every other type's
`constructor` there is nothing to convert: any arguments are read off the
stack and thrown away, never inspected, so `types.Null(1, 2, 3)` is legal and
still gives `null`. This never throws.

`null` has no other method - not `tostring`, `tointeger`, `tofloat`,
`clone` or `weakref`. Calling any of those on `null` throws `the index
'<name>' does not exist`, the plain missing-slot error, not a type-specific
one. `null` still formats as `"null"` when `print` or string concatenation
converts it implicitly, since that path does not go through a method lookup;
see [`types.Bool.tostring`](sym:types.Bool.tostring) for the same contrast
on a type that does have the method.

`null.constructor()` parses like an ordinary method call, even though `null`
has no other reachable methods.

## Example

{{example:types.Null.constructor}}

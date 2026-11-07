---
see_also: [type, getobjflags]
---

Returns the class that `obj` belongs to.

## Parameters

- `obj` - any value

## Return value

For an instance, its own defining class - the exact class it was
constructed from, not a common ancestor. For anything else, including a
class value itself, the built-in class object for `obj`'s type: `types.Integer`,
`types.Float`, `types.Bool`, `types.String`, `types.Array`, `types.Table`,
`types.Function` (for both a script closure and a native closure),
`types.Generator`, `types.Thread`, `types.Class`, `types.WeakRef`,
`types.UserData` or `types.Null`.

`types.Instance` is never returned this way: a plain instance always maps to
its own defining class instead of that placeholder built-in class.

## Notes

Like `type`, `classof` ignores metamethods: it never calls `_typeof`, so a
class that defines `_typeof` has no effect on it. Unlike `type`, which
reports every instance with the same string `"instance"`, `classof` returns
the actual class, so it can tell one class of instance from another. Unlike
`typeof`, whose result a class can change by defining `_typeof`, `classof`
always returns the real class object read straight from `obj`.

Marked `pure` because the result depends only on `obj`, and `fastcall` because
it calls nothing back and pushes at most one value.

## Example

{{example:types.classof}}

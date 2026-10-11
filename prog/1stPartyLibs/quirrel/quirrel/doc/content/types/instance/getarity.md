---
see_also: [types.Instance.getfuncinfos, types.Class.getarity, types.Function.getarity]
---

Returns the argument counts of this instance's class's `_call` metamethod,
the same way `function.getarity` does for a plain function.

## Return value

`null` if the instance's class defines no `_call` metamethod,
so the instance cannot be called like a function.
Otherwise the counts of the `_call` closure, its environment parameter included,
as `function.getarity` describes.

## Notes

Equivalent to `getclass().getarity()`. The counts come from the `_call` closure,
not from a `getfuncinfos` method that the class defines.

## Example

{{example:types.Instance.getarity}}

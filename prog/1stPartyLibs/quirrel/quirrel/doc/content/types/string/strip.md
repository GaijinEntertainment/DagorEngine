---
see_also: [types.String.lstrip, types.String.rstrip, string.strip]
---

Returns `str` with white space removed from both ends.

## Return value

A new string with the leading and trailing white space removed. See
[string.strip](sym:string.strip) for which bytes count as white
space and what happens to an all-white-space or empty `str`.

## Notes

Takes no arguments. This method and [string.strip](sym:string.strip) call
the same C++ implementation with the same behavior; `"  hi  ".strip()` gives
the same result as `strip("  hi  ")`. Both are `pure`. They differ only in
`fastcall`: the module function has it in its declaration string, while this
method is registered through a type-mask table that has no `fastcall` field.

## Example

{{example:types.String.strip}}

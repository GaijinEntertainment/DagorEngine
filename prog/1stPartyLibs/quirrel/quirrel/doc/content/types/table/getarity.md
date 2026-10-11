---
see_also: [types.Table.getfuncinfos, types.Function.getarity]
---

If the table has a delegate with a `_call` metamethod, returns the argument
counts of that metamethod's closure; otherwise returns `null`.

## Return value

`null`, or the counts of the `_call` closure, its environment parameter included,
as `function.getarity` describes.

## Notes

Takes no arguments. Like `getfuncinfos`, this almost always returns `null`
for a table made in script: only the C API can attach a delegate to a table,
and a slot named `_call` on the table itself does not count.

## Example

{{example:types.Table.getarity}}

---
see_also: [types.UserData.getfuncinfos, types.Table.getarity]
---

If the userdata has a delegate with a `_call` metamethod,
returns the argument counts of that metamethod's closure; otherwise returns `null`.

## Return value

`null`, or the counts of the `_call` closure, its environment parameter included,
as [`types.Function.getarity`](sym:types.Function.getarity) describes.

## Errors

A delegate table without its own `getarity` slot gives the receiver-type error
that [`types.UserData.getfuncinfos`](sym:types.UserData.getfuncinfos) describes.
Script code cannot make a userdata; it reaches this method on one from a host.

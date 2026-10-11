---
see_also: [types.Function.getfuncinfos, types.Function.call]
---

Returns how many arguments the closure takes: the counts that `getfuncinfos`
gives, without the names, defaults and type masks it collects.

## Return value

A table with three keys:

- `num_params` - the number of named parameters, without `this` and the `...` tail;
  the same as `getfuncinfos().parameters.len() - 1`
- `num_required` - how many of them have no default value,
  the same as `num_params - getfuncinfos().defparams.len()`
- `varargs` - `true` when the closure ends in `...`

## Notes

Use it where only the argument count matters, for example to select how many
arguments to pass to a callback. `getfuncinfos` makes a table of 16 or 17 slots
and 3 arrays on each call, and `getarity` makes one table of 3 slots.

A native closure without a declaration string reports `max(abs(paramscheck), 1) - 1`
for both counts, and `varargs` when `paramscheck` is 0 or less.

## Example

{{example:types.Function.getarity}}

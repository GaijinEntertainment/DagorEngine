---
see_also: [types.Class.getfuncinfos, types.Instance.getarity, types.Function.getarity]
---

Returns the argument counts of the class's `_call` metamethod, the same way
`function.getarity` does for a plain function.

## Return value

`null` if the class defines no `_call` metamethod.
Otherwise a table with `num_params`, `num_required` and `varargs`, as `function.getarity`
describes. The counts include the environment parameter that `_call` declares first.

## Notes

Use `getarity` when only the counts are necessary, as it does not make the full
introspection table.

## Example

{{example:types.Class.getarity}}

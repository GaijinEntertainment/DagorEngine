---
params: [comparator]
see_also: [types.Array.sort, types.Array.sortby_reversed, types.Array.reverse]
---

Sorts the array in place in descending order: the order of
[sort](sym:types.Array.sort) with the two arguments of each compare swapped.

## Parameters

- `comparator` - the ascending order, as for [sort](sym:types.Array.sort): a
  negative result puts `a` after `b`; the default ordering when left out

## Return value

This array.

## Errors

The same as [sort](sym:types.Array.sort). The `comparison between X and Y`
error names the pair in the swapped order.

## Notes

`arr.sort_reversed(cmp)` gives the same result as
`arr.sort(@(a, b) cmp(b, a))`, and `arr.sort_reversed()` the same as
`arr.sort(@(a, b) b <=> a)`. The `comparator` and the `_cmp` metamethods get
the same calls as with that swapped comparator, but no script function swaps
the arguments.

The sort is stable: elements that compare equal keep their relative order,
as with `reverse=True` in Python. `arr.sort()` and then `arr.reverse()` puts
equal elements in the reverse of their input order, so it gives the same
result only when equal elements cannot be told apart, such as integers or
strings.

Without a `comparator`, `sort_reversed` costs the same as `sort`: for 32
elements or more it is 6 to 7 times faster than
`arr.sort(@(a, b) b <=> a)`, and 2.3 times faster for 2 elements. For an
order by a value of each element, use
[sortby_reversed](sym:types.Array.sortby_reversed).

As with `sort`, more than one argument is not an error: the `comparator` is
then ignored and the default ordering is used.

## Example

{{example:types.Array.sort_reversed}}

---
params: [key]
see_also: [types.Array.sortby, types.Array.sort_reversed]
---

Sorts the array in place by the value that `key` returns for each element,
in descending order.

## Parameters

- `key` - `key(value, [index], [array])`, called once for each element, in
  order, before the sort; not called for an array of one element or none

## Return value

This array.

## Errors

The same as [sortby](sym:types.Array.sortby). The `comparison between X and
Y` error names the pair of keys in the swapped order.

## Notes

`arr.sortby_reversed(key)` gives the same order as
`arr.sort(@(a, b) key(b) <=> key(a))`, and calls `key` once for each element
as [sortby](sym:types.Array.sortby) does. The `_cmp` metamethods of the keys
get the same calls as with that comparator.

The sort is stable: elements with equal keys keep their relative order.
`arr.sortby(@(v) -v.score)` gives the same order for a numeric key, but it
cannot negate a string, a bool or a null key; `sortby_reversed` sorts any
keys that `<` can compare, and a null key sorts last.

It holds the same extra slots as `sortby` and costs the same.

## Example

{{example:types.Array.sortby_reversed}}

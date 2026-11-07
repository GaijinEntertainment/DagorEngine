---
params: [comparator]
see_also: [types.Array.sort_reversed, types.Array.sortby, types.Array.reverse]
---

Sorts the array in place.

## Parameters

- `comparator` - compares two elements; the default ordering when left out
  (see Notes)

## Return value

This array.

## Errors

Whatever `comparator` throws. Without a `comparator`, throws `comparison
between X and Y` for a pair of elements neither numeric-with-numeric,
string-with-string, nor both instances of a class (or tables) that defines
`_cmp`.

## Notes

`comparator(a, b)` must return a negative number when `a` sorts before `b`,
a positive number when it sorts after, and `0` when they are equal, the same
convention as `_cmp`.

Despite the trailing `...` in the signature above, `sort` is not variadic:
it takes zero or one real argument. Calling it with more than one extra
argument does not throw, unlike [findvalue](sym:types.Array.findvalue); it
silently falls back to the default ordering, as if `comparator` had been
left out entirely.

Without a `comparator`, elements are compared the way `<` compares them:
numbers by value, strings lexicographically, and a table or instance through
its `_cmp` metamethod if it has one; anything else is a `comparison between`
error.

Each call of a script `comparator` costs much more than a compare without
one. For a descending order, [sort_reversed](sym:types.Array.sort_reversed)
gives the result of `arr.sort(@(a, b) b <=> a)` with no comparator call,
6 to 7 times faster for 32 elements or more. For an order by a value of
each element, such as a field, [sortby](sym:types.Array.sortby) and
[sortby_reversed](sym:types.Array.sortby_reversed) call a script function n
times in place of about n log2(n) times.

`sort` is stable: elements that compare equal keep their relative order. It
is a merge sort: the first merge that moves elements allocates a buffer of
at most half the array (n/2 slots) for the time of the call; an array of 32
elements or less, or one already sorted, needs none. An already sorted array
costs n - 1 compares, a reversed one about n + n/32.

A `comparator` that does not define a consistent order (one that returns
different signs for the same pair) gives some permutation of the array; no
error is raised for it.

## Example

{{example:types.Array.sort}}

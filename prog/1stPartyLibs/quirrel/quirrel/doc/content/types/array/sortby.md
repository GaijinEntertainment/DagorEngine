---
params: [key]
see_also: [types.Array.sort, types.Array.sortby_reversed]
---

Sorts the array in place by the value that `key` returns for each element.

## Parameters

- `key` - `key(value, [index], [array])`, called once for each element, in
  order, before the sort; not called for an array of one element or none

## Return value

This array.

## Errors

Whatever `key` throws. Without a `_cmp` metamethod, a pair of keys that `<`
cannot compare throws `comparison between X and Y`. The call moves no element
before the sort is done, so after an error only what `key` wrote to the array
has changed.

## Notes

`key` gets exactly as many of the arguments listed above as it declares
parameters for, and never more.

The keys are compared the way [sort](sym:types.Array.sort) compares elements
without a comparator: numbers by value, strings lexicographically, and a table
or instance through its `_cmp` metamethod. The sort is stable: elements with
equal keys keep their relative order.

`arr.sortby(@(v) v.score)` gives the same order as
`arr.sort(@(a, b) a.score <=> b.score)`, but it calls a script function n
times in place of about n log2(n) times, so it is faster for all but the
smallest arrays. For more than one element, each call holds 2 n slots for
its keys and the order of the elements. The first merge that moves elements
adds a buffer of n/2 slots, for a total of at most 2.5 n slots. An array of
32 elements or less, or one already sorted by its keys, holds only 2 n
slots. Up to 32 elements, outside a metamethod, the slots are on the VM
stack and no array is allocated. One element or none needs no extra slots.

`key` must not change the size of the array: that throws `array resized
during sort operation`.

## Example

{{example:types.Array.sortby}}

---
params: [separators, skip_empty]
see_also: [types.String.split, string.split_by_chars]
---

Splits `str` into an array of strings at every byte that occurs in
`separators`.

## Parameters

- `separators` - the set of bytes that split `str`, each one its own
  separator, not a substring to match as a whole
- `skip_empty` - when true, a piece with zero bytes is left out of the
  result instead of being added as `""`

## Return value

An array of the pieces of `str`, in order, with every separator byte
removed. See [string.split_by_chars](sym:string.split_by_chars) for the exact
rules on leading, trailing and repeated separators.

## Errors

Throws `empty separators string` when `separators` is `""`.

## Notes

Takes 1 or 2 arguments: `skip_empty` is optional and defaults to false. Same
implementation and attributes as
[string.split_by_chars](sym:string.split_by_chars).

Contrast [types.String.split](sym:types.String.split), whose separator is a
literal substring matched as a whole rather than a set of individual bytes.

## Example

{{example:types.String.split_by_chars}}

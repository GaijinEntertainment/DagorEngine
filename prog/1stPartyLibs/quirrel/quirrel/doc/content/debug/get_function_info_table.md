---
see_also: [debug.get_function_decl_string, debug.type_mask_to_string, debug.doc]
---

Returns meta information about a function as table.

## Parameters

- `func` - the function to inspect

## Return value

A table with these keys:

- `functionName` - the function's name
- `native` - `true` for a native closure, `false` for a script closure
- `requiredArgs` - how many arguments must be passed, not counting `this`
- `argNames` - array of parameter names
- `argTypeMask` - array of type masks, one per entry in `argNames`; pair one with `type_mask_to_string` to read it
- `ellipsisArgTypeMask` - the type mask for a trailing `...` parameter, or `0` when the function does not take one
- `objectTypeMask` - the type mask accepted for the implicit `this`
- `returnTypeMask` - the declared return type mask
- `pure`, `nodiscard`, `fastcall` - the function's attributes
- `doc` - the docstring `doc` would return for the same function, or `null`

## Errors

`func` must already be a closure or a native closure; passing anything else
fails the parameter type check before this function runs.

## Notes

`pure` is `true` only for a function marked pure: a script function with the
`[pure]` attribute, a native bound as pure, or a closure that the host marked
with `sq_mark_pure_inplace`. The compiler does not infer it from the body.
`fastcall` is a native-only trait; a script closure never reports it.

A native bound through a type mask instead of a declaration string has no real
parameter names, so `argNames` is filled with placeholders `arg1`, `arg2`, and so
on. Most methods on the built-in types (`array`, `string`, ...) are bound this
way, since they typecheck arguments with a mask rather than a parsed decl string.

When `SQ_STORE_DOC_OBJECTS` is disabled, native declaration text is not
retained. Native functions then use the type-mask fallback, so declaration-only
parameter names and the declared return type are not available.

## Example

{{example:debug.get_function_info_table}}

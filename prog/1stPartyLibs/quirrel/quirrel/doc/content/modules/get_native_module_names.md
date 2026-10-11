---
see_also: [modules.on_module_unload]
---

Lists the name of every native module registered in this host.

## Return value

An `array` of `string`s, one per native module, in ascending alphabetical
order.

## Notes

The set of registered native modules depends on the host binary: a thin
embedding registers only a few, while a host built for full scripting support
registers many more. A script cannot rely on any particular name being
present, with one exception: the module system registers `modules` and
`types` together as part of setting itself up, so any host where
`require("modules")` succeeds also has `types`.

`require("quirrel.native_modules")` returns the same array directly, under a
reserved pseudo-filename, without needing to require `modules` first.

## Example

{{example:modules.get_native_module_names}}

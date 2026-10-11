---
see_also: [modules.get_native_module_names]
---

Registers a callback to run when the module system tears down.

## Parameters

- `arg1` - `arg1([is_app_closing])`, the callback to run. This function is
  bound with a type mask instead of a declaration string, so the VM cannot
  report the real parameter name; `arg1` is the callback, not a value passed
  to it.

## Return value

`null`.

## Errors

Throws when the callback takes more than one argument (besides `this`): a
registered callback must accept zero or one argument.

## Notes

The callback runs later, not immediately: once when the module manager is
destroyed (typically at process exit), with `is_app_closing` set to `true`,
and also once before a hot reload of a module or of all modules, with
`is_app_closing` set to `false`. Callbacks run in the order they were
registered.

Registering the exact same function object a second time has no extra effect:
only one call happens per distinct callback, however many times it was
registered.

## Example

{{example:modules.on_module_unload}}

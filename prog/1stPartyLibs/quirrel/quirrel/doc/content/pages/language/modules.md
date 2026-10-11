---
title: Modules
group: Language
order: 75
summary: `require`, `from`-`import`, module exports, `persist` and reloading.
---

A module is a `.nut` file loaded through `require` or `import`, not run
directly. Its code runs once, with `this` set to `null` and not to a table. The
value it `return`s becomes its exports. Every later `require`/`import` of the
same file gets that value back without running the file again. Everything else
the file declared stays private to it.

## require and require_optional

`require("modname")` reads, compiles and runs the named file the first time it
is requested, then returns its exports. A second `require` of the same name
returns the same exports without running the file again. A missing file throws.
`require_optional("modname")` is the same, except a missing file returns `null`.
Use it for an optional dependency such as a mod or a plugin that may not be
installed.

A module and the file that uses it:

```nut weapons.nut
let MAX_AMMO = 240

function reload(state) {
  return state.__merge({ ammo = MAX_AMMO })
}

function isEmpty(state) {
  return state.ammo <= 0
}

// only these three names leave the file; MAX_AMMO would be private without this
return {
  MAX_AMMO
  reload
  isEmpty
}
```

```nut main.nut
let weapons = require("weapons.nut")

let rifle = { ammo = 0 }
println(weapons.isEmpty(rifle))          // true
println(weapons.reload(rifle).ammo)      // 240
```

`require_optional` differs only in what a missing file does:

```nut
let saveApi = require_optional("mods/customSaveFormat.nut")
if (saveApi)
  saveApi.write(currentSlot)
```

The two files above are an illustration, not a runnable example. The test
runner runs each committed sample alone, from a directory it controls, so a
sample cannot `require` another committed file. For this reason every runnable
sample on this page uses a native module (`math`, `string`, and so on) and not a
file of its own.

## import and from ... import

`import` and `from ... import` do at compile time what `require` does at run
time. They work only as the first statements in a file: write every one of them
before any other code. After the compiler has passed that opening run of
imports, a later `import` line no longer parses as an import. It is read as a
plain expression that starts with the identifier `import`, and fails to compile
with `end of statement expected`.

`import "mod"` binds the whole exports object under the name `mod`;
`import "mod" as Name` binds it under `Name`.
`from "mod" import a, b as c` binds the individual exports `a` and `b` (renamed
`c`), and `from "mod" import *` binds every exported name at once.
Every bound name is a compile-time constant in the file that imported it, not a
variable. A plain member access resolves without a lookup at run time, and a
pure, constant export can fold into the call site.

`import *` on the same `weapons.nut` as above binds each export under its own
name, with no module object:

```nut main.nut
from "weapons.nut" import *

let rifle = { ammo = 0 }
println(isEmpty(rifle))                  // true
println(reload(rifle).ammo)              // 240
println(MAX_AMMO)                        // 240
```

Prefer the explicit `from "mod" import a, b` for maintainability.

{{example:language/modules-imports}}

## persist and keepref

A module's top level runs again on a hot reload, but the file's persisted state
must not reset because its code was rewritten and reloaded.
`persist(key, initializer)` runs `initializer` once and stores the result under
`key` (a string, unique within the file). Every later run of the same `persist`
call, including one after a reload, returns that stored value and does not call
`initializer` again. `key` has to be unique only among the `persist` calls in
this file, not across the whole project.

The stored value must be a value that can be changed in place: a table, an
array, a class, an instance or a userdata. `persist` keeps the same container
alive and changes what is inside it. An initializer that returns a plain number,
string or bool throws, because there is no way to update such a value from
outside without a new `persist` call that replaces it.

`keepref(value)` roots `value` for the lifetime of the module manager, then
returns it unchanged. Use it for an object that a module builds and installs
into an engine callback without keeping its own reference. Without `keepref`,
nothing holds such an object alive.

{{example:language/modules-persist}}

## Reloading

`require` is not part of the VM. It comes from `SqModules`, a library the host
builds on top of the VM. A host may leave it out. These functions do not appear
in this site's symbol reference because they are bound per module and not
registered in a table the VM can enumerate.

That library also makes reloading possible. A reload does not re-run one file.
It drops every loaded module and runs the entry point again, so everything
reachable through `require` runs a second time with fresh code. Values kept by
[persist](page:language/modules#persist-and-keepref) survive a reload. A reload
replaces behaviour without a reset of the state that the behaviour manages.

## Native modules

A native module is a table the host registers under a name. Script then
`require`s it like a file. The standard library modules on this site are all of
this kind.

The host declares each function with a declaration string and an optional docstring:

```cpp
{ debug_setdebughook, "setdebughook(hook: function|null)",
  SQ_DOC("Installs the given function as the VM debug hook; null clears it") },
```

The signature and the summary at the top of every symbol page on this site come
from that line. The VM reports them through
[debug.get_function_decl_string](sym:debug.get_function_decl_string) and
[debug.doc](sym:debug.doc). A binding registered without a declaration string
still works, but the VM can then report only placeholder parameter names. Some
pages here carry a note about this. `SQ_DOC` compiles the text away when the
build sets `SQ_STORE_DOC_OBJECTS` to 0, so shipping builds need not carry it.

## __name__

`__name__` is the module name of the running file: `"__main__"` for the entry
point script, or the name it was `require`d under.

## See also

[modules.get_native_module_names](sym:modules.get_native_module_names) lists
every native module a host has registered. Use it to check which optional
native modules (`io`, `datetime`, `async`, ...) a particular embedding shipped.
[modules.on_module_unload](sym:modules.on_module_unload) is the other
function of the module system's own native module, `require("modules")`.

# Modules Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.**

**A diff that changes GPU kernel code applies `REVIEW_SHADER_EMITTERS.md` (beside this file)
together with its own folder's checklist.** GPU kernel code is a function carrying
`[metal_kernel]`, `[spirv_kernel]` or an annotation whose name ends in `_shader` from `dasSpirv`,
`dasVulkan` or `dasMetal`; every `def` it reaches; the class and base classes a kernel method
belongs to, with their member declarations; and the structs their `@push_constant` members name.

**A diff that changes a `.das` under this folder listed in its module's `*_AOT_FILES` in
`modules/<M>/CMakeLists.txt`, or named in an `AOT_*_MODULE_FILES` variable of
`tests/aot/CMakeLists.txt` (repo root), and not marked `options no_aot`, states in the PR body
that the full `test_aot` lane (`preflight --full`, or a manual dispatch of `build.yml` on the
branch) ran green on the diff's head commit.** Per-PR CI compiles only the `tests/language` AOT
subset.

**In its own hand-written `initDependencies`, a C++ module calls `Module::require("<name>")` for
every in-tree module its CMake target links, and calls `initDependencies()` on each module that
call returns - in the same change as the link.** A module no other module requires is left
unloaded, and the linking module's imports are resolved before any of its code runs, so its next
load fails on the missing sibling.

**A diff that makes a `dasClangBind`-generated binding depend on another in-tree module declares
that dependency in the `bind_*.das` that generates the module's bindings - under
`modules/<M>/bind/` or `modules/dasClangBind/bind/` - `require_modules` when the binding uses the
other module's types, `require_load_modules` when only the shared library links against the
other's - never by hand in the generated file.** The binder emits `initDependencies` from those
two lists, and `require_modules` puts the other module's types into this module's type library,
so both modules binding the same C++ types resolve to one copy.

**A diff that hand-binds a function in a module's own C++ - a function the `bind_*.das` that
generates the module's bindings (under `modules/<M>/bind/` or `modules/dasClangBind/bind/`) would
otherwise generate - makes that binder's `skip_function` override return true for the function,
and deletes the function's generated registration from the module's `src/`, in the same
change.** Otherwise the generated registration sits beside the
hand-bound one and the module registers the name twice.

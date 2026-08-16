# rapidcheck

Property-based testing for C++ (QuickCheck-style): generate random inputs,
check invariants, and shrink a failure to a minimal counterexample. Pairs with
catch2 via the `rapidcheck/catch.h` integration header (use `rc::prop(...)`
inside a `TEST_CASE`).

## Info

- Upstream: https://github.com/emil-e/rapidcheck
- License: BSD 2-Clause (see `LICENSE.md`), (c) Emil Eriksson.
- Commit: `b2d9ed2dddefc4b84318d664b4f221eb792d89c7` (the upstream default
  branch HEAD at import time). rapidcheck has no tagged releases and is
  largely dormant, so a commit hash is used, not a version.
- Imported: `include/`, `src/`, `extras/catch/`, `LICENSE.md`.
- Dropped: `test/`, `examples/`, `ext/` (its own bundled Catch), `doc/`,
- Added: jam build (and this file)

## Build / usage constraints (test/host-only)

- **Requires RTTI.** `typeid` is used in `Any`/`ShowType`; the jamfile sets
  `Rtti = yes`, and any translation unit that includes rapidcheck headers and
  instantiates its generators must also build with `Rtti = yes`. (RTTI is a
  per-TU flag, so RTTI-off engine libs link against this fine.)
- **Uses C++ exceptions and the std library**. Like catch2, it is meant only for
  dev test executables built in `dev`/`dbg` (where exceptions are on).

## Consuming it

```
UseProgLibs += 3rdPartyLibs/rapidcheck ;
AddIncludes  =
  $(Root)/prog/3rdPartyLibs/rapidcheck/include
  $(Root)/prog/3rdPartyLibs/rapidcheck/extras/catch/include  # for rapidcheck/catch.h
;
Rtti = yes ;
```

Include `<catch2/catch_test_macros.hpp>` before `<rapidcheck/catch.h>` so the
integration header binds to Catch2 v3 rather than pulling the amalgamated
single-header.

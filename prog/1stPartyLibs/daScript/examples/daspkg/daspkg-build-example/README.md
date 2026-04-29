# daspkg build example

Demonstrates the daspkg workflow with C and C++ native modules.

## Setup

From this directory:

```bash
# Install packages from local paths (listed in .das_package)
daslang ../../../utils/daspkg/main.das -- --root . install

# Build native modules (compiles .shared_module files via CMake)
daslang ../../../utils/daspkg/main.das -- --root . build

# Run the example
daslang main.das
```

Expected output:

```
fast_inv_sqrt(4.0) = 0.49915358
fast_lerp(0, 10, 0.3) = 3
fast_clamp_int(15, 0, 10) = 10
counter(step=5), 2x increment = 10
```

## Updating after changes

If you modify the source packages under `../packages/`, re-sync and rebuild:

```bash
daslang ../../../utils/daspkg/main.das -- --root . update
daslang ../../../utils/daspkg/main.das -- --root . build
```

## Packages used

- **daspkg-example-c** — Pure C module using `daScriptC.h` (fast math functions)
- **daspkg-example-cpp** — C++ module using `ManagedStructureAnnotation` (Counter type)

Both packages live under `../packages/` and are installed as local dependencies.

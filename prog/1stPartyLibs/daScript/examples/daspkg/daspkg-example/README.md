# daspkg example — pure daslang

Demonstrates installing packages from git via daspkg.

## Setup

From this directory:

```bash
# Install all dependencies from .das_package
daslang ../../../utils/daspkg/main.das -- --root . install

# Run the example
daslang main.das
```

## What happens

1. `daspkg install` reads `.das_package` and installs two packages:
   - `daspkg-test-pure` — a simple daslang module with `greet()` and `add()`
   - `daspkg-test-deps` — a module that itself depends on `daspkg-test-pure` (transitive dependency)

2. Packages are cloned into `modules/` and recorded in `daspkg.lock`

3. `main.das` uses `require test_pure/test_pure` to call functions from the installed package

## Other commands

```bash
# List installed packages
daslang ../../../utils/daspkg/main.das -- --root . list

# Search the package index
daslang ../../../utils/daspkg/main.das -- --root . search test

# Remove a package
daslang ../../../utils/daspkg/main.das -- --root . remove daspkg-test-deps

# Install a specific package by URL
daslang ../../../utils/daspkg/main.das -- --root . install github.com/borisbat/daspkg-test-pure

# Install a specific version
daslang ../../../utils/daspkg/main.das -- --root . install github.com/borisbat/daspkg-test-versions@v1.0
```

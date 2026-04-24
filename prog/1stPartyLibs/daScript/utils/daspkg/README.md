# daspkg — daslang package manager

Package manager for [daslang](https://dascript.org/). Installs, updates, builds, and manages daslang modules from git repositories or a central package index.

## Quick start

```bash
# run daspkg
daslang utils/daspkg/main.das -- <command> [args...]

# install a package by name (from the index)
daslang utils/daspkg/main.das -- install daspkg-test-pure

# install a package by URL
daslang utils/daspkg/main.das -- install github.com/borisbat/daspkg-test-pure

# install a specific version
daslang utils/daspkg/main.das -- install github.com/borisbat/daspkg-test-versions@1.0

# install all dependencies listed in .das_package
daslang utils/daspkg/main.das -- install

# install globally (shared across projects)
daslang utils/daspkg/main.das -- install --global dasImgui
```

## Commands

| Command | Description |
|---------|-------------|
| `install <url\|name\|path>[@version]` | Install a package from git, index, or local path |
| `install` (no args) | Install all dependencies from `.das_package` |
| `remove <name>` | Remove an installed package |
| `update [name]` | Re-install at pinned version (re-clone, rebuild) |
| `upgrade [name]` | Upgrade to latest version |
| `list` | List installed packages |
| `search <query>` | Search the package index |
| `build` | Build all C/C++ packages (cmake) |
| `check` | Verify installed packages are present |
| `doctor` | Check environment (git, cmake, gh) |
| `introduce [url]` | Submit a package to the index via PR |
| `withdraw <name>` | Remove a package from the index via PR |

All package commands accept `--global` / `-g` to operate on global modules.

### Options

| Flag | Description |
|------|-------------|
| `--root <path>` | Project root (default: current directory) |
| `--force` | Force reinstall (overrides duplicate/version checks) |
| `--global`, `-g` | Operate on global modules in `{das_root}/modules/` |
| `--color` / `--no-color` | Enable/disable ANSI colored output |
| `--verbose`, `-v` | Print debug details (git commands, resolve steps) |
| `--json` | Machine-readable JSON output (`search`, `list`, `check`) |

## Global modules

Large packages (e.g. dasImgui) can be installed **globally** — once under `{das_root}/modules/` — shared across all projects using that SDK. Avoids redundant clones and builds.

```bash
daspkg install --global dasImgui        # install to das_root/modules/
daspkg list --global                    # list global packages
daspkg update --global dasImgui         # re-install at pinned version
daspkg remove --global dasImgui         # remove globally
```

- **Local install auto-uses global:** `daspkg install foo` checks the global lock file first. If compatible, records a reference instead of cloning.
- **Version mismatch:** errors with a suggestion. Use `--force` to install locally, or `--global` to update the global copy.
- **Shadow detection:** if a module exists both locally and globally, the local version wins (warning printed). Removing the local copy falls back to global.
- **CMake:** global packages with native builds get a `.daspkg_standalone` marker so the main CMake skips them during auto-discovery.

## `.das_package` manifest

Executable daslang script declaring metadata, version resolution, dependencies, and build info.

```das
options gen2
require daslib/daspkg

[export]
def package() {
    package_name("mymodule")
    package_author("username")
    package_description("What this module does")
    package_source("github.com/user/mymodule")
    package_license("MIT")
    package_tag("networking")
    package_min_sdk("0.4")
}

[export]
def resolve(sdk_version, version : string) {
    if (version == "" || version == "latest") {
        download_tag("v2.0")
    } else {
        download_tag("v{version}")
    }
    // alternatives: download_branch("main"), download_redirect("github.com/other/repo", "v3.0")
}

[export]
def dependencies(version : string) {
    require_package("github.com/user/dep-a", ">=1.0")
    require_package("github.com/user/dep-b")
}

[export]
def build() {
    cmake_build()       // or: custom_build("make all"), or: no_build()
}
```

All functions except `package()` are optional. A repo without `.das_package` gets a "dumb clone" — no version resolution, no deps, no build.

## Install flow

1. Shallow-clone from default branch
2. Run `.das_package` `resolve()` → checkout resolved tag/branch (or re-clone on redirect)
3. Move to `modules/<name>/`
4. Record in `daspkg.lock`
5. Install transitive dependencies
6. Auto-build if `.das_package` has `build()`

## Project layout

```
my_project/
  main.das
  .das_package                   # project manifest (lists dependencies)
  daspkg.lock                    # installed packages, versions, sources
  modules/
    <package_name>/
      .das_module                # provided by package author
      .das_package               # package manifest
      _build/                    # cmake build directory (if C++ package)
      *.shared_module            # built C++ output (if applicable)
    .daspkg_cache/               # index cache (gitignored)
    .daspkg_tmp/                 # temp dir during install (gitignored)

{das_root}/                      # daScript SDK root (for global modules)
  modules/
    .daspkg_global.lock          # global lock file
    <global_package>/
      .daspkg_standalone         # marker: built by daspkg, skip in CMake
```

## Lock file (`daspkg.lock`)

```json
{
  "sdk_version": "",
  "packages": [
    {
      "name": "mymodule",
      "source": "github.com/user/mymodule",
      "version": "1.0",
      "tag": "v1.0",
      "branch": "",
      "root": true,
      "local": false,
      "global": false
    }
  ]
}
```

- **root** — `true` if user-installed, `false` if transitive dependency
- **tag/branch** — resolved git ref
- **local** — installed from local path
- **global** — resolved from global install (no local copy)

The global lock file (`{das_root}/modules/.daspkg_global.lock`) uses the same format.

## Architecture

| File | Description |
|------|-------------|
| `main.das` | CLI entry point — parses args, dispatches to commands |
| `commands.das` | Command implementations: install, remove, update, upgrade, build, check, doctor |
| `index.das` | Package index: fetch, search, introduce, withdraw |
| `lockfile.das` | `LockFile` / `PackageEntry` structs, JSON serialization |
| `package_runner.das` | In-process `.das_package` compiler — compiles, simulates, extracts metadata |
| `utils.das` | Shared utilities: `run_cmd`, `force_rmdir`, path helpers |
| `daslib/daspkg.das` | API module that `.das_package` scripts `require` |

The **package runner** compiles `.das_package` scripts in-process using `compile_file` + `simulate` + `invoke_in_context`. It calls exported functions and reads state from `daslib/daspkg` module globals via `get_context_global_variable`.

## Tests

| File | Count | Type |
|------|-------|------|
| `test_daspkg.das` | 151 | Unit tests (local operations, parsing, lock file, package_runner) |
| `test_daspkg_git.das` | 70 | Integration tests (git clone, version resolve, index, global modules) |

```bash
# unit tests — fast, no network
daslang dastest/dastest.das -- --test utils/daspkg/test_daspkg.das

# integration tests — requires network
daslang dastest/dastest.das -- --test utils/daspkg/test_daspkg_git.das
```

### Test repositories (GitHub)

| Repository | Purpose |
|------------|---------|
| [`borisbat/daspkg-test-pure`](https://github.com/borisbat/daspkg-test-pure) | Pure daslang module |
| [`borisbat/daspkg-test-versions`](https://github.com/borisbat/daspkg-test-versions) | Module with version tags (v1.0, v2.0) |
| [`borisbat/daspkg-test-deps`](https://github.com/borisbat/daspkg-test-deps) | Module with transitive dependency |
| [`borisbat/daspkg-index`](https://github.com/borisbat/daspkg-index) | Central package index |

## Design rationale

### Why git-based (like Go modules)?
- Packages are git repositories — no centralized registry server to maintain
- Version = git tag. `resolve()` maps version requests to tags.
- Index is a curated JSON file in a git repo. Authors add via PR (`daspkg introduce`).

### Why executable manifests?
- Authors can put version-conditional logic in `resolve()` and `dependencies()`
- Same pattern as `.das_module` — familiar to daslang authors
- daspkg provides registration functions via `daslib/daspkg`; `.das_package` just calls them

### Why per-project by default?
- Game projects need reproducible builds — `modules/` is self-contained
- Global install (`--global`) for large shared modules (dasImgui, etc.)
- Runtime already scans both `das_root/modules/` and project `modules/`

### Version model
- **Package version** — semver, resolved via `.das_package` `resolve()`
- **SDK version** — `resolve()` receives it, can return different tags per SDK
- **Dependency constraints** — operators `>=`, `>`, `<=`, `<`, `=`, comma AND: `">=1.0,<2.0"`
- **Diamond deps** — first installed wins (single `require` namespace). Manual upgrade with `--force`.

### Transport
- Only external dependency: `git` CLI. No HTTP library needed.
- Shallow clones (`--depth 1`) for speed.
- Local installs: filesystem copy, no git.

## Requirements

- **git** — required for all remote operations
- **cmake** — required for building C/C++ packages
- **gh** (GitHub CLI) — optional, only for `introduce`/`withdraw`

Run `daspkg doctor` to check your environment.

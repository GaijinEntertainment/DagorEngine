# daspkg Code Review Checklist

Run this list on every daspkg change before it ships. Every entry is checkable against a diff.

**This file reviews itself: a rule a reviewer cannot apply as written is a defect of this
file.** Mark it like any other finding — a checklist defect blocks nothing, but its fix (a
rewrite or a move, never silent tolerance) lands in the same batch as the round's other fixes.

## Tests

**Run the unit suite on every change:**

```text
bin/daslang dastest/dastest.das -- --test utils/daspkg/test_daspkg.das
```

Fast, no network, runs interpreted. A daspkg change without a green unit run is a defect.

**Run the integration suite when install, resolve, index, or git behavior changes:**

```text
bin/daslang dastest/dastest.das -- --test utils/daspkg/test_daspkg_git.das
```

Needs network (the `borisbat/daspkg-test-*` fixture repos).

**A release-path change is verified on macOS, or the review says it was not.** The release
layout forks per platform (`.app` bundle vs flat directory); the only CI on these suites is
the nightly (`nightly_daspkg_index.yml`: Linux unit+git, macOS unit), so a per-change run is
still the review's job — a change green on one platform has shipped red on the other before.

**A new command or flag lands with its test cell, its `print_usage` line, and its README table
row in the same change.**

## Behavior

**A release always mints the tune sidecar; `--quick` is the only inherit, and it accepts only a
complete one.** A bundle that ships an exe without a sidecar beside it is a defect.

**`release_include_if_missing` files are user-owned after initialization.** A release path that
overwrites or deletes one, on any platform, is a defect; `.daspkg_release.manifest` is written
on every platform.

**Unit cells touch only local fixtures.** A `test_daspkg.das` cell that reaches the network is
a defect — network coverage belongs in `test_daspkg_git.das`.

**Anything interpolated into a shell command is validated first** (`shell_unsafe` /
`is_safe_pkg_name`). A new interpolation site without its check is a defect.

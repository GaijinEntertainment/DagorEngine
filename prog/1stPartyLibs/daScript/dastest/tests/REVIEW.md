# dastest Tests Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture
doc: `../README.md`.

**A dastest fixture - a file that exists to be run by another test rather than collected as a
test of its own - starts its name, or its directory's name, with `_`** - dastest skips a `_`
name when it scans a directory, so a fixture never runs as a test of its own.

**A new test for dastest itself that spawns a process or asserts an exit code is run on Windows
and Linux before it lands, or the PR body names each of those two it was not run on** - the
`extended_checks` workflow runs the dastest tests on macOS alone per pull request; Windows and
Linux see them nightly.

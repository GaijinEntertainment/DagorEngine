# Utils Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture
doc: `CLAUDE.md` (repo root).

A tool is a directory holding a program and the files only that program uses. A directory is
a tool when `CMakeLists.txt` (beside this file) or the repo root's `CMakeLists.txt` builds or
ships a program it holds, or when `CMakeLists.txt` (beside this file) runs a `DAS_UTILS_TO_TEST`
suite against such a program, wherever that directory sits; a directory under `utils/` is also
a tool when its `.das_package` declares a program with `release_main` (its own) or
`release_program` (a companion), or when a CMake target or a `.github/workflows/` step (repo
root) names in its command a program it holds. A directory holds a program when the program's
entry file sits in it - or sits in a child folder of it, when the directory itself holds a
`README.md`. A change under `common/` (beside this file) is a change to every tool that requires
it.

A tool is shipped when an `install(...)` rule in `CMakeLists.txt` (beside this file) or the
repo root's `CMakeLists.txt` puts its program in the bundle - a built executable
(`install(PROGRAMS ...)` for a `DAS_UTILS_SHIPPED_EXES` entry, `install(TARGETS ...)` for a
C++ target), or the entry file copied as source (`install(FILES ...)` or
`install(DIRECTORY ...)`).

An arm is one test case that a test runner executes and reports pass or fail for on its own,
in any language: a `dastest` `[test]` function and each `t |> run(...)` case inside that
function are arms, and so is each `test(...)` case of a JavaScript runner (`node:test`,
Playwright). An arm's load-bearing assertions are the ones that prove the change, never a
skip-path assertion.

A CI row is a workflow step whose command runs the arm, directly or through a process it
spawns. An assertion no CI row can run is one that needs something no pull-request lane's
runner image provides - a GPU, a host tool the image does not carry, a network service, a
credential - or one the arm returns or skips before on every such runner; whether a row runs
the arm today does not decide it. An arm that skips unless a host tool is present has
assertions a CI row can run when the pull-request lane's runner image carries that tool, and
the change names that lane. One arm can hold assertions of both kinds.

**A changed file that belongs to a tool, wherever the tool sits, is reviewed with that tool's
own `REVIEW.md`, where one exists, as well as with this checklist.**

**A diff that adds a `[test]` file covering a change to a tool puts that file under the tool's
directory; a test covering a change under `common/` goes under `common/`, and a test covering
two tools goes under either one.**

**A diff that changes how a tool builds a `.dlim` from a gguf, how one loads it, or what
identifies one - the fields that decide whether two `.dlim`s are the same image - answers to
`modules/dasLLAMA/REVIEW.md` (repo root) too.**

**A file that belongs to a tool and carries `options _dasllama_internal`, wherever the diff
puts it, applies `modules/dasLLAMA/REVIEW.md` (repo root) too.**

**Weakening `REVIEW.das` (beside this file) is a defect: dropping a check, narrowing what a check
walks, or rewriting a finding text so it no longer names what failed.**

**A diff that drops a tool from `DAS_UTILS_SHIPPED_EXES` (`CMakeLists.txt`, beside this file)
while keeping that tool's directory records the decision to stop shipping it in that tool's
own `README.md`, wherever that directory is, in the same change** - `REVIEW.das` cannot see a
removed entry, and it reads each shipped tool's `README.md` for the standing decision.

**A diff that deletes a tool named in `DAS_UTILS_SHIPPED_EXES` leaves a comment line beside
the list naming the removed exe, in the same change.**

**An arm the diff adds or changes that covers a change to a tool, whose load-bearing
assertions a CI row can run against the change, ships with a CI row that executes those
assertions on every pull request, wherever the diff puts the arm, added in the same change
when no row does that yet.** A row that only compile-checks the arm (`dastest --compile-only`)
does not execute them.

**An arm the diff adds or changes that covers a change to a tool, whose load-bearing
assertions no CI row can run, ships with a row that compile-checks the file the arm sits in -
`dastest --compile-only` - added in the same change if no row already compile-checks that
file.**

**An arm the diff adds or changes that covers a change to a tool, whose load-bearing
assertions no CI row can run, records in the PR description an executed run against the build
the diff produces - the machine the assertions ran on, why no CI row can run them, and the pass
count - or cites the log of a workflow run on this diff's branch showing those assertions ran.**
A run against an already-deployed artifact proves nothing about the diff under review.

**A diff that adds or renames a key in a `watchdog.json` that belongs to a tool, wherever the
tool sits, names a `WatchdogConfig` field in `watchdog/watchdog.das`, in the same change** -
the supervisor refuses to start on an unknown key.

**A diff that adds a front to a shipped tool adds a `run_check` row starting that front to
`ci/smoke_test_bundle.sh` (repo root), in the same change.** A front is a command-line mode
that makes a program serve a different role or protocol. The bundle gate proves only the
fronts it starts.

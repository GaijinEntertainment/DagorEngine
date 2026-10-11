# dastest Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`README.md`.

**A diff under this folder applies `utils/REVIEW.md` (repo root) too** - dastest ships as a
prebuilt exe, the `DAS_UTILS_SHIPPED_EXES` entry in `utils/CMakeLists.txt` (repo root).

**A dastest `[test]` file, and any file one of them spawns, wherever the diff puts it, answers
to the `tests/` subfolder's checklist.**

**A diff that adds, renames, or removes a public function in `review_gate.das` updates the
`review_gate.das` section of `README.md` in the same change.**

**A diff that adds a flag to `dastest_clargs.das`, or changes an existing flag's name, default,
or `@clarg_doc` text, updates that flag's bullet in `README.md`'s argument list in the same
change** - the list carries the user-facing flags; a flag it does not list gets a bullet only
when the diff makes it user-facing.

**A new test for dastest itself goes under `tests/` (this folder) with no registration** - the
extended lane runs that whole directory on every PR.

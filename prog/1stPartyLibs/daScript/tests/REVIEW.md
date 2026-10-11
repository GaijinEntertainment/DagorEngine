# Tests Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`README.md` (this folder).

**A test answers to its kind's checklist wherever under this folder the diff puts it:**
a test that asserts on `require` path lookup, module compile order, or files under the module
cache folder - `module_cache/REVIEW.md`;
a test that runs a dasMetal kernel class or creates any Metal object - `metal/REVIEW.md`;
an MSL emitter fixture (a test that compiles a kernel and checks the emitted MSL) or a change to
`msl/test_msl_census.das` - `msl/REVIEW.md`;
a SPIR-V emitter fixture (a test holding a `*_words` function that compiles a shader and returns
its emitted words) or a change to `spirv/test_census.das` - `spirv/REVIEW.md`;
a test that calls `tick_debug_agent` - `debug_agent/REVIEW.md`.

**A diff that widens the `dasbind` skip in `.das_test` or drops one of the `[extern]` probe
functions or any of its assertions from `dasbind/test_extern_abi.das` is a defect.** The suite is the only
check of which register or stack slot an interpreted `[extern]` call puts each argument in - the
JIT never takes that path - so coverage lost there reports green on every lane.

**A changed `.das` file under this folder that sits in an `AOT_*_FILES` variable of
`tests/aot/CMakeLists.txt` (by glob or by name) and is not marked `options no_aot` passes its
`dastest` run and `test_aot` (and the per-PR AOT subset binary too when it sits under
`language/`), and the PR body names the run that showed the `test_aot` pass: a local
`preflight --full`, a `--target run_tests_aot` build, or a CI job link.** Per-PR CI builds only
the AOT subset binary (`tests/aot/CMakeLists.txt`), so the PR's own checks do not prove
`test_aot`.

**A new `.das` file under `tests/` that a glob in `tests/aot/CMakeLists.txt` puts in an
`AOT_*_FILES` variable and that does not compile on its suite's `test_aot` lane is filtered out
of that variable or marked `options no_aot`, in the same change, with the reason in a comment
beside the filter or the option.** The glob picks a new file up silently.

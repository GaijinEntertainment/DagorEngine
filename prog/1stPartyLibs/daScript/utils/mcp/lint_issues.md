# Lint Issues to Fix

Discovered while running `lint` tool across daslib/. These are false positives or missing features in `daslib/lint.das`.

## False positives

### Tuple destructuring variables flagged as "used with underscore prefix"
- `let (_, succ) = table.emplace(...)` generates a variable named `` _`succ ``
- Lint sees the `_` prefix and warns: "variable is used and should be named without underscore prefix"
- **Fix:** skip variables whose name contains a backtick (`` ` ``) — those are compiler-generated tuple field names
- **Files affected:** `daslib/aot_cpp.das:609,621`

### "can be made const" for variables filled via out-ref parameter
- `var st : FStat; stat(f, st)` — `st` is filled by `stat()` via reference parameter
- Lint sees no direct `=` assignment and suggests `let`, but `let` would fail since `stat` writes into it
- **Root cause:** `builtin_stat` and `builtin_fstat` in `module_builtin_fio.cpp` were bound with `SideEffects::modifyExternal` instead of `SideEffects::modifyArgumentAndExternal` — FIXED
- After C++ rebuild, lint should no longer report this false positive
- **Files affected:** `modules/dasLiveHost/live/live_watch_boost.das:49`

## Notes

- `daslib/just_in_time.das` fails to compile (needs LLVM) — skip
- `daslib/debug_eval.das:402` — PERF003 character_at, intentionally unfixed
- `daslib/json.das:258` — PERF006 tiny hex loop, intentionally unfixed
- `daslib/regex.das:665,1514` — PERF006 in regex compilation, intentionally unfixed
- `daslib/rst.das` — PERF003 character_at in function_name, intentionally unfixed

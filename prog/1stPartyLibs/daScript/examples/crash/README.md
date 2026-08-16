# crash — deliberate failure generator

One failure per run, selected by `--kind`, so the crash-capture stack can be checked against a
known shape. It exists because several failure modes used to terminate the process while printing
nothing at all, and "nothing happened" is indistinguishable from "nothing was captured".

The interesting output is not that a crash occurred — it is **which tier saw it**.

## Running

```bash
daslang utils/daspkg/main.das -- --root examples/crash install   # build the native module
daslang examples/crash/main.das -- --kind null_write             # interpreter
daslang -jit examples/crash/main.das -- --kind null_write        # JIT
daslang examples/crash/main.das -- --show-help                   # every kind
```

Enum values keep underscores (`--kind das_panic`); only flag names hyphenate.

`--delay N` runs for N seconds before failing, to mimic a long-lived service. `--code N` sets the
code for `das_exit_code` / `das_fio_exit`.

## What each kind produces

Measured on Windows, interpreter and JIT. `rc` is the shell's view; the true Windows status is in
parentheses where it differs usefully.

| kind | rc | reported by |
|---|---|---|
| `none` | 0 | — (control case) |
| `das_panic` | 1 | host: `EXCEPTION: …` |
| `das_verify` | 1 | host: `EXCEPTION: assert failed, …` |
| `das_array_bounds` | 1 | host: `EXCEPTION: array index out of range` |
| `das_exit_code` | 1 | **nothing — silent by design** |
| `das_fio_exit` | 1 | `exit(1) called from <file:line>` + das stack |
| `null_write` / `null_read` / `wild_read` | 139 | handler: `CRASH: ACCESS_VIOLATION` + stack + source line |
| `divide_by_zero` | 139 / 127 | handler: `CRASH: INT_DIVIDE_BY_ZERO` — see follow-up below |
| `stack_overflow` | 127 | handler: `CRASH: STACK_OVERFLOW`, no stack (declined by design) |
| `uncaught_exception` | 127 | handler: `CRASH: UNHANDLED_CPP_EXCEPTION` + stack |
| `native_terminate` / `native_abort` | 3 | handler: `CRASH: std::terminate` / `abort() (SIGABRT)` |
| `worker_null_write` | 139 | handler: full stack including the das frame |
| `worker_das_panic` | 127 | `JOB EXCEPTION: … at <file:line>` |

## The three tiers, and what each cannot see

| failure class | in-process handler | WER local dump | host `EXCEPTION:` |
|---|---|---|---|
| SEH faults (AV, div0) | yes — stack **and source lines** | yes | — |
| stack overflow | no — declined by design | yes | — |
| fast-fail (`abort`, `terminate`) | yes — via `set_terminate` + `SIGABRT`, which run before the fast-fail | yes | — |
| das panic, main thread | — | no (no fault) | yes |
| das panic, jobque worker | — | no | yes |
| `main` returns non-zero | — | no | no |

The handler (`src/hal/crash_handler.cpp`) is on by default and is the better instrument for faults:
it prints source file/line, which a minidump does not. It reaches the fast-fail family too, through
`set_terminate` and `SIGABRT` — the only window before `__fastfail`, which bypasses the SEH filter
by design. The one class it declines is stack overflow, where there is no stack left to walk.

Every path hands back to the default handling after printing rather than exiting, so the WER local
dump is still collected. Printing must not cost the dump: for `abort`/`terminate` the dump was
historically the only witness, and a stack trace is a poor trade for losing it.

Neither tier sees a das panic — that is the host's job.

Ship symbols with `release_include_symbols()` in `.das_package` so a dump from a deployed tree
resolves (`cdb -y <bundle>\symbols`); see `skills/daspkg.md`.

## Follow-ups (not yet fixed)

**`divide_by_zero` exits with a different status per tier** — `0xC0000005` interpreted,
`0xC0000094` JIT-ed, for the same reported exception. The interpreter takes a *second* fault (an
access violation) after the divide-by-zero, and exits with that one. The failure is reported either
way, so this is a wrong exit code rather than a silent failure — deliberately left for later.

**Secondary faults are invisible.** `crash_handler.cpp`'s `in_handler` latch is set on first entry
and never cleared, so every fault after the first is silently swallowed. That is what hides the
divide-by-zero's second fault, and it hides any crash-after-crash. A bounded counter (report the
first N, then latch) would keep the anti-recursion protection without the blindness. These two are
the same investigation.

## Layout

- `main.das` — argument parsing and the nested call chain (several das frames, so a captured stack
  shows whether frame recovery works)
- `crash_kinds.das` — the kinds and what each one does
- `packages/dascrash/` — the native module, installed via `daspkg install` from this local path.
  `volatile` and the `g_sink` accumulator in `module_crash.cpp` are load-bearing: without them the
  optimizer folds the null store away or turns the recursion into a loop, and the harness stops
  reproducing the shape it claims to.

# debug_agent tests Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`daslib/ARCHITECTURE.md` (repo root).

**A test in this folder that calls `tick_debug_agent` starts every thread its agent's
callbacks wait on before that call, and holds each such thread on an atomic flag until the
callback has entered.** A callback runs under the agent registry lock and `new_thread` cannot
create its context without that same lock, so the wait never ends.

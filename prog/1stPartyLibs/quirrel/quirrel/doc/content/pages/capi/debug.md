---
title: Debug interface
group: C API
order: 136
summary: The hooks a debugger, profiler or coverage tool attaches to.
---

The hooks a debugger, profiler or coverage tool attaches to.

`sq_setdebughook` installs a callback that the VM calls as the script runs:
when a function is called, when it returns, and, if line hooks are enabled,
when execution comes to a new source line. A call or a return also starts a
new line, even if the line number stays the same.

This hook is enough to build a stepping debugger. `sq_stackinfos` gives one
frame of the call stack, and a stack trace is made from these frames.

Line information has a cost and can be turned off.

{{capi:debug}}

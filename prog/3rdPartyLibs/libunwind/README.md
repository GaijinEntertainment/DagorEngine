# libunwind

A portable C library for determining the call chain of program threads
and resuming execution at any point in that chain (stack unwinding,
backtraces, setjmp/longjmp support). Used here for the aarch64/Android
target (see jamfile).

Upstream: libunwind, https://github.com/libunwind/libunwind
Vendored version: 1.8.1 (see include/config.h PACKAGE_VERSION).
License: MIT.

Local modification: `access_mem()` with validation enabled reads memory
through the kernel (`unw_mem_read_safe()`: `process_vm_readv(2)` with a
pipe-based fallback, see `src/mi/Gaddress_validator.c`) instead of
probe-then-load, so concurrently unmapped pages (e.g. ART discarding JIT
code) fail the unwind step instead of crashing with SIGSEGV.

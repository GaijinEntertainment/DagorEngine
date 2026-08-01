# EASTL

Electronic Arts Standard Template Library, a C++ container/algorithm library
used throughout Dagor in place of (or alongside) the standard library.
Includes EABase (platform-independent types and feature macros), vendored
under `include/EABase`.

Upstream:
- EASTL: https://github.com/electronicarts/EASTL, vendored version 3.21.23
- EABase: https://github.com/electronicarts/EABase, vendored version 2.09.12

`mem.cpp`, `snprintf.cpp`, `assert_e2k.cpp`, `eastl.natvis` and
`eastl.natstepfilter` are Dagor-authored glue/integration code, not part of
upstream EASTL.

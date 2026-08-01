# fast_float

Header-only C++ library implementing fast `from_chars` functions for
parsing floating-point and integer numbers from text.

Upstream: fast_float, https://github.com/fastfloat/fast_float
Vendored version: 6.6.1, with local fixes on top (arm64 clang, LCC/e2k,
32-bit x86 FPU "0" vs "-0" parsing).

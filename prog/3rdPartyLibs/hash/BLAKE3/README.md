BLAKE3 is a cryptographic hash function: fast, parallelizable, and a
Merkle-tree-based design.

Upstream: BLAKE3, https://github.com/BLAKE3-team/BLAKE3, vendored version 1.8.2
(see BLAKE3_VERSION_STRING in blake3.h). Only the portable/dispatch/SIMD C and
assembly sources are vendored here (no Rust bindings, test vectors, or docs).

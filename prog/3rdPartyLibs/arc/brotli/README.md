Brotli is a generic-purpose lossless compression algorithm and library
by Google, combining a modern variant of the LZ77 algorithm, Huffman
coding, and 2nd order context modeling.

Upstream: brotli, https://github.com/google/brotli
Vendored version: 1.2.1 (see VERSION)

Local changes (keep them when syncing):

- enc/matching_tag_mask.h: no SSE2 path on ARM64EC, which defines _M_AMD64
  but cannot compile x86 intrinsics with clang.

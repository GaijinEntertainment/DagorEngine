# src/misc Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`ARCHITECTURE.md`.

**A second-tier kind the arm does not recognize is not compute.** `JobQue::is_slow_tier_compute`
returns true only for a perflevel name on the list it carries ("Performance", "Super"); a diff
that treats an unmatched name as compute is a defect.

**A core-count query on a topology it cannot read answers 0.** `JobQue::get_num_perf_cores`
returns 0 when the platform reports no tier structure; a diff that makes it guess is a defect.

**A fixed-length copy in `luau_float2string.cpp` stays fixed-length.** `DAS_F2S_MEMCPY` and
`DAS_F2S_MEMSET` write the constant `sizefast` bytes, not the requested `size`; a diff that
switches either to `size` is a defect - the constant length is what the compiler inlines.

**A diff that raises a `sizefast` constant in `luau_float2string.cpp` also raises
`DAS_F2S_BUFFER_SIZE` (`include/daScript/misc/float2string.h`), which sizes every caller's
buffer.**

**Never classify a float in `luau_float2string.cpp` through libc - `isfinite`, `isnan`,
`isinf`, `fpclassify`, `signbit` - read the IEEE bits instead: the exponent bits for NaN and
infinity, the sign bit for the sign.** A build with `-ffinite-math-only` folds a NaN or infinity
test to a constant.

**A diff in `daScriptC.cpp` that hands a writing `SerializationStorageVector`'s `buffer` out to
a caller calls `flush()` on the storage first.** The writer grows the vector by doubling and
counts the written bytes in `writePos`, so before a flush the vector is longer than the stream
and the caller gets trailing garbage.

**Weakening `REVIEW.das` (beside this file) is a defect: dropping a check, narrowing the files or
lines a check scans, or rewriting a finding text so it no longer names what failed.**

**A diff that makes `dep_recorder.cpp` write more than one rule per depfile is a defect** -
several rules naming one target is ninja's deprecated `depfilemulti`, and a parallel sweep whose
workers each wrote their own would produce one per worker. A process merges into the rule already
there, under the lock file beside it.

**A diff that has `dep_recorder.cpp` record anything but a source the compile read is a
defect** - a file read at runtime, a directory listing, a build artifact. A check's inputs are
its require graph; an artifact belongs to another edge, so naming one makes ninja build that
edge, and an artifact the build rewrites leaves the check dirty for good.

**A diff that drops the exists-at-flush filter in `dep_recorder.cpp` is a defect**: a path under
a temp directory the run removed would be named, and ninja treats a missing dependency as dirty
forever.

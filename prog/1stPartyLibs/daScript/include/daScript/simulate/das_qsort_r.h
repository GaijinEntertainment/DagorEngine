#pragma once

#include "daScript/misc/platform.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace das
{

// Header-only byte-pointer algorithms with inline comparator. std::sort,
// std::nth_element, std::partial_sort, std::make_heap etc. template on the
// iterator type and cannot operate on opaque void* data with runtime-known
// element width. The daslang any-cblock binding path (user-defined struct
// types going through __builtin_*_any_cblock in module_builtin_runtime_sort.cpp)
// needs algorithms that take (void*, nel, [n,] width, Compare) — these
// implementations fill that gap.
//
// Each algorithm was picked from a bake-off (examples/sort/bench_sort_family.cpp)
// against std::* and against alternative implementations. Notes per op:
//
//   - das_qsort_r            — pdqsort-lite (Hoare partition, median-of-3,
//                              insertion-sort tail at 24, heapsort fallback on
//                              2*log2(N) depth blowup). Won over smoothsort
//                              (Anton Yudintsev's musl port, prior implementation)
//                              by 51-66% across element sizes, and over plain
//                              introsort with Lomuto partition by 7-10%.
//   - das_nth_element_r      — introselect with Hoare partition + median-of-3.
//                              Beats std::nth_element by ~30% across all sizes
//                              and types (vs prior Lomuto introselect which
//                              merely matched std).
//   - das_partial_sort_r     — heap-of-topn scan: build max-heap of first N,
//                              replace+sift while scanning the rest, drain.
//                              Matches std::partial_sort; 4-7% FASTER than std
//                              on large struct elements; 17× faster than the
//                              prior nth_element+sort strategy when topn << N.
//   - das_sift_down_r /      — hole-sliding sift-down with memcpy. Saves parent
//     das_make_heap_r /        value to a stack buffer, slides larger children
//     das_pop_heap_r           up into the hole (1 memcpy per level vs 3 memcpys
//                              per byte_swap), places value at final hole. Floyd
//                              bottom-up for make_heap. 35% faster than the
//                              prior swap-based variants; matches std::make_heap;
//                              7% faster than std for >32-byte elements.
//   - das_push_heap_r        — classic sift-up with byte_swap. No bake-off
//                              candidate measurably faster.

// byte_swap — sized dispatch for common element widths plus a chunked-memcpy
// fallback. Constant-size memcpy at w ∈ {4,8,16,32,64,128,256} is inlined by
// every supported compiler to a single SIMD load/store pair per direction;
// 30-140× faster than the generic loop for workhorse-type widths (measured
// at examples/sort/bench_byte_swap.cpp). Wider elements fall through to the
// chunked path.
static inline void byte_swap(void *pa, void *pb, size_t width)
{
    unsigned char *a = (unsigned char *)pa;
    unsigned char *b = (unsigned char *)pb;
    switch (width) {
        case 4:   { uint32_t t; memcpy(&t, a, 4); memcpy(a, b, 4); memcpy(b, &t, 4); return; }
        case 8:   { uint64_t t; memcpy(&t, a, 8); memcpy(a, b, 8); memcpy(b, &t, 8); return; }
        case 16:  { unsigned char t[16];  memcpy(t, a, 16);  memcpy(a, b, 16);  memcpy(b, t, 16);  return; }
        case 32:  { unsigned char t[32];  memcpy(t, a, 32);  memcpy(a, b, 32);  memcpy(b, t, 32);  return; }
        case 64:  { unsigned char t[64];  memcpy(t, a, 64);  memcpy(a, b, 64);  memcpy(b, t, 64);  return; }
        case 128: { unsigned char t[128]; memcpy(t, a, 128); memcpy(a, b, 128); memcpy(b, t, 128); return; }
        case 256: { unsigned char t[256]; memcpy(t, a, 256); memcpy(a, b, 256); memcpy(b, t, 256); return; }
    }
    unsigned char tmp[256];
    while (width) {
        size_t chunk = sizeof(tmp) < width ? sizeof(tmp) : width;
        memcpy(tmp, a, chunk);
        memcpy(a, b, chunk);
        memcpy(b, tmp, chunk);
        a += chunk; b += chunk; width -= chunk;
    }
}

// sized_memcpy — same sized dispatch idea for plain dst<-src copies. Used
// by das_sift_down_r's hole-sliding inner loop so the per-level memcpy at
// known struct widths becomes a single inlined SIMD load/store pair instead
// of a runtime-width libc memcpy call. Same widths as byte_swap.
static inline void sized_memcpy(void *dst, const void *src, size_t width)
{
    switch (width) {
        case 4:   memcpy(dst, src, 4);   return;
        case 8:   memcpy(dst, src, 8);   return;
        case 16:  memcpy(dst, src, 16);  return;
        case 32:  memcpy(dst, src, 32);  return;
        case 64:  memcpy(dst, src, 64);  return;
        case 128: memcpy(dst, src, 128); return;
        case 256: memcpy(dst, src, 256); return;
    }
    memcpy(dst, src, width);
}

// Hole-sliding sift-down. Saves parent value to stack buffer, slides larger
// children up into the hole one memcpy at a time, places parent value at the
// final hole position. For elements wider than the stack buffer, falls back
// to a classic swap-based sift (uncommon — most workhorse + user-struct types
// fit). Used by make_heap (Floyd bottom-up), pop_heap, and the partial_sort
// heap drain.
template <typename Compare>
static inline void das_sift_down_r(unsigned char *data, size_t parent, size_t nel, size_t width, Compare cmp)
{
    if (2 * parent + 1 >= nel) return;
    unsigned char tmp[256];
    if (width > sizeof(tmp)) {
        // Swap-based fallback for very wide elements.
        while (true) {
            size_t left = 2 * parent + 1;
            if (left >= nel) return;
            size_t right = left + 1;
            size_t largest = parent;
            if (cmp(data + parent * width, data + left * width)) largest = left;
            if (right < nel && cmp(data + largest * width, data + right * width)) largest = right;
            if (largest == parent) return;
            byte_swap(data + parent * width, data + largest * width, width);
            parent = largest;
        }
    }
    sized_memcpy(tmp, data + parent * width, width);
    size_t hole = parent;
    while (true) {
        size_t left = 2 * hole + 1;
        if (left >= nel) break;
        size_t larger = left;
        if (left + 1 < nel && cmp(data + left * width, data + (left + 1) * width)) {
            larger = left + 1;
        }
        if (!cmp(tmp, data + larger * width)) break;
        sized_memcpy(data + hole * width, data + larger * width, width);
        hole = larger;
    }
    sized_memcpy(data + hole * width, tmp, width);
}

template <typename Compare>
inline void das_make_heap_r(void *base, size_t nel, size_t width, Compare cmp)
{
    if (nel <= 1) return;
    unsigned char *data = (unsigned char *)base;
    // Floyd bottom-up: sift_down from the last non-leaf node.
    for (size_t i = nel / 2; i-- > 0;) {
        das_sift_down_r(data, i, nel, width, cmp);
    }
}

template <typename Compare>
inline void das_push_heap_r(void *base, size_t nel, size_t width, Compare cmp)
{
    // Assumes caller has just appended the new element at index (nel-1) and
    // now wants the heap property restored. Classic sift-up with byte_swap.
    if (nel <= 1) return;
    unsigned char *data = (unsigned char *)base;
    size_t child = nel - 1;
    while (child > 0) {
        size_t parent = (child - 1) / 2;
        if (!cmp(data + parent * width, data + child * width)) return;
        byte_swap(data + parent * width, data + child * width, width);
        child = parent;
    }
}

template <typename Compare>
inline void das_pop_heap_r(void *base, size_t nel, size_t width, Compare cmp)
{
    // Swap root with last, then sift_down over the reduced range [0..nel-2].
    // Caller is expected to pop / drop the last slot after.
    if (nel <= 1) return;
    unsigned char *data = (unsigned char *)base;
    byte_swap(data, data + (nel - 1) * width, width);
    das_sift_down_r(data, 0, nel - 1, width, cmp);
}

// Used by qsort/nth_element as the depth-blowup fallback.
template <typename Compare>
static inline void das_heapsort_helper_r(unsigned char *data, size_t nel, size_t width, Compare cmp)
{
    if (nel <= 1) return;
    das_make_heap_r(data, nel, width, cmp);
    for (size_t len = nel; len > 1; len--) {
        byte_swap(data, data + (len - 1) * width, width);
        das_sift_down_r(data, 0, len - 1, width, cmp);
    }
}

// Forward declaration of das_block_partition_r — defined below, referenced
// by das_qsort_r and das_sort<T> which dispatch to it for large sub-ranges.
template <typename Compare>
static inline size_t das_block_partition_r(
    unsigned char *data, size_t lo, size_t hi, size_t width,
    Compare cmp, bool *already_partitioned);

// Hoare partition with pivot at data[lo] (median-of-3 placed it there).
// Returns pivot's final position. Used by das_qsort_r for sub-ranges below
// the block-partition threshold and as the width-too-wide fallback inside
// das_block_partition_r.
template <typename Compare>
static inline size_t das_hoare_partition_r(
    unsigned char *data, size_t lo, size_t hi, size_t width, Compare cmp)
{
    size_t i = lo + 1, j = hi;
    unsigned char *piv = data + lo * width;
    while (true) {
        while (i <= j && cmp(data + i * width, piv)) i++;
        while (i <= j && cmp(piv, data + j * width)) j--;
        if (i >= j) break;
        byte_swap(data + i * width, data + j * width, width);
        i++; j--;
    }
    byte_swap(piv, data + j * width, width);
    return j;
}

// das_qsort_r — introsort with libc++-style block-bitset partition for
// sub-ranges ≥ 128 elements; Hoare partition for smaller (bitset setup
// overhead doesn't amortize below ~128 elements, especially on big structs).
// Median-of-3 pivot placed at data[lo] for both partition styles.
// Insertion-sort tail at 24; heapsort fallback at 2*log2(N) recursion depth.
// Iterative explicit-stack form; stack depth ≤ 128 frames covers nel ~ 2^64.
//
// Block-partition strategy from Edelkamp & Weiß "BlockQuicksort" (ESA 2016),
// landed in libc++ via D93923 (Kutenin, 2021). Closes ~half the gap to
// libc++ std::sort on workhorse types and beats both libstdc++ Musser
// introsort and libc++ pdqsort+block on struct types.
template <typename Compare>
inline void das_qsort_r(void *base, size_t nel, size_t width, Compare cmp)
{
    if (nel <= 1) return;
    unsigned char *data = (unsigned char *)base;
    int max_depth = 0;
    for (size_t x = nel; x > 0; x >>= 1) max_depth += 2;
    struct Frame { size_t lo, hi; int depth; };
    Frame stack[128];
    int sp = 0;
    stack[sp++] = { size_t(0), nel - 1, max_depth };
    while (sp > 0) {
        Frame f = stack[--sp];
        size_t lo = f.lo, hi = f.hi;
        int depth = f.depth;
        while (lo < hi) {
            if (hi - lo < 24) {
                for (size_t i = lo + 1; i <= hi; i++) {
                    for (size_t j = i; j > lo && cmp(data + j * width, data + (j - 1) * width); j--) {
                        byte_swap(data + j * width, data + (j - 1) * width, width);
                    }
                }
                break;
            }
            if (depth-- <= 0) {
                das_heapsort_helper_r(data + lo * width, hi - lo + 1, width, cmp);
                break;
            }
            // Median-of-3 pivot at mid, then move median to lo.
            size_t mid = lo + (hi - lo) / 2;
            unsigned char *plo = data + lo * width;
            unsigned char *pmid = data + mid * width;
            unsigned char *phi = data + hi * width;
            if (cmp(pmid, plo)) byte_swap(plo, pmid, width);
            if (cmp(phi,  plo)) byte_swap(plo, phi,  width);
            if (cmp(phi,  pmid)) byte_swap(pmid, phi, width);
            byte_swap(plo, pmid, width);

            size_t p;
            if (hi - lo >= 128) {
                bool already_partitioned;
                p = das_block_partition_r(data, lo, hi, width, cmp, &already_partitioned);
            } else {
                p = das_hoare_partition_r(data, lo, hi, width, cmp);
            }
            // Iterative tail-call on the larger side.
            size_t lsz = p > lo ? p - lo : 0;
            size_t rsz = hi > p ? hi - p : 0;
            if (lsz < rsz) {
                if (p + 1 < hi) stack[sp++] = { p + 1, hi, depth };
                if (p > lo) hi = p - 1; else break;
            } else {
                if (p > lo) stack[sp++] = { lo, p - 1, depth };
                if (p + 1 < hi) lo = p + 1; else break;
            }
        }
    }
}

// ============================================================================
// Block-partition bake-off candidate (Phase 0.2)
// ============================================================================
//
// Port of libc++'s __bitset_partition + __introsort to byte-pointer form,
// runtime width, runtime cmp. The win vs Hoare partition: branchless inner
// populate loop fills a uint64_t bitset of "which 64 elements need to move",
// then the swap pass is driven by countr_zero — branches collapse from one
// per cmp to one per 64 elements. Reference: Edelkamp & Weiß "BlockQuicksort"
// (ESA 2016), Peters pdqsort, libc++ <__algorithm/sort.h>:495.

static constexpr int DAS_BLOCK_SIZE = 64;

// Block-bitset partition. Pre: data[lo..hi] inclusive, lo < hi, pivot value
// is at data[lo]. Post: pivot moved to its final position p; data[lo..p-1] <
// pivot; data[p+1..hi] >= pivot. Returns p. Sets *already_partitioned to
// true if no element needed to move (no swaps in the initial guarded find).
template <typename Compare>
static inline size_t das_block_partition_r(
    unsigned char *data, size_t lo, size_t hi, size_t width,
    Compare cmp, bool *already_partitioned)
{
    unsigned char pivot[256];
    if (width > sizeof(pivot)) {
        // Width too wide for stack buffer — fall back to Hoare partition.
        // Reuses das_qsort_r body via a separate code path would be cleanest;
        // for now, embed a minimal Hoare partition here.
        size_t i = lo + 1, j = hi;
        unsigned char *piv = data + lo * width;
        while (true) {
            while (i <= j && cmp(data + i * width, piv)) i++;
            while (i <= j && cmp(piv, data + j * width)) j--;
            if (i >= j) break;
            byte_swap(data + i * width, data + j * width, width);
            i++; j--;
        }
        byte_swap(piv, data + j * width, width);
        *already_partitioned = false;
        return j;
    }
    memcpy(pivot, data + lo * width, width);

    // Initial guarded find: skip elements already on the correct side.
    // After median-of-3 the caller placed median at lo; data[hi] is >= median
    // and at least one element exists with cmp(pivot, *) == true at or before hi.
    size_t first = lo + 1;
    while (first <= hi && !cmp(pivot, data + first * width)) first++;

    size_t last = hi;
    if (first < last) {
        while (cmp(pivot, data + last * width)) {
            if (last == lo + 1) break;
            last--;
        }
    }

    *already_partitioned = (first >= last);
    if (!*already_partitioned) {
        byte_swap(data + first * width, data + last * width, width);
        first++;
    }
    // From here on, last is inclusive on the right.

    uint64_t left_bitset = 0;
    uint64_t right_bitset = 0;
    constexpr int B = DAS_BLOCK_SIZE;

    // Main block loop: process full 64-element blocks from each side.
    while (first + 2 * size_t(B) - 1 <= last) {
        if (left_bitset == 0) {
            // Populate: bit j set if data[first+j] >= pivot (needs to move right)
            for (int j = 0; j < B; j++) {
                bool gte = !cmp(data + (first + j) * width, pivot);
                left_bitset |= uint64_t(gte) << j;
            }
        }
        if (right_bitset == 0) {
            // Populate: bit j set if data[last-j] <= pivot (needs to move left)
            for (int j = 0; j < B; j++) {
                bool lte = !cmp(pivot, data + (last - j) * width);
                right_bitset |= uint64_t(lte) << j;
            }
        }
        // Pairwise swap driven by countr_zero on both bitsets.
        while (left_bitset != 0 && right_bitset != 0) {
            int li = int(das_ctz64(left_bitset));
            int ri = int(das_ctz64(right_bitset));
            byte_swap(data + (first + li) * width, data + (last - ri) * width, width);
            left_bitset  &= left_bitset  - 1;
            right_bitset &= right_bitset - 1;
        }
        if (left_bitset  == 0) first += B;
        if (right_bitset == 0) last  -= B;
    }

    // Tail: < 2*B remaining elements; one side may still have a residual bitset.
    size_t remaining = last - first + 1;
    size_t l_size, r_size;
    if (left_bitset == 0 && right_bitset == 0) {
        l_size = remaining / 2;
        r_size = remaining - l_size;
    } else if (left_bitset == 0) {
        l_size = remaining - B;
        r_size = B;
    } else {
        l_size = B;
        r_size = remaining - B;
    }
    if (left_bitset == 0) {
        for (size_t j = 0; j < l_size; j++) {
            bool gte = !cmp(data + (first + j) * width, pivot);
            left_bitset |= uint64_t(gte) << j;
        }
    }
    if (right_bitset == 0) {
        for (size_t j = 0; j < r_size; j++) {
            bool lte = !cmp(pivot, data + (last - j) * width);
            right_bitset |= uint64_t(lte) << j;
        }
    }
    while (left_bitset != 0 && right_bitset != 0) {
        int li = int(das_ctz64(left_bitset));
        int ri = int(das_ctz64(right_bitset));
        byte_swap(data + (first + li) * width, data + (last - ri) * width, width);
        left_bitset  &= left_bitset  - 1;
        right_bitset &= right_bitset - 1;
    }
    if (left_bitset  == 0) first += l_size;
    if (right_bitset == 0) last  -= r_size;

    // Residual: at most one side still has set bits. Swap them into place by
    // walking from the highest set bit downward (libc++ __swap_bitmap_pos_within).
    if (left_bitset != 0) {
        while (left_bitset != 0) {
            int tz = int(63 - das_clz64(left_bitset));
            left_bitset &= (uint64_t(1) << tz) - 1;
            size_t pos = first + size_t(tz);
            if (pos != last) byte_swap(data + pos * width, data + last * width, width);
            last--;
        }
        first = last + 1;
    } else if (right_bitset != 0) {
        while (right_bitset != 0) {
            int tz = int(63 - das_clz64(right_bitset));
            right_bitset &= (uint64_t(1) << tz) - 1;
            size_t pos = last - size_t(tz);
            if (pos != first) byte_swap(data + pos * width, data + first * width, width);
            first++;
        }
    }

    // Place pivot at first-1 (its final position).
    size_t pivot_pos = first - 1;
    if (pivot_pos != lo) {
        memcpy(data + lo * width, data + pivot_pos * width, width);
    }
    memcpy(data + pivot_pos * width, pivot, width);
    return pivot_pos;
}

// das_qsort_block_r — introsort with libc++-style block-bitset partition.
// Same outer skeleton as das_qsort_r (depth limit, insertion-sort tail at 24,
// heapsort fallback at 2*log2(N)). Only the partition algorithm differs.
template <typename Compare>
inline void das_qsort_block_r(void *base, size_t nel, size_t width, Compare cmp)
{
    if (nel <= 1) return;
    unsigned char *data = (unsigned char *)base;
    int max_depth = 0;
    for (size_t x = nel; x > 0; x >>= 1) max_depth += 2;
    struct Frame { size_t lo, hi; int depth; };
    Frame stack[128];
    int sp = 0;
    stack[sp++] = { size_t(0), nel - 1, max_depth };
    while (sp > 0) {
        Frame f = stack[--sp];
        size_t lo = f.lo, hi = f.hi;
        int depth = f.depth;
        while (lo < hi) {
            if (hi - lo < 24) {
                for (size_t i = lo + 1; i <= hi; i++) {
                    for (size_t j = i; j > lo && cmp(data + j * width, data + (j - 1) * width); j--) {
                        byte_swap(data + j * width, data + (j - 1) * width, width);
                    }
                }
                break;
            }
            if (depth-- <= 0) {
                das_heapsort_helper_r(data + lo * width, hi - lo + 1, width, cmp);
                break;
            }
            // Median-of-3 pivot at mid, then move median to lo so partition
            // can use the (pivot-at-first) convention.
            size_t mid = lo + (hi - lo) / 2;
            unsigned char *plo = data + lo * width;
            unsigned char *pmid = data + mid * width;
            unsigned char *phi = data + hi * width;
            if (cmp(pmid, plo)) byte_swap(plo, pmid, width);
            if (cmp(phi,  plo)) byte_swap(plo, phi,  width);
            if (cmp(phi,  pmid)) byte_swap(pmid, phi, width);
            byte_swap(plo, pmid, width);

            bool already_partitioned = false;
            size_t p = das_block_partition_r(data, lo, hi, width, cmp, &already_partitioned);

            // Iterative tail-call on the larger side.
            size_t lsz = p > lo ? p - lo : 0;
            size_t rsz = hi > p ? hi - p : 0;
            if (lsz < rsz) {
                if (p + 1 < hi) stack[sp++] = { p + 1, hi, depth };
                if (p > lo) hi = p - 1; else break;
            } else {
                if (p > lo) stack[sp++] = { lo, p - 1, depth };
                if (p + 1 < hi) lo = p + 1; else break;
            }
        }
    }
}

// das_nth_element_r — introselect with Hoare partition + median-of-3 pivot +
// insertion-sort cutoff at 16 + heapsort fallback at 2*log2(N) recursion depth.
// Beats std::nth_element by ~30% on the bake-off (large N, random data).
template <typename Compare>
inline void das_nth_element_r(void *base, size_t nel, size_t n, size_t width, Compare cmp)
{
    if (nel <= 1 || n >= nel) return;
    unsigned char *data = (unsigned char *)base;
    size_t lo = 0;
    size_t hi = nel - 1;
    int depth_limit = 0;
    for (size_t x = nel; x > 0; x >>= 1) depth_limit += 2;
    while (lo < hi) {
        if (depth_limit-- <= 0) {
            das_heapsort_helper_r(data + lo * width, hi - lo + 1, width, cmp);
            return;
        }
        if (hi - lo < 16) {
            for (size_t i = lo + 1; i <= hi; i++) {
                for (size_t j = i; j > lo && cmp(data + j * width, data + (j - 1) * width); j--) {
                    byte_swap(data + j * width, data + (j - 1) * width, width);
                }
            }
            return;
        }
        size_t mid = lo + (hi - lo) / 2;
        if (cmp(data + mid * width, data + lo * width)) byte_swap(data + lo * width, data + mid * width, width);
        if (cmp(data + hi  * width, data + lo * width)) byte_swap(data + lo * width, data + hi  * width, width);
        if (cmp(data + hi  * width, data + mid * width)) byte_swap(data + mid * width, data + hi * width, width);
        byte_swap(data + mid * width, data + (lo + 1) * width, width);
        unsigned char *pivot = data + (lo + 1) * width;
        size_t i = lo + 2;
        size_t j = hi;
        while (true) {
            while (i <= j && cmp(data + i * width, pivot)) i++;
            while (i <= j && cmp(pivot, data + j * width)) j--;
            if (i >= j) break;
            byte_swap(data + i * width, data + j * width, width);
            i++; j--;
        }
        byte_swap(pivot, data + j * width, width);
        if (j == n) return;
        if (j < n) lo = j + 1;
        else       hi = j > 0 ? j - 1 : 0;
    }
}

// Typed Hoare partition with pivot at data[lo]. Mirror of das_hoare_partition_r
// for the typed das_sort<T> path. Returns pivot's final position.
template <typename T, typename Compare>
static inline size_t das_hoare_partition_t(T *data, size_t lo, size_t hi, Compare cmp)
{
    using std::swap;
    size_t i = lo + 1, j = hi;
    while (true) {
        while (i <= j && cmp(data[i], data[lo])) i++;
        while (i <= j && cmp(data[lo], data[j])) j--;
        if (i >= j) break;
        swap(data[i], data[j]);
        i++; j--;
    }
    swap(data[lo], data[j]);
    return j;
}

// Typed block-bitset partition with pivot at data[lo]. Mirror of
// das_block_partition_r for the typed das_sort<T> path. Uses std::swap on T&
// instead of byte_swap, otherwise structurally identical. Returns pivot's
// final position; sets *already_partitioned if the guarded find skipped to
// the end without swaps.
template <typename T, typename Compare>
static inline size_t das_block_partition_t(
    T *data, size_t lo, size_t hi, Compare cmp, bool *already_partitioned)
{
    using std::swap;
    T pivot = data[lo];   // value copy
    size_t first = lo + 1;
    while (first <= hi && !cmp(pivot, data[first])) first++;
    size_t last = hi;
    if (first < last) {
        while (cmp(pivot, data[last])) {
            if (last == lo + 1) break;
            last--;
        }
    }
    *already_partitioned = (first >= last);
    if (!*already_partitioned) {
        swap(data[first], data[last]);
        first++;
    }

    uint64_t left_bitset = 0;
    uint64_t right_bitset = 0;
    constexpr int B = DAS_BLOCK_SIZE;

    while (first + 2 * size_t(B) - 1 <= last) {
        if (left_bitset == 0) {
            for (int j = 0; j < B; j++) {
                bool gte = !cmp(data[first + j], pivot);
                left_bitset |= uint64_t(gte) << j;
            }
        }
        if (right_bitset == 0) {
            for (int j = 0; j < B; j++) {
                bool lte = !cmp(pivot, data[last - j]);
                right_bitset |= uint64_t(lte) << j;
            }
        }
        while (left_bitset != 0 && right_bitset != 0) {
            int li = int(das_ctz64(left_bitset));
            int ri = int(das_ctz64(right_bitset));
            swap(data[first + li], data[last - ri]);
            left_bitset  &= left_bitset  - 1;
            right_bitset &= right_bitset - 1;
        }
        if (left_bitset  == 0) first += B;
        if (right_bitset == 0) last  -= B;
    }

    size_t remaining = last - first + 1;
    size_t l_size, r_size;
    if (left_bitset == 0 && right_bitset == 0) {
        l_size = remaining / 2;
        r_size = remaining - l_size;
    } else if (left_bitset == 0) {
        l_size = remaining - B;
        r_size = B;
    } else {
        l_size = B;
        r_size = remaining - B;
    }
    if (left_bitset == 0) {
        for (size_t j = 0; j < l_size; j++) {
            bool gte = !cmp(data[first + j], pivot);
            left_bitset |= uint64_t(gte) << j;
        }
    }
    if (right_bitset == 0) {
        for (size_t j = 0; j < r_size; j++) {
            bool lte = !cmp(pivot, data[last - j]);
            right_bitset |= uint64_t(lte) << j;
        }
    }
    while (left_bitset != 0 && right_bitset != 0) {
        int li = int(das_ctz64(left_bitset));
        int ri = int(das_ctz64(right_bitset));
        swap(data[first + li], data[last - ri]);
        left_bitset  &= left_bitset  - 1;
        right_bitset &= right_bitset - 1;
    }
    if (left_bitset  == 0) first += l_size;
    if (right_bitset == 0) last  -= r_size;

    if (left_bitset != 0) {
        while (left_bitset != 0) {
            int tz = int(63 - das_clz64(left_bitset));
            left_bitset &= (uint64_t(1) << tz) - 1;
            size_t pos = first + size_t(tz);
            if (pos != last) swap(data[pos], data[last]);
            last--;
        }
        first = last + 1;
    } else if (right_bitset != 0) {
        while (right_bitset != 0) {
            int tz = int(63 - das_clz64(right_bitset));
            right_bitset &= (uint64_t(1) << tz) - 1;
            size_t pos = last - size_t(tz);
            if (pos != first) swap(data[pos], data[first]);
            first++;
        }
    }

    size_t pivot_pos = first - 1;
    if (pivot_pos != lo) data[lo] = std::move(data[pivot_pos]);
    data[pivot_pos] = std::move(pivot);
    return pivot_pos;
}

// Shared introsort skeleton for typed das_sort<T> and das_sort_block<T>.
// The two differ only in which partition routine fires for ≥ 128 elements;
// below 128 both use typed Hoare partition.
template <typename T, typename Compare, bool ForceBlock>
static inline void das_sort_skeleton_t(T *first, T *last, Compare cmp)
{
    if (last - first <= 1) return;
    size_t nel = size_t(last - first);
    T *data = first;
    int max_depth = 0;
    for (size_t x = nel; x > 0; x >>= 1) max_depth += 2;
    struct Frame { size_t lo, hi; int depth; };
    Frame stack[128];
    int sp = 0;
    stack[sp++] = { size_t(0), nel - 1, max_depth };
    while (sp > 0) {
        Frame f = stack[--sp];
        size_t lo = f.lo, hi = f.hi;
        int depth = f.depth;
        while (lo < hi) {
            if (hi - lo < 24) {
                using std::swap;
                for (size_t i = lo + 1; i <= hi; i++) {
                    for (size_t j = i; j > lo && cmp(data[j], data[j - 1]); j--) {
                        swap(data[j], data[j - 1]);
                    }
                }
                break;
            }
            if (depth-- <= 0) {
                std::make_heap(data + lo, data + hi + 1, cmp);
                std::sort_heap(data + lo, data + hi + 1, cmp);
                break;
            }
            // Median-of-3, place median at lo.
            size_t mid = lo + (hi - lo) / 2;
            using std::swap;
            if (cmp(data[mid], data[lo])) swap(data[lo], data[mid]);
            if (cmp(data[hi],  data[lo])) swap(data[lo], data[hi]);
            if (cmp(data[hi],  data[mid])) swap(data[mid], data[hi]);
            swap(data[lo], data[mid]);

            size_t p;
            if (ForceBlock || hi - lo >= 128) {
                bool ap;
                p = das_block_partition_t(data, lo, hi, cmp, &ap);
            } else {
                p = das_hoare_partition_t(data, lo, hi, cmp);
            }
            size_t lsz = p > lo ? p - lo : 0;
            size_t rsz = hi > p ? hi - p : 0;
            if (lsz < rsz) {
                if (p + 1 < hi) stack[sp++] = { p + 1, hi, depth };
                if (p > lo) hi = p - 1; else break;
            } else {
                if (p > lo) stack[sp++] = { lo, p - 1, depth };
                if (p + 1 < hi) lo = p + 1; else break;
            }
        }
    }
}

// das_sort — typed introsort with block-bitset partition for sub-ranges
// ≥ 128 elements, typed Hoare for smaller. Mirror of das_qsort_r but
// operating on T* with std::swap. Provides an apples-to-apples peer for
// std::sort and a candidate for the workhorse-typed binding path in
// module_builtin_runtime_sort.cpp.
template <typename T, typename Compare>
inline void das_sort(T *first, T *last, Compare cmp)
{
    das_sort_skeleton_t<T, Compare, false>(first, last, cmp);
}

// das_sort_block — typed pure-block-partition introsort. No Hoare fallback
// at any sub-range size. Bench-only ceiling test for the block-partition
// algorithm's typed-access upper bound.
template <typename T, typename Compare>
inline void das_sort_block(T *first, T *last, Compare cmp)
{
    das_sort_skeleton_t<T, Compare, true>(first, last, cmp);
}

// das_partial_sort_r — heap-of-topn scan. Builds a max-heap from the first N
// elements; for each remaining element, if it's smaller than the heap top,
// replaces top + sifts to restore heap. Final drain via repeated pop produces
// sorted output. O(M log N). Matches std::partial_sort.
template <typename Compare>
inline void das_partial_sort_r(void *base, size_t nel, size_t n, size_t width, Compare cmp)
{
    if (nel <= 1 || n == 0) return;
    if (n > nel) n = nel;
    unsigned char *data = (unsigned char *)base;
    das_make_heap_r(data, n, width, cmp);
    for (size_t i = n; i < nel; i++) {
        unsigned char *xi = data + i * width;
        if (cmp(xi, data)) {           // a[i] < heap top (current Nth-smallest)
            byte_swap(data, xi, width);// displace top to position i (outside heap; never revisited)
            das_sift_down_r(data, 0, n, width, cmp);
        }
    }
    // Drain: pop max repeatedly into positions n-1, n-2, ...
    for (size_t len = n; len > 1; len--) {
        byte_swap(data, data + (len - 1) * width, width);
        das_sift_down_r(data, 0, len - 1, width, cmp);
    }
}

// ============================================================================
// Stable sort — adaptive natural-run merge (timsort-lite, no galloping).
//
// Detects ascending / strictly-descending(→reversed-in-place) natural runs,
// extends short runs to MINRUN via stable insertion, then bottom-up pairwise-
// merges runs with a ping-pong buffer. STABLE (equal elements keep input order).
// O(N) on already-sorted / reverse input (exactly N-1 comparisons), O(N log N)
// otherwise. Allocates N-element scratch via malloc (mirrors std::stable_sort's
// get_temporary_buffer); on OOM falls back to the unstable das_qsort_r / das_sort
// (only under genuine allocation failure). Won a bake-off vs std::stable_sort and
// two alternatives (bottom-up merge, index-decoration) — examples/sort/bench_stable_sort.cpp.
// ============================================================================

static constexpr size_t DAS_STABLE_MINRUN = 32;

// Stable insertion sort over the element range [lo, hi). Shifts only elements
// strictly greater than the held value, so equal elements never reorder.
template <typename Compare>
static inline void das_stable_insertion_run_r(unsigned char *data, size_t lo, size_t hi, size_t width, Compare cmp, unsigned char *tmp)
{
    for (size_t i = lo + 1; i < hi; i++) {
        if (!cmp(data + i * width, data + (i - 1) * width)) continue;   // data[i] >= data[i-1]
        sized_memcpy(tmp, data + i * width, width);
        size_t j = i;
        do {
            sized_memcpy(data + j * width, data + (j - 1) * width, width);
            j--;
        } while (j > lo && cmp(tmp, data + (j - 1) * width));
        sized_memcpy(data + j * width, tmp, width);
    }
}

// Threshold for switching one-at-a-time merge → galloping (batched) mode. Below 7
// consecutive wins from one side we stay one-at-a-time (no extra comparisons, no
// memcpy-call overhead on random data); a longer streak triggers a bulk run copy
// (the case that dominates on partially-ordered input). Classic timsort constant.
static constexpr int DAS_STABLE_MIN_GALLOP = 7;

// Merge sorted runs [l,m) and [m,r) from src into dst. Stable: takes the left
// element on a tie (right < left false). Galloping batches whole-run copies once
// one side wins a streak — turns the per-element copy loop into bulk memcpy on
// structured data, where the element-wise copy would otherwise dominate.
template <typename Compare>
static inline void das_stable_merge_runs_r(const unsigned char *src, unsigned char *dst,
                                           size_t l, size_t m, size_t r, size_t width, Compare cmp)
{
    size_t i = l, j = m, k = l;
    while (i < m && j < r) {
        int cl = 0, cr = 0;
        for (;;) {                                                          // one-at-a-time
            if (cmp(src + j * width, src + i * width)) {                     // right < left
                sized_memcpy(dst + k * width, src + j * width, width); k++; j++;
                cr++; cl = 0;
                if (j >= r) goto done;
                if (cr >= DAS_STABLE_MIN_GALLOP) break;
            } else {                                                        // left <= right (stable)
                sized_memcpy(dst + k * width, src + i * width, width); k++; i++;
                cl++; cr = 0;
                if (i >= m) goto done;
                if (cl >= DAS_STABLE_MIN_GALLOP) break;
            }
        }
        for (;;) {                                                          // galloping
            size_t is = i;                                                  // left run: src[i..] <= src[j]
            while (i < m && !cmp(src + j * width, src + i * width)) i++;
            size_t lenL = i - is;
            if (lenL) { memcpy(dst + k * width, src + is * width, lenL * width); k += lenL; if (i >= m) goto done; }
            sized_memcpy(dst + k * width, src + j * width, width); k++; j++; if (j >= r) goto done;
            size_t js = j;                                                  // right run: src[j..] < src[i]
            while (j < r && cmp(src + j * width, src + i * width)) j++;
            size_t lenR = j - js;
            if (lenR) { memcpy(dst + k * width, src + js * width, lenR * width); k += lenR; if (j >= r) goto done; }
            sized_memcpy(dst + k * width, src + i * width, width); k++; i++; if (i >= m) goto done;
            if (lenL < size_t(DAS_STABLE_MIN_GALLOP) && lenR < size_t(DAS_STABLE_MIN_GALLOP)) break;
        }
    }
done:
    if (i < m) memcpy(dst + k * width, src + i * width, (m - i) * width);
    if (j < r) memcpy(dst + k * width, src + j * width, (r - j) * width);
}

// Byte-pointer stable sort — the daslang interp any-cblock binding path.
template <typename Compare>
inline void das_stable_sort_r(void *base, size_t nel, size_t width, Compare cmp)
{
    if (nel <= 1) return;
    unsigned char *data = (unsigned char *)base;
    unsigned char *buf = (unsigned char *)malloc(nel * width);
    if (!buf) { das_qsort_r(base, nel, width, cmp); return; }   // OOM → unstable fallback
    unsigned char tmpStack[256];
    unsigned char *tmp = width <= sizeof(tmpStack) ? tmpStack : (unsigned char *)malloc(width);
    size_t maxRuns = nel / DAS_STABLE_MINRUN + 2;
    size_t *bndA = (size_t *)malloc((maxRuns + 1) * sizeof(size_t));
    size_t *bndB = (size_t *)malloc((maxRuns + 1) * sizeof(size_t));
    if (!tmp || !bndA || !bndB) {
        if (tmp && tmp != tmpStack) free(tmp);
        free(bndA); free(bndB); free(buf);
        das_qsort_r(base, nel, width, cmp);
        return;
    }

    // 1. Detect natural runs (ascending, or strictly-descending then reversed),
    //    each extended to MINRUN via stable insertion. Collect run boundaries.
    size_t *cur = bndA, *nxt = bndB;
    size_t nb = 0;
    cur[nb++] = 0;
    size_t i = 0;
    while (i < nel) {
        size_t runStart = i, j = i + 1;
        if (j < nel) {
            if (cmp(data + j * width, data + i * width)) {                  // strictly descending
                j++;
                while (j < nel && cmp(data + j * width, data + (j - 1) * width)) j++;
                for (size_t a = runStart, b = j - 1; a < b; a++, b--)        // reverse → ascending (stable: strict)
                    byte_swap(data + a * width, data + b * width, width);
            } else {                                                        // non-decreasing
                j++;
                while (j < nel && !cmp(data + j * width, data + (j - 1) * width)) j++;
            }
        }
        if (j - runStart < DAS_STABLE_MINRUN) {
            size_t hi = (runStart + DAS_STABLE_MINRUN < nel) ? runStart + DAS_STABLE_MINRUN : nel;
            das_stable_insertion_run_r(data, runStart, hi, width, cmp, tmp);
            j = hi;
        }
        cur[nb++] = j;
        i = j;
    }

    // 2. Bottom-up pairwise merge over the run list, ping-pong data <-> buf.
    unsigned char *src = data, *dst = buf;
    while (nb - 1 > 1) {                                                    // > 1 run
        size_t R = nb - 1, nn = 0, t = 0;
        nxt[nn++] = 0;
        for (; t + 1 < R; t += 2) {
            das_stable_merge_runs_r(src, dst, cur[t], cur[t + 1], cur[t + 2], width, cmp);
            nxt[nn++] = cur[t + 2];
        }
        if (t < R) {                                                       // lone last run
            memcpy(dst + cur[t] * width, src + cur[t] * width, (cur[t + 1] - cur[t]) * width);
            nxt[nn++] = cur[t + 1];
        }
        unsigned char *ts = src; src = dst; dst = ts;
        size_t *tb = cur; cur = nxt; nxt = tb;
        nb = nn;
    }
    if (src != data) memcpy(data, src, nel * width);

    if (tmp != tmpStack) free(tmp);
    free(bndA); free(bndB); free(buf);
}

// ---- Typed twins (the AOT path) ----
// Element moves go through memcpy(sizeof(T)) — a compile-time size the compiler
// inlines, unlike the runtime-width byte path — while comparisons use the typed
// comparator. Requires trivially-relocatable T (the daslang array-element model,
// same assumption as das_sort<T>).

template <typename T, typename Compare>
static inline void das_stable_insertion_run_t(T *data, size_t lo, size_t hi, Compare cmp)
{
    for (size_t i = lo + 1; i < hi; i++) {
        if (!cmp(data[i], data[i - 1])) continue;
        T held = data[i];                                                  // value copy for the comparisons
        size_t j = i;
        do {
            memcpy(&data[j], &data[j - 1], sizeof(T));
            j--;
        } while (j > lo && cmp(held, data[j - 1]));
        memcpy(&data[j], &held, sizeof(T));
    }
}

template <typename T, typename Compare>
static inline void das_stable_merge_runs_t(const T *src, T *dst, size_t l, size_t m, size_t r, Compare cmp)
{
    size_t i = l, j = m, k = l;
    while (i < m && j < r) {
        int cl = 0, cr = 0;
        for (;;) {                                                          // one-at-a-time
            if (cmp(src[j], src[i])) {                                       // right < left
                memcpy(&dst[k++], &src[j++], sizeof(T));
                cr++; cl = 0;
                if (j >= r) goto done;
                if (cr >= DAS_STABLE_MIN_GALLOP) break;
            } else {                                                        // left <= right (stable)
                memcpy(&dst[k++], &src[i++], sizeof(T));
                cl++; cr = 0;
                if (i >= m) goto done;
                if (cl >= DAS_STABLE_MIN_GALLOP) break;
            }
        }
        for (;;) {                                                          // galloping
            size_t is = i;                                                  // left run: src[i..] <= src[j]
            while (i < m && !cmp(src[j], src[i])) i++;
            size_t lenL = i - is;
            if (lenL) { memcpy(&dst[k], &src[is], lenL * sizeof(T)); k += lenL; if (i >= m) goto done; }
            memcpy(&dst[k++], &src[j++], sizeof(T)); if (j >= r) goto done;
            size_t js = j;                                                  // right run: src[j..] < src[i]
            while (j < r && cmp(src[j], src[i])) j++;
            size_t lenR = j - js;
            if (lenR) { memcpy(&dst[k], &src[js], lenR * sizeof(T)); k += lenR; if (j >= r) goto done; }
            memcpy(&dst[k++], &src[i++], sizeof(T)); if (i >= m) goto done;
            if (lenL < size_t(DAS_STABLE_MIN_GALLOP) && lenR < size_t(DAS_STABLE_MIN_GALLOP)) break;
        }
    }
done:
    if (i < m) { memcpy(&dst[k], &src[i], (m - i) * sizeof(T)); }
    if (j < r) { memcpy(&dst[k], &src[j], (r - j) * sizeof(T)); }
}

// Typed stable sort — the AOT _T binding path. Same algorithm as das_stable_sort_r
// with compile-time element size + typed comparator.
template <typename T, typename Compare>
inline void das_stable_sort(T *first, T *last, Compare cmp)
{
    if (last - first <= 1) return;
    size_t nel = size_t(last - first);
    T *data = first;
    T *buf = (T *)malloc(nel * sizeof(T));
    if (!buf) { das_sort(first, last, cmp); return; }                      // OOM → unstable fallback
    size_t maxRuns = nel / DAS_STABLE_MINRUN + 2;
    size_t *bndA = (size_t *)malloc((maxRuns + 1) * sizeof(size_t));
    size_t *bndB = (size_t *)malloc((maxRuns + 1) * sizeof(size_t));
    if (!bndA || !bndB) { free(bndA); free(bndB); free(buf); das_sort(first, last, cmp); return; }

    size_t *cur = bndA, *nxt = bndB;
    size_t nb = 0;
    cur[nb++] = 0;
    size_t i = 0;
    while (i < nel) {
        size_t runStart = i, j = i + 1;
        if (j < nel) {
            if (cmp(data[j], data[i])) {                                   // strictly descending
                j++;
                while (j < nel && cmp(data[j], data[j - 1])) j++;
                using std::swap;
                for (size_t a = runStart, b = j - 1; a < b; a++, b--) swap(data[a], data[b]);
            } else {                                                       // non-decreasing
                j++;
                while (j < nel && !cmp(data[j], data[j - 1])) j++;
            }
        }
        if (j - runStart < DAS_STABLE_MINRUN) {
            size_t hi = (runStart + DAS_STABLE_MINRUN < nel) ? runStart + DAS_STABLE_MINRUN : nel;
            das_stable_insertion_run_t(data, runStart, hi, cmp);
            j = hi;
        }
        cur[nb++] = j;
        i = j;
    }

    T *src = data, *dst = buf;
    while (nb - 1 > 1) {
        size_t R = nb - 1, nn = 0, t = 0;
        nxt[nn++] = 0;
        for (; t + 1 < R; t += 2) {
            das_stable_merge_runs_t(src, dst, cur[t], cur[t + 1], cur[t + 2], cmp);
            nxt[nn++] = cur[t + 2];
        }
        if (t < R) {
            memcpy(&dst[cur[t]], &src[cur[t]], (cur[t + 1] - cur[t]) * sizeof(T));
            nxt[nn++] = cur[t + 1];
        }
        T *ts = src; src = dst; dst = ts;
        size_t *tb = cur; cur = nxt; nxt = tb;
        nb = nn;
    }
    if (src != data) memcpy(data, src, nel * sizeof(T));

    free(bndA); free(bndB); free(buf);
}

}

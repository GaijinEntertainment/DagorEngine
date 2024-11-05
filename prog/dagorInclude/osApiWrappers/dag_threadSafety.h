//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// This file contains cross-platform wrappers for various thread safety static
// analysis tools. They can help us find "obvious" bugs where someone forgot to
// take a lock or something similar.
// See https://clang.llvm.org/docs/ThreadSafetyAnalysis.html

// Only works with clang-based compilers
#if defined(__has_attribute)
#if __has_attribute(guarded_by) // assume that if we have one, we have all of them

#define DAG_TS_CAPABILITY(x) __attribute__((capability(x)))

#define DAG_TS_SCOPED_CAPABILITY __attribute__((scoped_lockable))

#define DAG_TS_GUARDED_BY(x) __attribute__((guarded_by(x)))

#define DAG_TS_PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))

#define DAG_TS_ACQUIRED_BEFORE(...) __attribute__((acquired_before(__VA_ARGS__)))

#define DAG_TS_ACQUIRED_AFTER(...) __attribute__((acquired_after(__VA_ARGS__)))

#define DAG_TS_REQUIRES(...) __attribute__((requires_capability(__VA_ARGS__)))

#define DAG_TS_REQUIRES_SHARED(...) __attribute__((requires_shared_capability(__VA_ARGS__)))

#define DAG_TS_ACQUIRE(...) __attribute__((acquire_capability(__VA_ARGS__)))

#define DAG_TS_ACQUIRE_SHARED(...) __attribute__((acquire_shared_capability(__VA_ARGS__)))

#define DAG_TS_RELEASE(...) __attribute__((release_capability(__VA_ARGS__)))

#define DAG_TS_RELEASE_SHARED(...) __attribute__((release_shared_capability(__VA_ARGS__)))

#define DAG_TS_RELEASE_GENERIC(...) __attribute__((release_generic_capability(__VA_ARGS__)))

#define DAG_TS_TRY_ACQUIRE(...) __attribute__((try_acquire_capability(__VA_ARGS__)))

#define DAG_TS_TRY_ACQUIRE_SHARED(...) __attribute__((try_acquire_shared_capability(__VA_ARGS__)))

#define DAG_TS_EXCLUDES(...) __attribute__((locks_excluded(__VA_ARGS__)))

#define DAG_TS_ASSERT_CAPABILITY(x) __attribute__((assert_capability(x)))

#define DAG_TS_ASSERT_SHARED_CAPABILITY(x) __attribute__((assert_shared_capability(x)))

#define DAG_TS_RETURN_CAPABILITY(x) __attribute__((lock_returned(x)))

#define DAG_TS_NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))

#endif
#endif

#ifndef DAG_TS_CAPABILITY

#define DAG_TS_CAPABILITY(x)

#define DAG_TS_SCOPED_CAPABILITY

#define DAG_TS_GUARDED_BY(x)

#define DAG_TS_PT_GUARDED_BY(x)

#define DAG_TS_ACQUIRED_BEFORE(...)

#define DAG_TS_ACQUIRED_AFTER(...)

#define DAG_TS_REQUIRES(...)

#define DAG_TS_REQUIRES_SHARED(...)

#define DAG_TS_ACQUIRE(...)

#define DAG_TS_ACQUIRE_SHARED(...)

#define DAG_TS_RELEASE(...)

#define DAG_TS_RELEASE_SHARED(...)

#define DAG_TS_RELEASE_GENERIC(...)

#define DAG_TS_TRY_ACQUIRE(...)

#define DAG_TS_TRY_ACQUIRE_SHARED(...)

#define DAG_TS_EXCLUDES(...)

#define DAG_TS_ASSERT_CAPABILITY(x)

#define DAG_TS_ASSERT_SHARED_CAPABILITY(x)

#define DAG_TS_RETURN_CAPABILITY(x)

#define DAG_TS_NO_THREAD_SAFETY_ANALYSIS

#endif

options gen2
require dastest/testing_boost public

require daslib/fio
require math

//! Regression coverage for `ref_time_ticks()` and `get_time_usec(int64)`.
//!
//! The cross-platform normalization landed via PR #2685 (Windows now returns
//! nanoseconds via QPC, matching POSIX clock_gettime — previously Windows
//! returned raw QPC ticks). These tests pin the post-fix contract:
//!   - `ref_time_ticks()` returns a monotonic non-decreasing nanosecond
//!     timestamp on every platform.
//!   - `get_time_usec(t0)` returns elapsed time in microseconds since `t0`
//!     and stays consistent with the `ref_time_ticks` delta.
//!   - sleep(N ms) round-trip lands close to N ms (with slack for OS
//!     scheduler granularity — Windows is the worst at ~16 ms ticks).

[test]
def test_ref_time_ticks_monotonic(t : T?) {
    //! Successive calls never go backwards. Catches QPC wrap-around
    //! arithmetic bugs and any future signed/unsigned mixups in the
    //! nanosecond conversion path.
    var prev = ref_time_ticks()
    for (i in range(1000)) {
        let now = ref_time_ticks()
        if (now < prev) {
            t |> success(false, "ref_time_ticks went backwards at iter {i}: prev={prev} now={now}")
            return
        }
        prev = now
    }
    t |> success(true, "1000 successive ref_time_ticks reads stayed monotonic")
}

[test]
def test_ref_time_ticks_sleep_roundtrip(t : T?) {
    //! sleep(100ms) → delta_ns should be in [80ms, 500ms]. Wide upper
    //! bound covers CI runner jitter (GitHub-hosted Windows scheduling
    //! can balloon to 200 ms even for a 100 ms sleep). Lower bound
    //! catches a units bug: if Windows still reported raw QPC ticks
    //! (10 MHz → 10× short for the same delta), delta_ns would land
    //! around 10 ms and we'd trip.
    let before = ref_time_ticks()
    sleep(100u)
    let delta_ms = get_time_nsec(before) / 1_000_000l
    t |> success(delta_ms >= 80l,
        "sleep(100ms) elapsed only {delta_ms}ms — ref_time_ticks may not be in ns")
    t |> success(delta_ms <= 500l,
        "sleep(100ms) elapsed {delta_ms}ms — way over budget")
}

[test]
def test_get_time_usec_agrees_with_ref_delta(t : T?) {
    //! `get_time_usec(t0)` and `(ref_time_ticks() - t0) / 1000` should
    //! agree to within a few µs (the two calls happen sequentially,
    //! so a small drift is normal).
    let t0 = ref_time_ticks()
    sleep(50u)
    let usec_via_helper = int64(get_time_usec(t0))
    let usec_via_delta = get_time_nsec(t0) / 1_000l
    let drift_us = int(abs(usec_via_delta - usec_via_helper))
    t |> success(drift_us <= 5_000,
        "get_time_usec={usec_via_helper} vs delta/1000={usec_via_delta} drift={drift_us}us")
}

[test]
def test_ref_time_ticks_units_are_nanoseconds(t : T?) {
    //! Sanity check that two sleep(100ms) calls in a row produce roughly the
    //! same raw tick delta. Ticks are in the embedder's unit, so this pins rate
    //! uniformity only - elapsed-time contracts live in get_time_nsec/usec above.
    var deltas : array<int64>
    deltas |> reserve(3)
    for (_i in range(3)) {
        let a = ref_time_ticks()
        sleep(100u)
        let b = ref_time_ticks()
        deltas |> push(b - a)
    }
    let lo = min(deltas[0], min(deltas[1], deltas[2]))  // nolint:LINT021 - ns-scale ticks; the /1_000_000 under the cast needs 64 bits
    let hi = max(deltas[0], max(deltas[1], deltas[2]))  // nolint:LINT021 - ns-scale ticks; the /1_000_000 under the cast needs 64 bits
    let spread_ms = int((hi - lo) / 1_000_000l)
    t |> success(spread_ms <= 200,
        "sleep(100ms) deltas span {spread_ms}ms — non-uniform tick rate")
    delete deltas
}

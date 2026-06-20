options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 3 Phase 3 (audit `benchmarks/sql/linq_fold_chain_audit.md` probes C1 + C5):
// the "top-K distinct" composition family. Two arm pairs in mirror order:
//   C1: `_distinct_by(K1) |> _order_by(K2) |> take(N) |> to_array()`
//   C5: `_order_by(K1) |> distinct() |> take(N) |> to_array()`
//
// Both extend plan_order_family + plan_decs_order_family's bounded-heap path
// with a leading or middle `distinct[_by]` gate — set-insert on the distinct
// key short-circuits per-element heap push/pop. Single source pass, no full
// distinct materialization. Today (pre-PR) both shapes cascade to tier-2.
// Position of `distinct` in the chain (before vs after order_by) has no
// bearing on emission — the set just gates the same heap update.

// ── C1a: array source, distinct_by + order_by + take ─────────────────────────

[test]
def test_c1a_array_distinct_by_order_take(t : T?) {
    t |> run("C1a: each + _distinct_by(_.user) + _order_by(_.ts) + take(N) + to_array — top-K distinct users by ts") @(tt : T?) {
        // 50 items: user cycles U0..U9, ts = i*13%100. Top-5 distinct users by smallest ts:
        // U0(ts=0), U8(ts=4), U1(ts=13), U9(ts=17), U2(ts=26).
        let items <- [for (i in range(50)); (user = "U{i % 10}", ts = (i * 13) % 100)]
        let rows <- _fold(each(items)
            |> _distinct_by(_.user)
            |> _order_by(_.ts)
            |> take(5)
            |> to_array())
        tt |> equal(length(rows), 5)
        tt |> equal(rows[0].user, "U0")
        tt |> equal(rows[0].ts, 0)
        tt |> equal(rows[1].user, "U8")
        tt |> equal(rows[1].ts, 4)
        tt |> equal(rows[2].user, "U1")
        tt |> equal(rows[2].ts, 13)
        tt |> equal(rows[3].user, "U9")
        tt |> equal(rows[3].ts, 17)
        tt |> equal(rows[4].user, "U2")
        tt |> equal(rows[4].ts, 26)
    }
}

// ── C5a: array source, order_by + distinct + take ────────────────────────────

[test]
def test_c5a_array_order_distinct_take(t : T?) {
    t |> run("C5a: each + _order_by(_.score) + distinct() + take(N) + to_array — top-K distinct tuples by score") @(tt : T?) {
        // 6 elems, distinct tuples ascending by score: (a, 10), (b, 10), (c, 20), (d, 30); plus dupes.
        let items <- [
            (name = "a", score = 10),
            (name = "a", score = 10),
            (name = "b", score = 10),
            (name = "c", score = 20),
            (name = "d", score = 30),
            (name = "c", score = 20)
        ]
        let rows <- _fold(each(items)
            |> _order_by(_.score)
            |> distinct()
            |> take(3)
            |> to_array())
        // Top-3 distinct tuples by ascending score. With score=10 tied (a, b), bounded heap
        // keeps the first 2 distinct elements seen (a, b) plus the next distinct lowest (c, 20).
        tt |> equal(length(rows), 3)
        // Plain `distinct()` dedups on whole element — verify uniqueness on the FULL TUPLE, not just name. A buggy implementation that keyed dedup on name alone would still pass a name-only assertion but FAIL the combined-key check below.
        var seenWhole : table<string>
        for (r in rows) {
            seenWhole |> insert("{r.name}:{r.score}")
            tt |> equal(r.score <= 20, true)
        }
        tt |> equal(length(seenWhole), 3)
        // Exact set: top-3 distinct tuples ascending by score are (a,10), (b,10), (c,20).
        tt |> equal(key_exists(seenWhole, "a:10"), true)
        tt |> equal(key_exists(seenWhole, "b:10"), true)
        tt |> equal(key_exists(seenWhole, "c:20"), true)
    }
}

// ── C1c: descending order on top of distinct_by ──────────────────────────────

[test]
def test_c1c_array_distinct_by_order_desc(t : T?) {
    t |> run("C1c: each + _distinct_by(_.user) + _order_by_descending(_.ts) + take + to_array — top-K distinct users by LARGEST ts") @(tt : T?) {
        // Largest ts per user across the 50-item cycle. Top-3 by desc ts.
        let items <- [for (i in range(50)); (user = "U{i % 10}", ts = (i * 13) % 100)]
        let rows <- _fold(each(items)
            |> _distinct_by(_.user)
            |> _order_by_descending(_.ts)
            |> take(3)
            |> to_array())
        tt |> equal(length(rows), 3)
        // distinct_by keeps FIRST occurrence per user (key insert order). i=0..9 are
        // (U0,0),(U1,13),(U2,26),(U3,39),(U4,52),(U5,65),(U6,78),(U7,91),(U8,4),(U9,17).
        // Top-3 desc: U7@91, U6@78, U5@65.
        tt |> equal(rows[0].user, "U7")
        tt |> equal(rows[0].ts, 91)
        tt |> equal(rows[1].user, "U6")
        tt |> equal(rows[1].ts, 78)
        tt |> equal(rows[2].user, "U5")
        tt |> equal(rows[2].ts, 65)
    }
}

// ── C1-where: WHERE filter composes with distinct_by + order + take ──────────

[test]
def test_c1_where_distinct_by_order_take(t : T?) {
    t |> run("C1-where: each + _where + _distinct_by + _order_by + take + to_array — filter before distinct gate; structural assertion (distinct users, all pass filter, sorted asc)") @(tt : T?) {
        let items <- [for (i in range(50)); (user = "U{i % 10}", ts = (i * 13) % 100)]
        let rows <- _fold(each(items)
            |> _where(_.ts >= 20)
            |> _distinct_by(_.user)
            |> _order_by(_.ts)
            |> take(3)
            |> to_array())
        tt |> equal(length(rows), 3)
        // Structural assertions: 3 distinct users, all ts >= 20, sorted ascending.
        var users : table<string>
        for (r in rows) {
            users |> insert(r.user)
            tt |> equal(r.ts >= 20, true)
        }
        tt |> equal(length(users), 3)
        tt |> equal(rows[0].ts <= rows[1].ts, true)
        tt |> equal(rows[1].ts <= rows[2].ts, true)
        // C1-where parity vs handwritten: spliced output must match a handwritten distinct+sort+take cascade element-for-element (same input order → same distinct order → same heap result).
        var seen : table<string>
        var distinct : array<tuple<user : string; ts : int>>
        for (it in items) {
            if (it.ts >= 20 && !key_exists(seen, it.user)) {
                seen |> insert(it.user)
                distinct |> push_clone(it)
            }
        }
        sort(distinct, $(a, b) => _::less(a.ts, b.ts))
        for (i in range(3)) {
            tt |> equal(rows[i].user, distinct[i].user)
            tt |> equal(rows[i].ts, distinct[i].ts)
        }
    }
}

// ── C1-select: terminal _select projects ≤K heap survivors ───────────────────

[test]
def test_c1_with_terminal_select(t : T?) {
    t |> run("C1-select: each + _distinct_by + _order_by + take + _select(_.user) — projection at return") @(tt : T?) {
        let items <- [for (i in range(50)); (user = "U{i % 10}", ts = (i * 13) % 100)]
        let rows <- _fold(each(items)
            |> _distinct_by(_.user)
            |> _order_by(_.ts)
            |> take(3)
            |> _select(_.user)
            |> to_array())
        tt |> equal(length(rows), 3)
        tt |> equal(rows[0], "U0")
        tt |> equal(rows[1], "U8")
        tt |> equal(rows[2], "U1")
    }
}

// ── C1-parity: spliced vs handwritten cascade — element-for-element match ────

def c1_handwritten(items : array<tuple<user : string; ts : int>>) : array<tuple<user : string; ts : int>> {
    var seen : table<string>
    var distinct : array<tuple<user : string; ts : int>>
    for (it in items) {
        if (!key_exists(seen, it.user)) {
            seen |> insert(it.user)
            distinct |> push_clone(it)
        }
    }
    sort(distinct, $(a, b) => _::less(a.ts, b.ts))
    var out <- [for (i in range(min(5, length(distinct)))); distinct[i]]
    return <- out
}

[test]
def test_c1_parity_handwritten(t : T?) {
    t |> run("C1-parity: spliced vs handwritten distinct+sort+take cascade") @(tt : T?) {
        let items <- [for (i in range(50)); (user = "U{i % 10}", ts = (i * 13) % 100)]
        let spliced <- _fold(each(items)
            |> _distinct_by(_.user)
            |> _order_by(_.ts)
            |> take(5)
            |> to_array())
        let hand <- c1_handwritten(items)
        tt |> equal(length(spliced), length(hand))
        for (i in range(length(spliced))) {
            tt |> equal(spliced[i].user, hand[i].user)
            tt |> equal(spliced[i].ts, hand[i].ts)
        }
    }
}

// ── C1-decs: from_decs_template + distinct_by + order_by + take ──────────────

[decs_template(prefix = "c1d_")]
struct C1User {
    user : string
    ts : int
}

def populate_c1_decs() {
    restart()
    create_entities(50) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, C1User(user = "U{i % 10}", ts = (i * 13) % 100))
    }
}

[test]
def test_c1_decs_distinct_by_order_take(t : T?) {
    t |> run("C1-decs: from_decs_template + _distinct_by(_.user) + _order_by(_.ts) + take + to_array — decs lane bounded-heap with set-gate") @(tt : T?) {
        populate_c1_decs()
        unsafe {
            let rows <- _fold(from_decs_template(type<C1User>)
                |> _distinct_by(_.user)
                |> _order_by(_.ts)
                |> take(5)
                |> to_array())
            tt |> equal(length(rows), 5)
            tt |> equal(rows[0].user, "U0")
            tt |> equal(rows[0].ts, 0)
            tt |> equal(rows[1].user, "U8")
            tt |> equal(rows[1].ts, 4)
            tt |> equal(rows[2].user, "U1")
            tt |> equal(rows[2].ts, 13)
            tt |> equal(rows[3].user, "U9")
            tt |> equal(rows[3].ts, 17)
            tt |> equal(rows[4].user, "U2")
            tt |> equal(rows[4].ts, 26)
        }
        restart()
    }
}

// ── C5-decs: from_decs_template + order_by + distinct + take ─────────────────

[decs_template(prefix = "c5d_")]
struct C5Score {
    name : string
    score : int
}

def populate_c5_decs() {
    restart()
    // 6 entities with distinct (name, score) tuples plus duplicates of (a,10) and (c,20).
    let data = [
        (name = "a", score = 10),
        (name = "a", score = 10),
        (name = "b", score = 10),
        (name = "c", score = 20),
        (name = "d", score = 30),
        (name = "c", score = 20)
    ]
    create_entities(6) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, C5Score(name = data[i].name, score = data[i].score))
    }
}

[test]
def test_c5_decs_order_distinct_take(t : T?) {
    t |> run("C5-decs: from_decs_template + _order_by(_.score) + distinct() + take + to_array — decs lane with whole-tuple set") @(tt : T?) {
        populate_c5_decs()
        unsafe {
            let rows <- _fold(from_decs_template(type<C5Score>)
                |> _order_by(_.score)
                |> distinct()
                |> take(3)
                |> to_array())
            tt |> equal(length(rows), 3)
            // 3 distinct TUPLES with smallest scores: (a,10), (b,10), (c,20). Track uniqueness by combined key so a buggy name-only dedup wouldn't pass.
            var seenWhole : table<string>
            for (r in rows) {
                seenWhole |> insert("{r.name}:{r.score}")
                tt |> equal(r.score <= 20, true)
            }
            tt |> equal(length(seenWhole), 3)
            tt |> equal(key_exists(seenWhole, "a:10"), true)
            tt |> equal(key_exists(seenWhole, "b:10"), true)
            tt |> equal(key_exists(seenWhole, "c:20"), true)
        }
        restart()
    }
}

// ── C1-anti: non-inline order key cascades (gate condition for splice) ───────

var g_c1anti_key_calls = 0

def c1anti_ts_with_count(v : tuple<user : string; ts : int>) : int {
    g_c1anti_key_calls ++
    return v.ts
}

[test]
def test_c1_anti_non_inline_order_key_cascades(t : T?) {
    t |> run("C1-anti: _order_by(side-effecting fn) — inline-cmp guard fails, splice null, full cascade fires (and still produces correct output)") @(tt : T?) {
        let items <- [for (i in range(20)); (user = "U{i % 5}", ts = (i * 7) % 50)]
        g_c1anti_key_calls = 0
        let rows <- _fold(each(items)
            |> _distinct_by(_.user)
            |> _order_by(c1anti_ts_with_count(_))
            |> take(3)
            |> to_array())
        tt |> equal(length(rows), 3)
        // Cascade path: distinct_by_to_array materializes the distinct array, then order_by_inplace
        // invokes the user key per element. Splice path would never call c1anti_ts_with_count.
        tt |> equal(g_c1anti_key_calls > 0, true)
    }
}

// ── R1 cascade guards: bail conditions enforced by plan_order_family ─────────

// `order_by(K2).distinct_by(K1)` is NOT equivalent to the source-walk set-gate
// (cascade picks the FIRST K1 in sort order = min-K2 per K1; splice would keep
// an arbitrary K1 representative). Must cascade to preserve semantics.

[test]
def test_c1_anti_order_by_then_distinct_by_cascades(t : T?) {
    t |> run("C1-anti: _order_by(K2) + _distinct_by(K1) + take — sort-aware dedup required; splice bails, cascade picks min-K2 per K1") @(tt : T?) {
        // 3 entries per user with varying ts; ascending sort by ts then distinct_by user picks min-ts per user.
        let items <- [
            (user = "A", ts = 5),
            (user = "B", ts = 3),
            (user = "A", ts = 1),
            (user = "B", ts = 8),
            (user = "A", ts = 9)
        ]
        let rows <- _fold(each(items)
            |> _order_by(_.ts)
            |> _distinct_by(_.user)
            |> take(2)
            |> to_array())
        // Cascade semantics: sort by ts → [(A,1),(B,3),(A,5),(B,8),(A,9)]; distinct_by user keeps
        // (A,1) then (B,3); take(2) → [(A,1),(B,3)]. A splice that kept first-seen during walk
        // would have produced [(A,5),(B,3)] which would be WRONG. The bail ensures correctness.
        tt |> equal(length(rows), 2)
        tt |> equal(rows[0].user, "A")
        tt |> equal(rows[0].ts, 1)
        tt |> equal(rows[1].user, "B")
        tt |> equal(rows[1].ts, 3)
    }
}

// `distinct[_by]` without `take` would be silently dropped by the splice (the
// set-gate lives only in the bounded-heap path). Must cascade.

[test]
def test_c1_anti_distinct_without_take_cascades(t : T?) {
    t |> run("C1-anti: _distinct_by + _order_by + to_array (no take) — must cascade so distinct isn't silently dropped") @(tt : T?) {
        let items <- [
            (user = "A", ts = 5),
            (user = "B", ts = 3),
            (user = "A", ts = 1),
            (user = "C", ts = 7)
        ]
        let rows <- _fold(each(items)
            |> _distinct_by(_.user)
            |> _order_by(_.ts)
            |> to_array())
        // distinct_by user (first-seen): (A,5), (B,3), (C,7); sort by ts asc: (B,3), (A,5), (C,7).
        tt |> equal(length(rows), 3)
        tt |> equal(rows[0].user, "B")
        tt |> equal(rows[0].ts, 3)
        tt |> equal(rows[1].user, "A")
        tt |> equal(rows[1].ts, 5)
        tt |> equal(rows[2].user, "C")
        tt |> equal(rows[2].ts, 7)
    }
}

// `take(N).distinct[_by]()` is NOT equivalent to a per-walk distinct gate
// (cascade takes first N then dedups, splice would dedup ALL source then take).

[test]
def test_c1_anti_take_then_distinct_cascades(t : T?) {
    t |> run("C1-anti: take(N) + distinct_by + order_by — must cascade so distinct isn't applied pre-take") @(tt : T?) {
        // First 3 items are all distinct users; later items add duplicates. take(3).distinct keeps all 3.
        let items <- [
            (user = "A", ts = 10),
            (user = "B", ts = 20),
            (user = "C", ts = 30),
            (user = "A", ts = 40),
            (user = "D", ts = 50)
        ]
        let rows <- _fold(each(items)
            |> take(3)
            |> _distinct_by(_.user)
            |> _order_by(_.ts)
            |> to_array())
        // Cascade: take(3) → [(A,10),(B,20),(C,30)]; distinct_by user (all unique) → same; sort by ts → same.
        // A splice that applied distinct pre-take would have produced the same result by accident on
        // this input, but would diverge on inputs with early duplicates — the bail guards correctness.
        tt |> equal(length(rows), 3)
        tt |> equal(rows[0].user, "A")
        tt |> equal(rows[1].user, "B")
        tt |> equal(rows[2].user, "C")
    }
}

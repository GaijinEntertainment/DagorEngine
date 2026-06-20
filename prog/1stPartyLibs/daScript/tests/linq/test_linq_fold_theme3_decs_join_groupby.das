options gen2
options _no_linq_perf_warn = true  // contains C3-anti-* cascade-still-works regression guards

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 3 Phase 1 (audit `benchmarks/sql/linq_fold_chain_audit.md` probe C3): the
// "killer demo" composition. plan_decs_group_by recognizes a trailing `join`
// upstream of `group_by_lazy` and switches to the isDecsJoin adapter mode so
// plan_decs_join's hashB-collect + srcA-probe + per-pair result-lam bind feeds
// plan_group_by_core's bucket-update directly — one pass, no intermediate alloc.
// Today (pre-PR) this composition cascades to tier-2 (3 allocs).

[decs_template(prefix = "dt3c_")]
struct C3Car {
    id : int
    dealer_id : int
    name : string
}

[decs_template(prefix = "dt3d_")]
struct C3Dealer {
    id : int
    region : string
    name : string
}

def populate_c3_fixture() {
    restart()
    // 6 cars across 3 dealers (id 1,2,3); dealers 1 & 2 are in REGION_NORTH, 3 in REGION_SOUTH.
    create_entities(6) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, C3Car(id = i + 1, dealer_id = i % 3 + 1, name = "Car{i}"))
    }
    create_entities(3) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        let region = (i == 2 ? "south" : "north")
        apply_decs_template(cmp, C3Dealer(id = i + 1, region = region, name = "Dealer{i}"))
    }
}

// Module-level counters for the side-effect ordering probe (C3e). Reset per test run.
var g_c3e_result_calls = 0

def c3e_make_result(carName, dealerRegion : string) : tuple<Region : string; Car : string> {
    g_c3e_result_calls ++
    return (Region = dealerRegion, Car = carName)
}

// ── C3a: count terminator + count reducer ──────────────────────────────────────

[test]
def test_c3a_count_count(t : T?) {
    t |> run("C3a: _join + _group_by(_.Region) + _select(N=count()) + count() (bucket count)") @(tt : T?) {
        populate_c3_fixture()
        unsafe {
            let nBuckets = _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarName = l.name))
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> count())
            // 2 distinct regions across the 6 join pairs → 2 buckets.
            tt |> equal(nBuckets, 2)
        }
        restart()
    }
}

// ── C3b: count terminator + sum reducer ────────────────────────────────────────

[test]
def test_c3b_sum_count(t : T?) {
    t |> run("C3b: _join + _group_by(_.Region) + _select(S=sum(r.CarId)) + count()") @(tt : T?) {
        populate_c3_fixture()
        unsafe {
            let nBuckets = _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarId = l.id))
                |> _group_by(_.Region)
                |> _select((R = _._0, S = _._1 |> select(@(r : tuple<Region : string; CarId : int>) => r.CarId) |> sum))
                |> count())
            tt |> equal(nBuckets, 2)
        }
        restart()
    }
}

// ── C3c: to_array terminator + named-tuple reducer (full BI shape) ─────────────

[test]
def test_c3c_to_array_named_tuple(t : T?) {
    t |> run("C3c: _join + _group_by(_.Region) + _select((R, N=count)) + to_array (per-region bucket count)") @(tt : T?) {
        populate_c3_fixture()
        unsafe {
            let rows <- _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarName = l.name))
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> to_array())
            // Cars by dealer_id: dealer1 gets cars 0,3; dealer2 gets 1,4; dealer3 gets 2,5.
            // Regions: dealer1,2 → north (4 cars), dealer3 → south (2 cars).
            tt |> equal(length(rows), 2)
            var perRegion <- {for (item in rows); item.R => item.N}
            tt |> equal(perRegion["north"], 4)
            tt |> equal(perRegion["south"], 2)
        }
        restart()
    }
}

// ── C3d: parity vs handwritten cascade ─────────────────────────────────────────

def c3d_handwritten() : array<tuple<R : string; N : int>> {
    // Manual cascade equivalent to the C3c chain. The C3d test below compares per-bucket counts
    // (length + dict-comprehension lookup), so bucket order does not need to match — only the
    // per-key totals must agree between the spliced and handwritten paths.
    var hashB : table<int; array<tuple<region : string; name : string>>>
    query() $ [unused_argument(eid)] (eid : EntityId; dt3d_id : int; dt3d_region : string; dt3d_name : string) {
        hashB[dt3d_id] |> push_clone((region = dt3d_region, name = dt3d_name))
    }
    var perRegion : table<string; int>
    query() $ [unused_argument(eid, dt3c_id, dt3c_name)] (eid : EntityId; dt3c_id : int; dt3c_dealer_id : int; dt3c_name : string) {
        if (key_exists(hashB, dt3c_dealer_id)) {
            for (d in hashB[dt3c_dealer_id]) {
                if (key_exists(perRegion, d.region)) {
                    perRegion[d.region] ++
                } else {
                    perRegion |> insert(d.region, 1)
                }
            }
        }
    }
    var out <- [for (k, v in keys(perRegion), values(perRegion)); (R = k, N = v)]
    return <- out
}

[test]
def test_c3d_parity_with_handwritten(t : T?) {
    t |> run("C3d: spliced vs handwritten cascade — per-region bucket counts must agree") @(tt : T?) {
        populate_c3_fixture()
        unsafe {
            let rows <- _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarName = l.name))
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> to_array())
            let hand <- c3d_handwritten()
            tt |> equal(length(rows), length(hand))
            var spliced  <- {for (item in rows); item.R => item.N}
            var hand_dict <- {for (item in hand); item.R => item.N}
            tt |> equal(spliced["north"], hand_dict["north"])
            tt |> equal(spliced["south"], hand_dict["south"])
        }
        restart()
    }
}

// ── C3e: side-effect ordering — result lambda fires once per join pair ─────────

[test]
def test_c3e_result_lambda_called_once_per_pair(t : T?) {
    t |> run("C3e: result lambda fires exactly len(srcA)·matchesPerKey times (no double-eval)") @(tt : T?) {
        populate_c3_fixture()
        g_c3e_result_calls = 0
        unsafe {
            let nBuckets = _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => c3e_make_result(l.name, r.region))
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> count())
            tt |> equal(nBuckets, 2)
            // 6 cars × 1 dealer match per car = 6 result-lam invocations.
            tt |> equal(g_c3e_result_calls, 6)
        }
        restart()
    }
}

// ── C3-anti-having: cascade-still-works guard ──────────────────────────────────

[test]
def test_c3_anti_having_cascades(t : T?) {
    t |> run("C3-anti-having: _join + _group_by + _select + _where (HAVING) — v1 cascades, must produce correct result") @(tt : T?) {
        populate_c3_fixture()
        unsafe {
            let rows <- _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarName = l.name))
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> _where(_.N >= 3)
                |> to_array())
            // Only "north" (4 cars) passes N>=3; "south" (2 cars) filtered out.
            tt |> equal(length(rows), 1)
            tt |> equal(rows[0].R, "north")
            tt |> equal(rows[0].N, 4)
        }
        restart()
    }
}

// ── C3-anti-segment: cascade-still-works guard ─────────────────────────────────

[test]
def test_c3_anti_segment_cascades(t : T?) {
    t |> run("C3-anti-segment: _join + _where + _group_by — segment between join and group_by_lazy cascades, must produce correct result") @(tt : T?) {
        populate_c3_fixture()
        unsafe {
            let nBuckets = _fold(from_decs_template(type<C3Car>)
                |> _join(from_decs_template(type<C3Dealer>),
                         $(l, r) => l.dealer_id == r.id,
                         $(l, r) => (Region = r.region, CarName = l.name))
                |> _where(_.Region == "north")
                |> _group_by(_.Region)
                |> _select((R = _._0, N = _._1 |> count()))
                |> count())
            // After WHERE filter: only north (4 cars) survives → 1 bucket.
            tt |> equal(nBuckets, 1)
        }
        restart()
    }
}

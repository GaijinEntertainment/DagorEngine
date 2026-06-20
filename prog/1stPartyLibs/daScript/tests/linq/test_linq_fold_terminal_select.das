options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

struct Sound {
    id   : int
    x    : float
    rank : int
}

[decs_template(prefix = "ds_")]
struct DecsSound {
    id : int
    x : float
}

[decs_template(prefix = "dc_")]
struct DecsCar {
    id : int
    dealer_id : int
    name : string
}

[decs_template(prefix = "dd_")]
struct DecsDealer {
    id : int
    name : string
}

def make_sounds() : array<Sound> {
    return <- [
        Sound(id = 1, x = 3.0, rank = 1),
        Sound(id = 2, x = 1.0, rank = 2),
        Sound(id = 3, x = 4.0, rank = 3),
        Sound(id = 4, x = 1.0, rank = 4),
        Sound(id = 5, x = 5.0, rank = 5)
    ]
}

[test]
def test_order_take_select_array(t : T?) {
    t |> run("plan_order_family: take + terminal _select") @(tt : T?) {
        let sounds <- make_sounds()
        // Closest 3 by |x|; return their ids.
        unsafe {
            let ids <- _fold(each(sounds)._order_by(abs(_.x)).take(3)._select(_.id).to_array())
            // Sounds with smallest |x|: id 2 (1.0), id 4 (1.0), id 1 (3.0).
            tt |> equal(length(ids), 3)
            tt |> equal(true, ids[0] == 2 || ids[0] == 4)
            tt |> equal(true, ids[2] == 1)
        }
    }
}

[test]
def test_where_order_take_select_array(t : T?) {
    t |> run("plan_order_family: where + take + terminal _select") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            let ids <- _fold(each(sounds)._where(_.rank >= 2)._order_by(_.x).take(2)._select(_.id).to_array())
            // After filter rank>=2: ids 2,3,4,5 (x = 1,4,1,5). Top 2 by x: ids 2 (1.0), 4 (1.0).
            tt |> equal(length(ids), 2)
            for (id in ids) {
                tt |> equal(true, id == 2 || id == 4)
            }
        }
    }
}

[test]
def test_where_order_bare_select_array(t : T?) {
    t |> run("plan_order_family: where + bare order + terminal _select") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            let ranks <- _fold(each(sounds)._where(_.id != 3)._order_by(_.x)._select(_.rank).to_array())
            // After filter id!=3: ids 1,2,4,5 (x = 3,1,1,5). Sorted by x: (2 or 4, 2 or 4, 1, 5).
            tt |> equal(length(ranks), 4)
            tt |> equal(ranks[3], 5)
        }
    }
}

[test]
def test_order_take_select_decs(t : T?) {
    t |> run("plan_decs_order_family: take + terminal _select") @(tt : T?) {
        restart()
        create_entities(5) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsSound(id = i + 1, x = float((i % 2) * 5 + 1)))
        }
        unsafe {
            let ids <- _fold(from_decs_template(type<DecsSound>)._order_by(_.x).take(2)._select(_.id).to_array())
            tt |> equal(length(ids), 2)
        }
        restart()
    }
}

[test]
def test_reverse_take_select_array(t : T?) {
    t |> run("plan_reverse: where + reverse + take + terminal _select") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            let ids <- _fold(each(sounds)._where(_.rank > 0).reverse().take(2)._select(_.id).to_array())
            // After filter (all 5), reverse: 5,4,3,2,1. take 2: 5,4.
            tt |> equal(length(ids), 2)
            tt |> equal(ids[0], 5)
            tt |> equal(ids[1], 4)
        }
    }
}

// PR after #2915: bare `reverse + take + _select` (no where, no pre-select) on an array source
// is the symmetric m3f mirror of #2915's decs skip-into-tail termsel extension. R6 backward-index
// walk now accepts an optional trailing `_select(F)` slot: the loop pushes K raw source elements
// into a srcElem-typed scratch buffer (`for k in 0..K: buf |> push_clone(src[len-1-k])`), then
// `build_terminal_select_tail` performs a post-loop projection pass into a separate projElem-typed
// output buffer. Two-buffer/two-pass mirrors the decs sibling + R1-R4 catch-all discipline (all
// source reads complete before any projection-side-effect runs). Cost vs the catch-all: K raw
// push_clones into the scratch buf instead of N; the post-loop projection K push_clones is shared
// with the catch-all. Previously this shape fell through to R1-R4 which walked ALL N entries.

[test]
def test_reverse_take_select_array_bare(t : T?) {
    t |> run("plan_reverse R6: bare reverse + take + terminal _select — backward index walk projects K only") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            // ids 1..5; reverse: 5,4,3,2,1; take 2: 5,4; select _.id: 5,4
            let ids <- _fold(each(sounds).reverse().take(2)._select(_.id).to_array())
            tt |> equal(length(ids), 2)
            tt |> equal(ids[0], 5)
            tt |> equal(ids[1], 4)
        }
    }
}

[test]
def test_reverse_take_select_array_take_exceeds(t : T?) {
    t |> run("plan_reverse R6: bare reverse + take(N>len) + _select clamps to length") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            // ids 1..5; reverse: 5,4,3,2,1; take 100 clamps to 5; select _.id*2
            let xs <- _fold(each(sounds).reverse().take(100)._select(_.id * 2).to_array())
            tt |> equal(length(xs), 5)
            tt |> equal(xs[0], 10)
            tt |> equal(xs[1], 8)
            tt |> equal(xs[4], 2)
        }
    }
}

[test]
def test_reverse_take_select_array_take_zero(t : T?) {
    t |> run("plan_reverse R6: bare reverse + take(0) + _select returns empty") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            let ids <- _fold(each(sounds).reverse().take(0)._select(_.id).to_array())
            tt |> equal(length(ids), 0)
        }
    }
}

[test]
def test_reverse_take_select_array_empty(t : T?) {
    t |> run("plan_reverse R6: bare reverse + take + _select on empty source") @(tt : T?) {
        var empty_sounds : array<Sound>
        unsafe {
            let ids <- _fold(each(empty_sounds).reverse().take(3)._select(_.id).to_array())
            tt |> equal(length(ids), 0)
        }
    }
}

[test]
def test_reverse_select_first_array(t : T?) {
    t |> run("plan_reverse: where + reverse + _select + first") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            let id = _fold(each(sounds)._where(_.rank > 0).reverse()._select(_.id).first())
            // Reverse of ids 1..5 is 5,4,3,2,1; first = 5.
            tt |> equal(id, 5)
        }
    }
}

// PR A extension: chains with BOTH a pre-reverse `_select(f)` AND a post-reverse `_select(g)` were
// previously rejected by the imperative plan_reverse (it had a `!seenSelect` guard before letting any
// terminal _select fire). The new pattern-table forms (R1-R4 + Rb) accept the combined shape because
// emit composes `pushExpr` from the pre-projection and the terminal-select pass from the post-projection;
// there's no semantic conflict (it's plain function composition over the reversed survivors).

[test]
def test_reverse_pre_and_post_select_array(t : T?) {
    t |> run("plan_reverse R1-R4: where + select(f) + reverse + take + select(g) — both selects compose") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            // pre-select: id*10  → [10, 20, 30, 40, 50]
            // reverse:           → [50, 40, 30, 20, 10]
            // take 3:            → [50, 40, 30]
            // post-select: +1    → [51, 41, 31]
            let out <- _fold(each(sounds)._where(_.rank > 0)._select(_.id * 10).reverse().take(3)._select(_ + 1).to_array())
            tt |> equal(length(out), 3)
            tt |> equal(out[0], 51)
            tt |> equal(out[1], 41)
            tt |> equal(out[2], 31)
        }
    }
}

[test]
def test_reverse_pre_and_post_select_first(t : T?) {
    t |> run("plan_reverse Rb: where + select(f) + reverse + select(g) + first — both selects compose") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            // pre-select: id*10  → [10, 20, 30, 40, 50]
            // reverse:           → [50, 40, 30, 20, 10]
            // post-select: +1    → [51, 41, 31, 21, 11]
            // first:             → 51
            let v = _fold(each(sounds)._where(_.rank > 0)._select(_.id * 10).reverse()._select(_ + 1).first())
            tt |> equal(v, 51)
        }
    }
}

// Regression for the bug Copilot caught in R6: emit must NOT re-project the user's `first_or_default` default
// through the post-reverse `_select(g)`. The default is already typed at the post-select element type, so
// applying termsel(d) double-applies and miscomputes when the chain hits the empty branch.

[test]
def test_reverse_pre_and_post_select_first_or_default_nonempty(t : T?) {
    t |> run("plan_reverse Rb: where + select(f) + reverse + select(g) + first_or_default — nonempty hits found branch") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            // sequence non-empty: termsel(lastName) = (50)+1 = 51
            let v = _fold(each(sounds)._where(_.rank > 0)._select(_.id * 10).reverse()._select(_ + 1).first_or_default(-99))
            tt |> equal(v, 51)
        }
    }
}

[test]
def test_reverse_pre_and_post_select_first_or_default_empty(t : T?) {
    t |> run("plan_reverse Rb: where + select(f) + reverse + select(g) + first_or_default — empty seq returns RAW default, not termsel(default)") @(tt : T?) {
        let sounds <- make_sounds()
        unsafe {
            // where filters everything out → empty sequence → default branch fires.
            // If emit re-projects, result would be (-99)+1 = -98 (wrong). Must be -99.
            let v = _fold(each(sounds)._where(_.rank > 9999)._select(_.id * 10).reverse()._select(_ + 1).first_or_default(-99))
            tt |> equal(v, -99)
        }
    }
}

[test]
def test_reverse_take_select_decs(t : T?) {
    t |> run("plan_decs_reverse: reverse + take + terminal _select") @(tt : T?) {
        restart()
        create_entities(4) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsSound(id = i + 1, x = float(i)))
        }
        unsafe {
            // ids: [1,2,3,4]; reverse: [4,3,2,1]; take 2: [4,3]; select _.id: [4,3]
            let ids <- _fold(from_decs_template(type<DecsSound>).reverse().take(2)._select(_.id).to_array())
            tt |> equal(length(ids), 2)
            tt |> equal(ids[0], 4)
            tt |> equal(ids[1], 3)
        }
        restart()
    }
}

[test]
def test_reverse_take_select_decs_take_exceeds(t : T?) {
    t |> run("plan_decs_reverse: reverse + take(N>total) + terminal _select clamps to total") @(tt : T?) {
        restart()
        create_entities(3) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsSound(id = i + 10, x = float(i)))
        }
        unsafe {
            // ids: [10,11,12]; reverse: [12,11,10]; take 100 clamps to 3: [12,11,10]; select _.id
            let ids <- _fold(from_decs_template(type<DecsSound>).reverse().take(100)._select(_.id).to_array())
            tt |> equal(length(ids), 3)
            tt |> equal(ids[0], 12)
            tt |> equal(ids[1], 11)
            tt |> equal(ids[2], 10)
        }
        restart()
    }
}

[test]
def test_reverse_take_select_decs_empty(t : T?) {
    t |> run("plan_decs_reverse: reverse + take + terminal _select on empty source") @(tt : T?) {
        restart()
        unsafe {
            let ids <- _fold(from_decs_template(type<DecsSound>).reverse().take(5)._select(_.id).to_array())
            tt |> equal(length(ids), 0)
        }
        restart()
    }
}

[test]
def test_join_select_to_array(t : T?) {
    t |> run("plan_decs_join: join + terminal _select") @(tt : T?) {
        restart()
        create_entities(3) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsCar(id = i + 1, dealer_id = i % 2 + 1, name = "Car{i}"))
        }
        create_entities(2) $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, DecsDealer(id = i + 1, name = "Dealer{i}"))
        }
        unsafe {
            let names <- _fold(from_decs_template(type<DecsCar>) |> _join(from_decs_template(type<DecsDealer>),
                                                                          $(l, r) => l.dealer_id == r.id,
                                                                          $(l, r) => (CarName = l.name, DealerName = r.name))
                                                                 |> _select(_.CarName)
                                                                 |> to_array())
            // 3 cars × 1 matching dealer each = 3 results.
            tt |> equal(length(names), 3)
        }
        restart()
    }
}

[test]
def test_zip_3arg_sum(t : T?) {
    t |> run("plan_zip: 3-arg zip + sum") @(tt : T?) {
        let a <- [1, 2, 3, 4]
        let b <- [10, 20, 30, 40]
        unsafe {
            let total = _fold(each(a) |> zip(each(b), $(x, y : int) => x * y) |> sum())
            // 1*10 + 2*20 + 3*30 + 4*40 = 10 + 40 + 90 + 160 = 300.
            tt |> equal(total, 300)
        }
    }
}

[test]
def test_zip_3arg_to_array(t : T?) {
    t |> run("plan_zip: 3-arg zip + to_array") @(tt : T?) {
        let a <- [1, 2, 3]
        let b <- [10, 20, 30]
        unsafe {
            let r <- _fold(each(a) |> zip(each(b), $(x, y : int) => x + y) |> to_array())
            tt |> equal(length(r), 3)
            tt |> equal(r[0], 11)
            tt |> equal(r[1], 22)
            tt |> equal(r[2], 33)
        }
    }
}

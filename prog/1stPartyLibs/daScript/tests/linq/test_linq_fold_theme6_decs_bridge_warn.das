options gen2

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 6 — `_fold` emits a `*warning*` to compiler log when a `from_decs_template`
// bridge survives all `plan_decs_*` arms and the chain materializes a temp array
// before the tier-2 cascade. This file exercises the FIRES path: the warning is
// visible at compile time, and the cascade still returns the correct result.
// Companion file `_silenced` covers `options _no_linq_perf_warn = true`.

[decs_template(prefix = "t6_")]
struct Theme6Row {
    val : int
}

def populate(n : int) {
    restart()
    create_entities(n) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, Theme6Row(val = i))
    }
}

[test]
def test_theme6_warning_fires_cascade_correct(t : T?) {
    t |> run("Theme 6: fall-off chain emits warning + cascade returns correct result") @(tt : T?) {
        populate(20)
        // `_select(F)._skip_while(P).count()` over decs — predicate is post-projection so
        // `plan_decs_unroll`'s range-suffix path bails (audit chain 6a). Tier-2 cascade
        // emits select_to_array → skip_while → count.
        unsafe {
            let c = _fold(from_decs_template(type<Theme6Row>)._select(_.val)._skip_while(_ < 5).count())
            // vals are 0..19, skip while < 5 drops 5 elements, count remaining 15.
            tt |> equal(c, 15)
        }
        restart()
    }
}

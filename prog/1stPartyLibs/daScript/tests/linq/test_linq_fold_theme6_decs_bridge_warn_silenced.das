options gen2
options _no_linq_perf_warn = true

require math
require strings
require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require daslib/decs
require daslib/decs_boost
require dastest/testing_boost public

// Theme 6 — companion to `test_linq_fold_theme6_decs_bridge_warn.das`. Same
// fall-off chain, but with `options _no_linq_perf_warn = true` the diagnostic
// is suppressed. Verifies the escape hatch works without breaking compilation
// or altering the cascade's runtime behavior.

[decs_template(prefix = "t6s_")]
struct Theme6SilencedRow {
    val : int
}

def populate(n : int) {
    restart()
    create_entities(n) $(eid : EntityId; i : int; var cmp : ComponentMap) {
        apply_decs_template(cmp, Theme6SilencedRow(val = i))
    }
}

[test]
def test_theme6_silenced_no_warn_cascade_correct(t : T?) {
    t |> run("Theme 6: `_no_linq_perf_warn` suppresses warning, cascade still correct") @(tt : T?) {
        populate(20)
        unsafe {
            let c = _fold(from_decs_template(type<Theme6SilencedRow>)._select(_.val)._skip_while(_ < 5).count())
            tt |> equal(c, 15)
        }
        restart()
    }
}

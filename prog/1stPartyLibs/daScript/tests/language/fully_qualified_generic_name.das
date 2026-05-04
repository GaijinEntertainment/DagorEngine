options gen2
require UnitTest
require math
require dastest/testing_boost public

def on_armor_hit_fx_effect_es(visual_armor_hit_dot_effectName : string) {
    var tm : float3x4
    identity(tm)
    return UnitTest::start_effect(visual_armor_hit_dot_effectName, tm)
}

[test]
def test_fully_qualified_generic_name(t : T?) {
    t |> success(!on_armor_hit_fx_effect_es("test"))
    var tm : float3x4
    identity(tm)
    UnitTest::start_effect("test-test", tm)
}


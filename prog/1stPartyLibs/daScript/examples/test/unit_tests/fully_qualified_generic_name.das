require UnitTest
require math

def on_armor_hit_fx_effect_es(visual_armor_hit_dot_effectName : string)
    var tm: float3x4
    identity(tm)
    return UnitTest::start_effect(visual_armor_hit_dot_effectName, tm)

[export]
def test
    verify(!on_armor_hit_fx_effect_es("test"))
    var tm: float3x4
    identity(tm)
    UnitTest::start_effect("test-test", tm)
    return true


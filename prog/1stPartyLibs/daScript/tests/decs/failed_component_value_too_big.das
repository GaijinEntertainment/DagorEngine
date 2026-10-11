options gen2
expect 31400

require daslib/decs_boost
require dastest/testing_boost public

struct BigComponent {
    a : float4
    b : float4
    c : float4
    d : float4
    e : float4
}

[test]
def test_component_value_rejects_oversized(t : T?) {
    var cmp : ComponentMap
    set(cmp.big, BigComponent())
}

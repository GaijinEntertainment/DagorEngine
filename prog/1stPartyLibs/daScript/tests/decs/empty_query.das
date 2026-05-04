options gen2
require daslib/decs_boost
require dastest/testing_boost public

[test]
def test_globals(t : T?) {
    feint("hello")
}

[decs(stage = update_ecs)]
def update_ecs(pos : float3) {
    pass // return
}

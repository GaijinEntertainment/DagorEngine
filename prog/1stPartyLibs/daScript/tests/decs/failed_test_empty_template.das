options gen2
expect 30111

options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

[decs_template]
struct Particle {}
// 30111: macro [decs_template] failed to apply to the structure Particle
// expecting at least one field in the desc_template Particle

[test]
def test_create_from_templates(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            apply_decs_template(cmp, Particle())
        }
    }
    commit()
}

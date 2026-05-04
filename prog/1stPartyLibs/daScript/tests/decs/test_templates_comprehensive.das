options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

// === decs_template with default prefix (StructName_) ===

[decs_template]
struct Particle {
    pos : float3
    vel : float3
    life : int
}

[test]
def test_template_default_prefix(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Particle(pos = float3(1, 2, 3), vel = float3(4, 5, 6), life = 100))
    }
    commit()
    // components stored with "Particle_" prefix
    var found = false
    query(eid) $(Particle_pos : float3; Particle_vel : float3; Particle_life : int) {
        t |> equal(Particle_pos, float3(1, 2, 3))
        t |> equal(Particle_vel, float3(4, 5, 6))
        t |> equal(Particle_life, 100)
        found = true
    }
    t |> success(found)
}

// === decs_template with custom prefix ===

[decs_template(prefix = "p_")]
struct Bullet {
    speed : float
    damage : int
}

[test]
def test_template_custom_prefix(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Bullet(speed = 100.0, damage = 50))
    }
    commit()
    var found = false
    query(eid) $(p_speed : float; p_damage : int) {
        t |> equal(p_speed, 100.0)
        t |> equal(p_damage, 50)
        found = true
    }
    t |> success(found)
}

// === decs_template with empty prefix ===

[decs_template(prefix)]
struct Vehicle {
    speed : float
    fuel : int
}

[test]
def test_template_empty_prefix(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Vehicle(speed = 60.0, fuel = 100))
    }
    commit()
    // no prefix — component names are "speed" and "fuel" directly
    var found = false
    query(eid) $(speed : float; fuel : int) {
        t |> equal(speed, 60.0)
        t |> equal(fuel, 100)
        found = true
    }
    t |> success(found)
}

// === Query with template struct ===

[test]
def test_query_with_template_struct(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            apply_decs_template(cmp, Particle(pos = float3(i), vel = float3(1), life = 10 * i))
        }
    }
    commit()
    var count = 0
    query() $(p : Particle) {
        count++
        t |> equal(p.vel, float3(1))
    }
    t |> equal(count, 3)
}

// === Mutable query with template struct ===

[test]
def test_query_mutate_template(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Particle(pos = float3(0), vel = float3(1, 0, 0), life = 10))
    }
    commit()
    // mutate via template
    query() $(var p : Particle) {
        p.pos += p.vel
        p.life -= 1
    }
    // verify mutation
    var found = false
    query(eid) $(Particle_pos : float3; Particle_life : int) {
        t |> equal(Particle_pos, float3(1, 0, 0))
        t |> equal(Particle_life, 9)
        found = true
    }
    t |> success(found)
}

// === remove_decs_template ===

[test]
def test_remove_template(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Vehicle(speed = 60.0, fuel = 100))
        cmp.name := "car"
    }
    commit()
    // remove the template components
    update_entity(eid) @(eid, cmp) {
        cmp |> remove("speed")
        cmp |> remove("fuel")
    }
    commit()
    // "speed" and "fuel" gone, but "name" remains
    var found = false
    query(eid) $(name : string) {
        t |> equal(name, "car")
        found = true
    }
    t |> success(found)

    var foundSpeed = false
    query(eid) $(speed : float) {
        foundSpeed = true
    }
    t |> success(!foundSpeed)
}

// === Multiple templates on same entity ===

[test]
def test_multiple_templates(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Particle(pos = float3(1), vel = float3(2), life = 5))
        apply_decs_template(cmp, Bullet(speed = 300.0, damage = 75))
    }
    commit()
    // both templates' components present
    var found = false
    query(eid) $(Particle_pos : float3; p_speed : float; p_damage : int) {
        t |> equal(Particle_pos, float3(1))
        t |> equal(p_speed, 300.0)
        t |> equal(p_damage, 75)
        found = true
    }
    t |> success(found)
}

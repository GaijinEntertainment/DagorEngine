options gen2
options persistent_heap = true
options gc
options relaxed_pointer_const

require daslib/decs_boost
require dastest/testing_boost public
require daslib/apply

[test]
def test_create_object(t : T?) {
    restart()
    create_entity() @(eid, cmp) {   // create one entity
        cmp |> set("pos", float3(1, 2, 3))
    }
    commit()
    var count = 0
    query() $(pos : float3) {
        count ++
        t |> equal(pos, float3(1, 2, 3))
    }
    t |> success(count == 1)                           // make sure its the right one
}

[test]
def test_clone_syntax(t : T?) {
    restart()
    create_entity() @(eid, cmp) {   // create one entity
        cmp.pos := float3(1, 2, 3)                  // same thing as in the text above, just different notation
    }
    commit()
    var count = 0
    query() $(pos : float3) {
        count ++
        t |> equal(pos, float3(1, 2, 3))
    }
    t |> success(count == 1)
}

[test]
def test_type_mismatch(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.pos := float3(1, 2, 3)
        var ok = true
        try {
            cmp |> set("pos", 13)           // mismatch set
        } recover {
            ok = false
        }
        // t |> equal( ok, false)
        ok = true
        try {
            cmp |> get("pos", default<int>)    // mismatch get
        } recover {
            ok = false
        }
        // t |> equal( ok, false)
        let f123 = cmp |> get("pos", float3(13))
        t |> equal("1, 2, 3", "{f123}")
    }
    commit()
}

[decs_template]
struct Particle {
    pos, vel : float3
}

[test]
def test_create_from_templates(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            apply_decs_template(cmp, Particle(pos = float3(i), vel = float3(i + 1)))
        }
    }
    commit()
    var count = 0
    query() $(p : Particle) {
        t |> equal(p.pos + float3(1), p.vel)
        count ++
    }
    t |> success(count == 3)
}

[decs_template(prefix)]
struct Prefixless {
    value : int
}

[test]
def test_create_from_prefixless_templates(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            apply_decs_template(cmp, Prefixless(value = i))
        }
    }
    commit()
    var count = 0
    query() $(p : Prefixless; value : int) {// access the same thing as template, and as value
        t |> equal(p.value, value)
        count ++
    }
    t |> success(count == 3)
}

struct Foo {
    pos : float3
    name : string
    eid : EntityId
    foobar : int
}

def test_create_via_apply(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        var foo = Foo(pos = float3(1.), name = "from template", eid = eid)
        apply(foo) $(n, v) {
            cmp |> set(n, v)
        }
    }
    commit()
    query() $(name : string) {
        t |> equal(name, "from template")
    }
}


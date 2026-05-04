options gen2
require daslib/decs_boost
require daslib/static_let
require dastest/testing_boost public

[decs_template]
struct Coord {
    xyz : float[3]
}

def operator ==(a, b : float[3]) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2]
}

[test]
def dim_test {
    static_let() {
        var migr : EntityId
    }
    for (t in range(3)) {
        create_entity() @(eid, cmp) {
            let ft = float(t)
            cmp |> set("eid", eid)
            apply_decs_template(cmp, Coord(xyz <- fixed_array<float>(1. + ft, 2. + ft, 3. + ft)))
            migr = eid
        }
    }
    commit()

    update_entity(migr) @(eid, cmp) {
        verify(cmp |> has("Coord_xyz"))
        cmp |> set("Coord_xyz", fixed_array<float>(-1., -2., -3.))
    }
    commit()

    var i = 0
    query() $(c : Coord) {
        if (i == 0) {
            verify(c.xyz == fixed_array<float>(1., 2., 3.))
        } elif (i == 1) {
            verify(c.xyz == fixed_array<float>(2., 3., 4.))
        } elif (i == 2) {
            verify(c.xyz == fixed_array<float>(-1., -2., -3.))
        }
        i ++
    }
    assert(i == 3)

    query() $(var c : Coord) {
        c.xyz = fixed_array<float>(13., 14., 15.)
    }

    query(migr) $(var c : Coord) {
        c.xyz = fixed_array<float>(-1., -2., -3.)
    }

    i = 0
    query() $(c : Coord) {
        if (i == 0 || i == 1) {
            verify(c.xyz == fixed_array<float>(13., 14., 15.))
        } elif (i == 2) {
            verify(c.xyz == fixed_array<float>(-1., -2., -3.))
        }
        i ++
    }
    assert(i == 3)
}

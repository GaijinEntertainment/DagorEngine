options gen2
require UnitTest
require dastest/testing_boost public

var g_tab : table<EntityId; int>
var g_vec : table<int2; int>
var g_pts : table<Point3; int>

let N = 40

// One table reached by both tiers in one program: [no_aot] keeps the write interpreted inside
// the AOT binary, while the read is AOT. The index is written directly here rather than through
// a daslib generic, so the hashing happens in this function and not in an AOT'd callee. A value
// type is hashed and compared by its workhorse in every tier, so the read finds what the write
// put there; hashing the C++ object in AOT instead puts the key in another bucket. A builtin
// vector key is the other direction: the interpreter hashes the int2 itself, so an AOT read that
// hashed its vec4f workhorse would miss. Point3 is a handled type narrower than its workhorse,
// and N keys carry every table past its first grow.
[no_aot, no_jit]
def write_interpreted {
    g_tab[EntityId(7)] = 42
    for (i in range(N)) {
        g_vec[int2(i, i * 7)] = i
        g_pts[Point3(float(i), 2.0, 3.0)] = i
    }
}

def read_aot : int {
    return g_tab ?[EntityId(7)] ?? -1
}

def read_aot_misses : int {
    var misses = 0
    for (i in range(N)) {
        misses += (g_vec ?[int2(i, i * 7)] ?? -1) != i ? 1 : 0
        misses += (g_pts ?[Point3(float(i), 2.0, 3.0)] ?? -1) != i ? 1 : 0
    }
    return misses
}

[test]
def test_cross_tier_table_hash(t : T?) {
    write_interpreted()
    t |> equal(length(g_tab), 1)
    t |> equal(read_aot(), 42)
    t |> equal(length(g_vec), N)
    t |> equal(length(g_pts), N)
    t |> equal(read_aot_misses(), 0, "every int2 and Point3 key the interpreter wrote is found from AOT")
}

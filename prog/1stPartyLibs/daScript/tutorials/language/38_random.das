// Tutorial 38: Random Numbers
//
// This tutorial covers:
//   - Seeding the RNG with random_seed
//   - Integer generation: random_int, random_big_int, random_uint
//   - Float generation: random_float, mapping to ranges
//   - Vector generation: random_float4, random_int4
//   - Unit vector and sphere: random_unit_vector, random_in_unit_sphere
//   - Infinite iterator: each_random_uint
//   - Practical examples: dice, shuffling, random pick
//
// requires: daslib/random
// Run: daslang.exe tutorials/language/38_random.das

options gen2

require daslib/random
require math

// === Seeding ===

// Every RNG function takes a mutable int4 seed.  Create one from a
// single integer with random_seed.  The same seed always produces
// the same sequence — useful for reproducible simulations.

def seeding_demo() {
    print("\n=== seeding ===\n")

    var seed = random_seed(42)
    print("seed: {seed}\n")

    // Same seed → same sequence
    var s1 = random_seed(42)
    var s2 = random_seed(42)
    print("same seed: {random_int(s1)} == {random_int(s2)}\n")

    // Different seed → different sequence
    var s3 = random_seed(99)
    var s4 = random_seed(42)
    print("diff seed: {random_int(s3)} != {random_int(s4)}\n")
}

// === Integer generation ===

def integer_demo() {
    print("\n=== integers ===\n")

    var seed = random_seed(1)

    // random_int: 0..32767 (LCG_RAND_MAX)
    print("random_int:     {random_int(seed)}\n")
    print("random_int:     {random_int(seed)}\n")

    // random_big_int: 0..32768*32768-1 (wider range)
    print("random_big_int: {random_big_int(seed)}\n")

    // random_uint: full uint range (0..4294967295)
    print("random_uint:    {random_uint(seed)}\n")
}

// === Float generation ===

def float_demo() {
    print("\n=== floats ===\n")

    var seed = random_seed(7)

    // random_float: 0.0 .. 1.0
    for (_ in range(5)) {
        print("random_float: {random_float(seed)}\n")
    }

    // Map to a custom range [lo, hi)
    let lo = 10.0
    let hi = 20.0
    let mapped = lo + random_float(seed) * (hi - lo)
    print("mapped [{lo}..{hi}): {mapped}\n")
}

// === Vector generation ===

def vector_demo() {
    print("\n=== vectors ===\n")

    var seed = random_seed(13)

    // random_int4: each component 0..32767
    let ri = random_int4(seed)
    print("random_int4:   {ri}\n")

    // random_float4: each component 0..1
    let rf = random_float4(seed)
    print("random_float4: {rf}\n")

    // random_unit_vector: normalized direction vector
    let uv = random_unit_vector(seed)
    print("unit_vector:   {uv}  length={length(uv)}\n")

    // random_in_unit_sphere: point inside unit sphere
    let sp = random_in_unit_sphere(seed)
    print("in_sphere:     {sp}  length={length(sp)}\n")

    // random_in_unit_disk: point inside unit disk (z=0)
    let dk = random_in_unit_disk(seed)
    print("in_disk:       {dk}  length={length(dk)}\n")
}

// === Infinite iterator ===

def iterator_demo() {
    print("\n=== each_random_uint ===\n")

    // each_random_uint produces an infinite stream of random uints
    var count = 0
    for (val in each_random_uint(42)) {
        print("{val} ")
        count ++
        if (count >= 5) {
            break
        }
    }
    print("\n")
}

// === Practical: dice roll ===

def roll_dice(var seed : int4&) : int {
    return (random_int(seed) % 6) + 1
}

def dice_demo() {
    print("\n=== dice rolls ===\n")
    var seed = random_seed(77)
    for (_ in range(10)) {
        print("{roll_dice(seed)} ")
    }
    print("\n")
}

// === Practical: pick random element from array ===

def random_pick(arr : array<string>; var seed : int4&) : string {
    let idx = random_int(seed) % length(arr)
    return arr[idx]
}

def pick_demo() {
    print("\n=== random pick ===\n")
    var colors <- ["red", "green", "blue", "yellow", "purple"]
    var seed = random_seed(5)
    for (_ in range(5)) {
        print("picked: {random_pick(colors, seed)}\n")
    }
}

// === Practical: Fisher-Yates shuffle ===

def shuffle(var arr : array<int>; var seed : int4&) {
    var i = length(arr) - 1
    while (i > 0) {
        let j = random_int(seed) % (i + 1)
        let tmp = arr[i]
        arr[i] = arr[j]
        arr[j] = tmp
        i --
    }
}

def shuffle_demo() {
    print("\n=== shuffle ===\n")
    var nums <- [1, 2, 3, 4, 5, 6, 7, 8]
    print("before: {nums}\n")
    var seed = random_seed(33)
    shuffle(nums, seed)
    print("after:  {nums}\n")
}

// === Summary ===
//
// +-------------------------+-------------------------------------+
// | Function                | Returns                             |
// +-------------------------+-------------------------------------+
// | random_seed(int)        | int4 seed state                     |
// | random_int(seed)        | int 0..32767                        |
// | random_big_int(seed)    | int 0..1073741823                   |
// | random_uint(seed)       | uint full range                     |
// | random_float(seed)      | float 0..1                          |
// | random_int4(seed)       | int4 each 0..32767                  |
// | random_float4(seed)     | float4 each 0..1                    |
// | random_unit_vector(s)   | float3 length=1                     |
// | random_in_unit_sphere(s)| float3 inside unit sphere           |
// | random_in_unit_disk(s)  | float3 inside unit disk (z=0)       |
// | each_random_uint(seed)  | infinite iterator<uint>             |
// +-------------------------+-------------------------------------+

[export]
def main() {
    seeding_demo()
    integer_demo()
    float_demo()
    vector_demo()
    iterator_demo()
    dice_demo()
    pick_demo()
    shuffle_demo()
    print("done\n")
}

// Note: Output contains random-looking but deterministic values (same seed = same output).
// The exact numeric values are implementation-dependent but reproducible.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/random
require math

[test]
def test_random_seeding(t : T?) {
    t |> run("same seed produces same sequence") @(t : T?) {
        var s1 = random_seed(12345)
        var s2 = random_seed(12345)
        let a1 = random_int(s1)
        let a2 = random_int(s2)
        t |> equal(a1, a2)
        let b1 = random_int(s1)
        let b2 = random_int(s2)
        t |> equal(b1, b2)
    }
    t |> run("different seeds produce different sequences") @(t : T?) {
        var s1 = random_seed(1)
        var s2 = random_seed(999)
        // Very unlikely to match
        let a1 = random_int(s1)
        let a2 = random_int(s2)
        t |> equal(a1 != a2, true)
    }
}

[test]
def test_random_float_range(t : T?) {
    t |> run("random_float is in [0,1)") @(t : T?) {
        var seed = random_seed(42)
        for (i in range(100)) {
            let f = random_float(seed)
            t |> equal(f >= 0.0, true)
            t |> equal(f < 1.0, true)
        }
    }
}

[test]
def test_random_unit_vector(t : T?) {
    t |> run("unit vector has length ~1") @(t : T?) {
        var seed = random_seed(10)
        for (i in range(50)) {
            let v = random_unit_vector(seed)
            let len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
            // Should be approximately 1.0
            t |> equal(abs(len - 1.0) < 0.001, true)
        }
    }
}

[test]
def test_random_in_unit_sphere(t : T?) {
    t |> run("point in unit sphere has length <= 1") @(t : T?) {
        var seed = random_seed(30)
        for (i in range(50)) {
            let v = random_in_unit_sphere(seed)
            let len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
            t |> equal(len <= 1.0, true)
        }
    }
}

[test]
def test_random_in_unit_disk(t : T?) {
    t |> run("point in unit disk has length <= 1") @(t : T?) {
        var seed = random_seed(50)
        for (i in range(50)) {
            let v = random_in_unit_disk(seed)
            let len = sqrt(v.x * v.x + v.y * v.y)
            t |> equal(len <= 1.0, true)
            t |> equal(v.z, 0.0)
        }
    }
}

[test]
def test_each_random_uint(t : T?) {
    t |> run("each_random_uint produces values") @(t : T?) {
        var count = 0
        var rng <- each_random_uint()
        for (v in rng) {
            count ++
            if (count >= 10) {
                break
            }
        }
        t |> equal(count, 10)
    }
}

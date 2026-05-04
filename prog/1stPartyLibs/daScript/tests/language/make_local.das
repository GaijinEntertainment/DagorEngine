options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[safe_when_uninitialized]
struct MlBar {
    x : int = 1
    y : int = 2
    z : int = 3
    w : int = 4
}

[safe_when_uninitialized]
struct MlFoo {
    i : int = 5
    f : float = 6.0
    s : string = "hello"
}

let {
    ml_testFoo = MlFoo()
}

let {
    ml_jestFoo = MlFoo(i = 3, f = 4., s = "blah")
}

def ml_test_bar(t : T?) {
    let tf = MlBar()
    t |> equal(tf.x, 1)
    t |> equal(tf.y, 2)
    t |> equal(tf.z, 3)
    t |> equal(tf.w, 4)
    let t0 = MlBar(uninitialized x = 13)
    t |> equal(t0.x, 13)
    t |> equal(t0.y, 0)
    t |> equal(t0.z, 0)
    t |> equal(t0.w, 0)
    let t1 = MlBar(x = 13)
    t |> equal(t1.x, 13)
    t |> equal(t1.y, 2)
    t |> equal(t1.z, 3)
    t |> equal(t1.w, 4)
}

[test]
def test_make_local(t : T?) {
    t |> run("bar default and uninitialized") @(t : T?) {
        ml_test_bar(t)
    }
    t |> run("global struct defaults") @(t : T?) {
        t |> equal(ml_testFoo.i, 5)
        t |> equal(ml_testFoo.f, 6.0)
        t |> equal(ml_testFoo.s, "hello")
    }
    t |> run("global struct custom init") @(t : T?) {
        t |> equal(ml_jestFoo.i, 3)
        t |> equal(ml_jestFoo.f, 4.0)
        t |> equal(ml_jestFoo.s, "blah")
    }
    t |> run("local struct defaults") @(t : T?) {
        let tf = MlFoo()
        t |> equal(tf.i, 5)
        t |> equal(tf.f, 6.0)
        t |> equal(tf.s, "hello")
    }
    t |> run("local struct uninitialized fields") @(t : T?) {
        let q = MlFoo(uninitialized i = 6, s = "world")
        t |> equal(q.i, 6)
        t |> equal(q.f, 0.0)
        t |> equal(q.s, "world")
    }
    t |> run("fixed_array of structs with uninitialized") @(t : T?) {
        let qq3 = [MlFoo(uninitialized i = 1), MlFoo(uninitialized i = 2), MlFoo(uninitialized i = 3)]
        t |> equal(qq3[0].i, 1)
        t |> equal(qq3[1].i, 2)
        t |> equal(qq3[2].i, 3)
    }
    t |> run("fixed_array of ints") @(t : T?) {
        let ar = fixed_array<int>(1, 2, 3, 4)
        t |> equal(ar[0], 1)
        t |> equal(ar[1], 2)
        t |> equal(ar[2], 3)
        t |> equal(ar[3], 4)
    }
    t |> run("fixed_array of structs") @(t : T?) {
        let arf = fixed_array<MlFoo>(MlFoo(), MlFoo())
        t |> equal(arf[0].i, 5)
        t |> equal(arf[1].i, 5)
    }
}

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require jobque

[test]
def test_atomic32_with(t : T?) {
    t |> run("with_atomic32 set/get") @(t : T?) {
        with_atomic32() $(ch) {
            ch |> set(13)
            t |> equal(13, ch |> get)
        }
    }
    t |> run("with_atomic32 inc/dec") @(t : T?) {
        with_atomic32() $(ch) {
            ch |> set(0)
            t |> equal(1, ch |> inc)
            t |> equal(2, ch |> inc)
            t |> equal(3, ch |> inc)
            t |> equal(2, ch |> dec)
            t |> equal(1, ch |> dec)
            t |> equal(0, ch |> dec)
        }
    }
    t |> run("with_atomic32 initial value is 0") @(t : T?) {
        with_atomic32() $(ch) {
            t |> equal(0, ch |> get)
        }
    }
    t |> run("with_atomic32 negative values") @(t : T?) {
        with_atomic32() $(ch) {
            ch |> set(-5)
            t |> equal(-5, ch |> get)
            t |> equal(-4, ch |> inc)
            t |> equal(-5, ch |> dec)
        }
    }
    t |> run("with_atomic32 large values") @(t : T?) {
        with_atomic32() $(ch) {
            ch |> set(int(0x7FFFFFFF))
            t |> equal(int(0x7FFFFFFF), ch |> get)
        }
    }
    t |> run("with_atomic32 overwrite") @(t : T?) {
        with_atomic32() $(ch) {
            ch |> set(10)
            t |> equal(10, ch |> get)
            ch |> set(20)
            t |> equal(20, ch |> get)
            ch |> set(30)
            t |> equal(30, ch |> get)
        }
    }
}

[test]
def test_atomic64_manual(t : T?) {
    t |> run("atomic64 create/set/get/remove") @(t : T?) {
        var ch = atomic64_create()
        ch |> set(13l)
        t |> equal(13l, ch |> get)
        t |> equal(14l, ch |> inc)
        t |> equal(13l, ch |> dec)
        unsafe(atomic64_remove(ch))
    }
    t |> run("atomic64 initial value is 0") @(t : T?) {
        var ch = atomic64_create()
        t |> equal(0l, ch |> get)
        unsafe(atomic64_remove(ch))
    }
    t |> run("atomic64 large values") @(t : T?) {
        var ch = atomic64_create()
        ch |> set(1000000000000l)
        t |> equal(1000000000000l, ch |> get)
        unsafe(atomic64_remove(ch))
    }
}

[test]
def test_atomic64_with(t : T?) {
    t |> run("with_atomic64 set/get/inc/dec") @(t : T?) {
        with_atomic64() $(ch) {
            ch |> set(100l)
            t |> equal(100l, ch |> get)
            t |> equal(101l, ch |> inc)
            t |> equal(102l, ch |> inc)
            t |> equal(101l, ch |> dec)
        }
    }
    t |> run("with_atomic64 negative values") @(t : T?) {
        with_atomic64() $(ch) {
            ch |> set(-100l)
            t |> equal(-100l, ch |> get)
            t |> equal(-99l, ch |> inc)
        }
    }
}

[test]
def test_atomic32_manual(t : T?) {
    t |> run("atomic32 create/set/get/remove") @(t : T?) {
        var ch = atomic32_create()
        ch |> set(42)
        t |> equal(42, ch |> get)
        t |> equal(43, ch |> inc)
        t |> equal(42, ch |> dec)
        unsafe(atomic32_remove(ch))
    }
}

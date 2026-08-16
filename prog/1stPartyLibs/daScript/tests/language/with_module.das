// `with (module foo)` — compile-time resolution scope: names inside resolve as if
// written in foo, including foo's privates; `_::` resolves at the enclosing module.
options gen2

require dastest/testing_boost public
require _with_module_helper
require _with_module_helper2

def local_probe() : int => 1000

[test]
def test_with_module_private_symbols(t : T?) {
    t |> run("private function") @(t : T?) {
        var r = 0
        with (module _with_module_helper) {
            r = secret_add(2, 3)
        }
        t |> equal(r, 5)
    }
    t |> run("private global read and write") @(t : T?) {
        var scale = 0
        var counter = 0
        with (module _with_module_helper) {
            scale = secret_scale
            secret_counter = 11
            counter = secret_counter
        }
        t |> equal(scale, 3)
        t |> equal(counter, 11)
    }
    t |> run("private struct and enum") @(t : T?) {
        var v = 0
        var e = 0
        with (module _with_module_helper) {
            let box = SecretBox(value = 42)
            v = box.value
            e = int(SecretEnum.Beta)
        }
        t |> equal(v, 42)
        t |> equal(e, 1)
    }
    t |> run("private type alias") @(t : T?) {
        var r = 0
        with (module _with_module_helper) {
            let v : SecretAlias = 21
            r = v * 2
        }
        t |> equal(r, 42)
    }
}

[test]
def test_with_module_scoping(t : T?) {
    t |> run("unqualified name resolves in the with-module, _:: at the enclosing module") @(t : T?) {
        var a = 0
        var b = 0
        with (module _with_module_helper) {
            a = local_probe()
            b = _::local_probe()
        }
        t |> equal(a, 7)
        t |> equal(b, 1000)
    }
    t |> run("nested with-module - innermost wins") @(t : T?) {
        var a = 0
        var b = 0
        var c = 0
        with (module _with_module_helper) {
            a = who_am_i()
            with (module _with_module_helper2) {
                b = who_am_i()
            }
            c = who_am_i()
        }
        t |> equal(a, 1)
        t |> equal(b, 2)
        t |> equal(c, 1)
    }
    t |> run("local variable shadows the module's global") @(t : T?) {
        let secret_scale = 55
        var r = 0
        with (module _with_module_helper) {
            r = secret_scale
        }
        t |> equal(r, 55)
    }
    t |> run("generic instantiated inside the block") @(t : T?) {
        var r = 0
        with (module _with_module_helper) {
            r = secret_generic(21)
        }
        t |> equal(r, 42)
    }
}

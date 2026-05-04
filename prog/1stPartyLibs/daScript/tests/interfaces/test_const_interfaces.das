options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/interfaces

// ═══════════════════════════════════════
// Const-only interface (all methods are const)
// ═══════════════════════════════════════

[interface]
class IReadable {
    def abstract const get_value() : int
    def abstract const get_name() : string
}

// Mixed interface — has both const and non-const methods
[interface]
class IMixed {
    def abstract const read_only() : int
    def abstract mutate() : void
}

// ═══════════════════════════════════════
// Implementing structs
// ═══════════════════════════════════════

[implements(IReadable)]
class Widget {
    value : int = 42
    name : string = "widget"
    def Widget(v : int; n : string) {
        value = v
        name = n
    }
    def const IReadable`get_value() : int {
        return value
    }
    def const IReadable`get_name() : string {
        return name
    }
}

[implements(IMixed)]
class Counter {
    count : int = 0
    def Counter() {
        pass
    }
    def const IMixed`read_only() : int {
        return count
    }
    def IMixed`mutate() {
        count++
    }
}

// ═══════════════════════════════════════
// Tests: const interface with let/const pointers
// ═══════════════════════════════════════

[test]
def test_const_is(tt : T?) {
    tt |> run("let ptr: is works on const pointer") @(tt : T?) {
        let w = new Widget(10, "test")
        tt |> equal(w is IReadable, true)
    }
    tt |> run("var ptr: is works on var pointer") @(tt : T?) {
        var w = new Widget(10, "test")
        tt |> equal(w is IReadable, true)
        unsafe { delete w; }
    }
}

[test]
def test_const_as(tt : T?) {
    tt |> run("let ptr: as IReadable works on const pointer") @(tt : T?) {
        let w = new Widget(10, "const-as")
        let iface = w as IReadable
        tt |> equal(iface != null, true)
        tt |> equal(iface->get_value(), 10)
        tt |> equal(iface->get_name(), "const-as")
    }
    tt |> run("var ptr: as IReadable works on var pointer") @(tt : T?) {
        var w = new Widget(20, "var-as")
        var iface = w as IReadable
        tt |> equal(iface != null, true)
        tt |> equal(iface->get_value(), 20)
        tt |> equal(iface->get_name(), "var-as")
        unsafe { delete w; }
    }
}

[test]
def test_const_safe_as(tt : T?) {
    tt |> run("let ptr: ?as IReadable works on const pointer") @(tt : T?) {
        let w = new Widget(30, "safe-as")
        let iface = w ?as IReadable
        tt |> equal(iface != null, true)
        tt |> equal(iface->get_value(), 30)
    }
    tt |> run("null const ptr: ?as IReadable returns null") @(tt : T?) {
        let nothing : Widget? const = null
        let iface = nothing ?as IReadable
        tt |> equal(iface == null, true)
    }
    tt |> run("var ptr: ?as IReadable works") @(tt : T?) {
        var w = new Widget(40, "safe-var")
        var iface = w ?as IReadable
        tt |> equal(iface != null, true)
        tt |> equal(iface->get_value(), 40)
        unsafe { delete w; }
    }
    tt |> run("null var ptr: ?as IReadable returns null") @(tt : T?) {
        var nothing : Widget?
        var iface = nothing ?as IReadable
        tt |> equal(iface == null, true)
    }
}

[test]
def test_const_function_arg(tt : T?) {
    tt |> run("pass const ptr to function using as") @(tt : T?) {
        let w = new Widget(50, "func-arg")
        let val = read_via_interface(w)
        tt |> equal(val, 50)
    }
}

def read_via_interface(w : Widget? const) : int {
    let iface = w as IReadable
    return iface->get_value()
}

// ═══════════════════════════════════════
// Tests: mixed interface still requires var
// ═══════════════════════════════════════

[test]
def test_mixed_interface_var(tt : T?) {
    tt |> run("var ptr: as IMixed works") @(tt : T?) {
        var c = new Counter()
        var iface = c as IMixed
        tt |> equal(iface != null, true)
        iface->mutate()
        iface->mutate()
        tt |> equal(iface->read_only(), 2)
        unsafe { delete c; }
    }
}

// ═══════════════════════════════════════
// Tests: lazy proxy caching works for const getter
// ═══════════════════════════════════════

[test]
def test_const_lazy_cache(tt : T?) {
    tt |> run("const getter caches the proxy") @(tt : T?) {
        let w = new Widget(60, "cache-test")
        let p1 = w as IReadable
        let p2 = w as IReadable
        tt |> equal(unsafe(reinterpret<int64>(p1)), unsafe(reinterpret<int64>(p2)))
    }
}

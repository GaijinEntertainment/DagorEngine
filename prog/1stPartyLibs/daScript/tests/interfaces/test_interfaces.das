options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/interfaces

// ═══════════════════════════════════════
// Interfaces for testing
// ═══════════════════════════════════════

[interface]
class ITick {
    def abstract tick(dt : float) : void
    def abstract beforeTick : bool
}

[interface]
class ILogger {
    def abstract log(message : string) : void
}

[interface]
class IEmpty {
    // no methods — valid but useless interface
}

// ═══════════════════════════════════════
// Implementing structs
// ═══════════════════════════════════════

// Implements both ITick and ILogger
[implements(ITick), implements(ILogger)]
class Actor {
    name : string
    ticked : int = 0
    logged : int = 0
    def Actor(n : string) {
        name = n
    }
    def ITick`tick(dt : float) {
        ticked++
    }
    def ITick`beforeTick() {
        return ticked < 10
    }
    def ILogger`log(message : string) {
        logged++
    }
}

// Implements only ITick
[implements(ITick)]
class Timer {
    elapsed : float = 0.0
    def Timer() {
        pass
    }
    def ITick`tick(dt : float) {
        elapsed += dt
    }
    def ITick`beforeTick() {
        return true
    }
}

// Implements only ILogger
[implements(ILogger)]
class ConsoleLogger {
    count : int = 0
    def ConsoleLogger() {
        pass
    }
    def ILogger`log(message : string) {
        count++
    }
}

// Implements IEmpty (empty interface)
[implements(IEmpty)]
class Placeholder {
    def Placeholder() {
        pass
    }
}

// ═══════════════════════════════════════
// Tests: basic interface dispatch
// ═══════════════════════════════════════

[test]
def test_basic_dispatch(t : T?) {

    t |> run("tick dispatch") @(t : T?) {
        var a = new Actor("hero")
        var tick_iface = a->get`ITick()
        tick_iface->tick(0.016)
        t |> equal(a.ticked, 1)
        tick_iface->tick(0.016)
        t |> equal(a.ticked, 2)
        unsafe { delete a; }
    }

    t |> run("beforeTick return value") @(t : T?) {
        var a = new Actor("hero")
        t |> equal(a->get`ITick()->beforeTick(), true)
        a.ticked = 10
        t |> equal(a->get`ITick()->beforeTick(), false)
        unsafe { delete a; }
    }

    t |> run("logger dispatch") @(t : T?) {
        var a = new Actor("hero")
        a->get`ILogger()->log("hello")
        a->get`ILogger()->log("world")
        t |> equal(a.logged, 2)
        unsafe { delete a; }
    }

    t |> run("timer ticks accumulate") @(t : T?) {
        var tm = new Timer()
        var tick_iface = tm->get`ITick()
        tick_iface->tick(1.0)
        tick_iface->tick(2.5)
        tick_iface->tick(0.5)
        t |> equal(tm.elapsed, 4.0)
        unsafe { delete tm; }
    }

    t |> run("console logger counts") @(t : T?) {
        var cl = new ConsoleLogger()
        var log_iface = cl->get`ILogger()
        log_iface->log("a")
        log_iface->log("b")
        log_iface->log("c")
        t |> equal(cl.count, 3)
        unsafe { delete cl; }
    }
}

// ═══════════════════════════════════════
// Tests: multiple interfaces on one struct
// ═══════════════════════════════════════

[test]
def test_multiple_interfaces(t : T?) {

    t |> run("actor has both ITick and ILogger") @(t : T?) {
        var a = new Actor("multi")
        a->get`ITick()->tick(0.1)
        a->get`ILogger()->log("msg")
        t |> equal(a.ticked, 1)
        t |> equal(a.logged, 1)
        unsafe { delete a; }
    }

    t |> run("proxies are independent") @(t : T?) {
        var a = new Actor("multi")
        var tick_proxy = a->get`ITick()
        var log_proxy = a->get`ILogger()
        tick_proxy->tick(1.0)
        log_proxy->log("x")
        tick_proxy->tick(1.0)
        log_proxy->log("y")
        t |> equal(a.ticked, 2)
        t |> equal(a.logged, 2)
        unsafe { delete a; }
    }
}

// ═══════════════════════════════════════
// Tests: lazy proxy construction
// ═══════════════════════════════════════

[test]
def test_lazy_proxy(t : T?) {

    t |> run("proxy is null before first get") @(t : T?) {
        var a = new Actor("lazy")
        t |> equal(a._interface_ITick == null, true)
        a->get`ITick()
        t |> equal(a._interface_ITick != null, true)
        unsafe { delete a; }
    }

    t |> run("repeated get returns same proxy") @(t : T?) {
        var a = new Actor("lazy")
        var p1 = a->get`ITick()
        var p2 = a->get`ITick()
        // both calls should return the same object
        t |> equal(unsafe(reinterpret<int64>(p1)), unsafe(reinterpret<int64>(p2)))
        unsafe { delete a; }
    }
}

// ═══════════════════════════════════════
// Tests: is operator (compile-time)
// ═══════════════════════════════════════

[test]
def test_is_operator(t : T?) {

    t |> run("Actor is ITick") @(t : T?) {
        var a = new Actor("is-test")
        t |> equal(a is ITick, true)
        unsafe { delete a; }
    }

    t |> run("Actor is ILogger") @(t : T?) {
        var a = new Actor("is-test")
        t |> equal(a is ILogger, true)
        unsafe { delete a; }
    }

    t |> run("Timer is ITick") @(t : T?) {
        var tm = new Timer()
        t |> equal(tm is ITick, true)
        unsafe { delete tm; }
    }

    t |> run("Timer is not ILogger") @(t : T?) {
        var tm = new Timer()
        t |> equal(tm is ILogger, false)
        unsafe { delete tm; }
    }

    t |> run("ConsoleLogger is ILogger") @(t : T?) {
        var cl = new ConsoleLogger()
        t |> equal(cl is ILogger, true)
        unsafe { delete cl; }
    }

    t |> run("ConsoleLogger is not ITick") @(t : T?) {
        var cl = new ConsoleLogger()
        t |> equal(cl is ITick, false)
        unsafe { delete cl; }
    }

    t |> run("Placeholder is IEmpty") @(t : T?) {
        var ph = new Placeholder()
        t |> equal(ph is IEmpty, true)
        unsafe { delete ph; }
    }

    t |> run("Placeholder is not ITick") @(t : T?) {
        var ph = new Placeholder()
        t |> equal(ph is ITick, false)
        unsafe { delete ph; }
    }
}

// ═══════════════════════════════════════
// Tests: as operator
// ═══════════════════════════════════════

[test]
def test_as_operator(t : T?) {

    t |> run("Actor as ITick returns proxy") @(t : T?) {
        var a = new Actor("as-test")
        var tick_proxy = a as ITick
        t |> equal(tick_proxy != null, true)
        tick_proxy->tick(1.0)
        t |> equal(a.ticked, 1)
        unsafe { delete a; }
    }

    t |> run("Actor as ILogger returns proxy") @(t : T?) {
        var a = new Actor("as-test")
        var log_proxy = a as ILogger
        t |> equal(log_proxy != null, true)
        log_proxy->log("via as")
        t |> equal(a.logged, 1)
        unsafe { delete a; }
    }

    t |> run("Timer as ITick works") @(t : T?) {
        var tm = new Timer()
        var tick_proxy = tm as ITick
        tick_proxy->tick(3.0)
        t |> equal(tm.elapsed, 3.0)
        unsafe { delete tm; }
    }
}

// ═══════════════════════════════════════
// Tests: ?as operator (null-safe)
// ═══════════════════════════════════════

[test]
def test_safe_as_operator(t : T?) {

    t |> run("non-null ?as returns proxy") @(t : T?) {
        var a = new Actor("safe-test")
        var tick_proxy = a ?as ITick
        t |> equal(tick_proxy != null, true)
        unsafe { delete a; }
    }

    t |> run("null ?as returns null") @(t : T?) {
        var nothing : Actor?
        var tick_proxy = nothing ?as ITick
        t |> equal(tick_proxy == null, true)
    }

    t |> run("null ?as ILogger returns null") @(t : T?) {
        var nothing : Actor?
        var log_proxy = nothing ?as ILogger
        t |> equal(log_proxy == null, true)
    }

    t |> run("non-null ?as ILogger works") @(t : T?) {
        var a = new Actor("safe-test")
        var log_proxy = a ?as ILogger
        t |> equal(log_proxy != null, true)
        unsafe { delete a; }
    }

    t |> run("Timer null ?as ITick") @(t : T?) {
        var nothing : Timer?
        var tick_proxy = nothing ?as ITick
        t |> equal(tick_proxy == null, true)
    }

    t |> run("?as dispatches correctly after null check") @(t : T?) {
        var a = new Actor("safe-dispatch")
        var tick_proxy = a ?as ITick
        if (tick_proxy != null) {
            tick_proxy->tick(0.5)
        }
        t |> equal(a.ticked, 1)
        unsafe { delete a; }
    }
}

// ═══════════════════════════════════════
// Tests: empty interface
// ═══════════════════════════════════════

[test]
def test_empty_interface(t : T?) {

    t |> run("Placeholder implements IEmpty") @(t : T?) {
        var ph = new Placeholder()
        t |> equal(ph is IEmpty, true)
        var proxy = ph as IEmpty
        t |> equal(proxy != null, true)
        unsafe { delete ph; }
    }

    t |> run("null Placeholder ?as IEmpty") @(t : T?) {
        var nothing : Placeholder?
        var proxy = nothing ?as IEmpty
        t |> equal(proxy == null, true)
    }
}

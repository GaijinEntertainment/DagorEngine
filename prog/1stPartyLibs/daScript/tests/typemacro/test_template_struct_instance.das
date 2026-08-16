options gen2

require dastest/testing_boost public
require _template_struct_instance_mod

typedef ModLevelT = float // module-level typedef: must not trip the unbound-alias check

[ |> template_struct_instance]
class template TKern {
    x : KT
    g : ModLevelT
    @template_constant WIDE : bool = true
    @template_constant OFFS : int = -3

    def plain : int {
        return x
    }
    def gated : int {
        static_if (WIDE) {
            return x + 1
        } else {
            return x + 100
        }
    }
    def doubled : int {
        return 2 * x
    }
    def offset : int {
        return x + OFFS
    }
}

class ToyNarrow : TKern {
    typedef KT = int
    override WIDE = false
    override OFFS = -7

    def override doubled : int { // instance-authored override must win over the template's
        return 20 * x
    }
    def probe : int { // template constant read from an instance-authored method
        return WIDE ? -1 : 1000 + x
    }
}

class ToyWide : TKern { // keeps the template's constant defaults
    typedef KT = int
}

[ |> template_struct_instance]
struct template PairT {
    a, b : KT
    @template_constant SCALE : int = 1
    sum : int = SCALE * 100
}

struct PairInt3 : PairT {
    typedef KT = int
    override SCALE = 3
}

struct PairInt7 : PairT {
    typedef KT = int
    override SCALE = 7
    extra : KT = 5
}

class Counter {
    hits : int = 0

    def bump : void {
        hits++
    }
    def abstract tag : int
}

[ |> template_struct_instance]
class template CKern : Counter {
    x : KT
    @template_constant STEP : int = 1

    def override tag : int {
        return x + STEP
    }
}

class CInt : CKern {
    typedef KT = int
    override STEP = 50
}

class XModInt : XModKern {
    typedef KT = int
    override BIAS = 1000
}

[tsi_test_bind_kt]
class LateInt : LateKern {
    // no typedef — [tsi_test_bind_kt] supplies KT after the reifier; unbound names are left for infer to resolve
}

def tsi_sdot(a, b : int) : int {
    return a * b
}

def tsi_ssdot(a, b : int) : int {
    return a * b * 1000
}

def tsi_via_ptr(f : function<(a, b : int) : int>; a, b : int) : int {
    return invoke(f, a, b)
}

[ |> template_struct_instance]
class template DotT {
    x : KT
    @template_call tsi_sdot = @@tsi_sdot // the field name is what bodies call; the init is the default target

    def run(v : KT) : KT {
        return tsi_sdot(x, v)
    }
    def run_ptr(v : KT) : KT {
        return tsi_via_ptr(@@tsi_sdot, x, v) // @@ address targets rebind too
    }
    def run_qual(v : KT) : KT {
        return _::tsi_sdot(x, v) // `_::` keeps normal resolution: pinned to the real function, past any rebind
    }
}

class DotDefault : DotT {
    typedef KT = int
}

class DotWide : DotT {
    typedef KT = int
    override tsi_sdot = @@tsi_ssdot
}

class DotStr : DotT {
    typedef KT = int
    override tsi_sdot = "tsi_ssdot"
}

[test]
def test_tsi_class_rail(t : T?) {
    t |> run("stamped methods, static_if fold, override wins, constants") @(t : T?) {
        unsafe {
            var inscope k <- new ToyNarrow()
            k.x = 42
            t |> equal(k->plain(), 42)
            t |> equal(k->gated(), 142)   // WIDE=false picks the static_if else-branch
            t |> equal(k->doubled(), 840) // the instance override, not the template's 2*x
            t |> equal(k->offset(), 35)   // signed-literal constant: OFFS=-7
            t |> equal(k->probe(), 1042)  // constant substituted in an instance-authored body
        }
    }
    t |> run("second instance keeps the template's constant defaults") @(t : T?) {
        unsafe {
            var inscope k <- new ToyWide()
            k.x = 42
            t |> equal(k->gated(), 43)   // WIDE=true default
            t |> equal(k->offset(), 39)  // OFFS=-3 default
            t |> equal(k->doubled(), 84) // the template's own doubled
        }
    }
}

[test]
def test_tsi_struct_rail(t : T?) {
    t |> run("two instances of one struct template") @(t : T?) {
        let p = PairInt3(a = 2, b = 3)
        let q = PairInt7(a = 10, b = 20)
        t |> equal(p.sum, 300) // SCALE=3 folded into the field init
        t |> equal(q.sum, 700)
        t |> equal(q.extra, 5) // instance-own field
        t |> equal(p.a + p.b, 5)
        t |> equal(q.a + q.b, 30)
    }
}

[test]
def test_tsi_base_class(t : T?) {
    t |> run("template's own base class survives reification") @(t : T?) {
        unsafe {
            var inscope k <- new CInt()
            k.x = 7
            k->bump()
            k->bump()
            var base : Counter? = k
            t |> equal(k.hits, 2)       // inherited base method
            t |> equal(base->tag(), 57) // dispatch through the base hits the stamped override
        }
    }
}

[test]
def test_tsi_cross_module(t : T?) {
    t |> run("template required from another module") @(t : T?) {
        unsafe {
            var inscope k <- new XModInt()
            k.x = 42
            t |> equal(k->value(), 1042)
        }
    }
}

[test]
def test_tsi_late_bind(t : T?) {
    t |> run("an alias another macro supplies after the reifier binds at infer") @(t : T?) {
        unsafe {
            var inscope k <- new LateInt()
            k.x = 40
            t |> equal(k->take(2), 42)
        }
    }
}

[test]
def test_tsi_call_rebind(t : T?) {
    t |> run("@template_call rebinds calls and @@ addresses per instance") @(t : T?) {
        unsafe {
            var inscope a <- new DotDefault()
            var inscope b <- new DotWide()
            var inscope c <- new DotStr()
            a.x = 6
            b.x = 6
            c.x = 6
            t |> equal(a->run(7), 42)        // default: the real tsi_sdot
            t |> equal(b->run(7), 42000)     // @@tsi_ssdot rebind
            t |> equal(c->run(7), 42000)     // string-form rebind
            t |> equal(a->run_ptr(7), 42)    // @@ address target, default
            t |> equal(b->run_ptr(7), 42000) // @@ address target, rebound
            t |> equal(a->run_qual(7), 42) // `_::` spelling pinned to the real callee...
            t |> equal(b->run_qual(7), 42) // ...even on the rebinding instance, by design
        }
    }
}

[ |> template_struct_instance]
struct template GatePairT {
    a : KT
    @template_gate = BATCHED rt : KT
    @template_gate = "!BATCHED" solo : KT
    @template_constant BATCHED : bool = false
}

struct GateOff : GatePairT {
    typedef KT = int
}

struct GateOn : GatePairT {
    typedef KT = int
    override BATCHED = true
}

[ |> template_struct_instance]
class template GKernT {
    x : int
    @template_gate = EXT ext : int
    @template_constant EXT : bool = false

    def sum : int {
        static_if (EXT) {
            return x + ext
        } else {
            return x
        }
    }
}

class GBase : GKernT {}

class GExt : GKernT {
    override EXT = true
}

[test]
def test_tsi_gated_fields(t : T?) {
    t |> run("a gated field exists only where its axis says so, both polarities") @(t : T?) {
        // each stamp dropped exactly one of the two gated ints — both remaining would be 12
        t |> equal(int(typeinfo sizeof(type<GateOff>)), 8)
        t |> equal(int(typeinfo sizeof(type<GateOn>)), 8)
        let off = GateOff(a = 1, solo = 3) // named-arg construction pins WHICH field survived
        let on = GateOn(a = 1, rt = 2)
        t |> equal(off.a + off.solo, 4)
        t |> equal(on.a + on.rt, 3)
    }
    t |> run("a dead static_if arm may reference a gated-off field") @(t : T?) {
        unsafe {
            var inscope b <- new GBase()
            var inscope e <- new GExt()
            b.x = 40
            e.x = 40
            e.ext = 2
            t |> equal(b->sum(), 40) // EXT=false: ext erased, the dead arm still names it
            t |> equal(e->sum(), 42)
        }
    }
}

[ |> template_struct_instance]
class template TagKernT {
    x : KT

    [tsi_test_fn_tag]
    def tick : KT {
        return x + x
    }
}

class TagInt : TagKernT {
    typedef KT = int
}

[test]
def test_tsi_method_annotation_apply(t : T?) {
    t |> run("stamped methods get their function-annotation applies") @(t : T?) {
        // TsiTestFnTag`apply added this global when the reifier stamped TagInt`tick —
        // referencing it IS the assertion (and a double apply fails the compile loudly)
        t |> equal(TagInt_tick_tag, 0)
        unsafe {
            var inscope k <- new TagInt()
            k.x = 21
            t |> equal(k->tick(), 42)
        }
    }
}

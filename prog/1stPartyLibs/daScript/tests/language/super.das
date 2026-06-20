// Tests `super` keyword: super() for parent constructor, super.method() for parent method calls
options gen2
require dastest/testing_boost public

class Base {
    x : int = 0
    def Base {
        x = 10
    }
    def Base(val : int) {
        x = val
    }
    def get_value() : int {
        return x
    }
    def add(a, b : int) : int {
        return a + b
    }
}

class Derived : Base {
    y : int = 0
    def Derived {
        super()
        y = 20
    }
    def Derived(val : int) {
        super(val)
        y = val * 2
    }
    def override get_value() : int {
        return x + y
    }
    def override add(a, b : int) : int {
        return super.add(a, b) * 2
    }
    def get_base_value() : int {
        return super.get_value()
    }
}

class Derived2 : Derived {
    z : int = 0
    def Derived2 {
        super()
        z = 30
    }
    def override get_value() : int {
        return x + y + z
    }
    def override add(a, b : int) : int {
        return super.add(a, b) + z
    }
    def get_derived_value() : int {
        return super.get_value()
    }
    def get_base_value_from_derived2() : int {
        return super.get_base_value()
    }
}

// Both if/else branches call super — CFG walker must accept (min=max=1 per path).
class CondSuper : Base {
    z : int = 0
    def CondSuper(flag : bool) {
        if (flag) {
            super(7)
        } else {
            super()
        }
        z = flag ? 100 : 200
    }
}

// super() before an early return — both paths have exactly one super call. CFG walker
// must accept (the if-true path's super count matches the fall-through path's count).
class EarlyReturnSuper : Base {
    z : int = 0
    def EarlyReturnSuper(early : bool) {
        super()
        if (early) return
        z = 999
    }
}

// super() inside a try block — runs on normal path. CFG walker must count it.
// recover is for diagnostics-before-exit (panic is fatal) and is ignored by the lint.
class TrySuper : Base {
    z : int = 0
    def TrySuper {
        try {
            super(5)
        } recover {
            z = -1
        }
        z = 500
    }
}

// Synthesized chain: ImplicitChain has no constructor; the compiler emits one that
// chains super() → Base default ctor. new ImplicitChain() must run Base, setting x=10.
class ImplicitChain : Base {
    z : int = 999
}

// Two-level synth: both MidImplicit and LeafImplicit have no constructor. The compiler
// must synthesize ctors for both, chaining MidImplicit → Base and LeafImplicit → MidImplicit.
class MidImplicit : Base {
    m : int = 50
}

class LeafImplicit : MidImplicit {
    leaf : int = 70
}

// Synth respects parent ctor with default arg: BaseWithDefault has def BaseWithDefault(v : int = 99).
// DerivedWithDefault has no ctor, must synth a chain that resolves to BaseWithDefault`BaseWithDefault(self).
class BaseWithDefault {
    bx : int = 0
    def BaseWithDefault(v : int = 99) {
        bx = v
    }
}

class DerivedWithDefault : BaseWithDefault {
    dx : int = 5
}

// Skip-level inheritance: SkipMid is empty (inherits Base's constructor and methods).
// SkipLeaf calls super() / super.method() — both must walk past SkipMid up to Base.
class SkipMid : Base {
}

class SkipLeaf : SkipMid {
    z : int = 0
    def SkipLeaf {
        super()
        z = 100
    }
    def SkipLeaf(val : int) {
        super(val)
        z = val + 100
    }
    def override get_value() : int {
        return x + z
    }
    def get_super_value() : int {
        return super.get_value()
    }
    def override add(a, b : int) : int {
        return super.add(a, b) + z
    }
}

[test]
def test_super(t : T?) {
    t |> run("super() calls parent default constructor") @(t : T?) {
        unsafe {
            let d = Derived()
            t |> equal(d.x, 10)
            t |> equal(d.y, 20)
        }
    }
    t |> run("super(args) calls parent parameterized constructor") @(t : T?) {
        unsafe {
            let d = Derived(5)
            t |> equal(d.x, 5)
            t |> equal(d.y, 10)
        }
    }
    t |> run("super.method() calls parent method bypassing override") @(t : T?) {
        unsafe {
            var d = Derived()
            // virtual dispatch: calls Derived.get_value
            t |> equal(d->get_value(), 30)
            // super.get_value in get_base_value: calls Base.get_value
            t |> equal(d->get_base_value(), 10)
        }
    }
    t |> run("super.method(args) calls parent method with arguments") @(t : T?) {
        unsafe {
            var d = Derived()
            // virtual dispatch: calls Derived.add which doubles super.add
            t |> equal(d->add(3, 4), 14)
        }
    }
    t |> run("super in 3-level hierarchy") @(t : T?) {
        unsafe {
            var d2 = Derived2()
            // constructor chain: Base(x=10) -> Derived(y=20) -> Derived2(z=30)
            t |> equal(d2.x, 10)
            t |> equal(d2.y, 20)
            t |> equal(d2.z, 30)
            // Derived2.get_value: x+y+z
            t |> equal(d2->get_value(), 60)
            // Derived2.get_derived_value calls super.get_value = Derived.get_value = x+y
            t |> equal(d2->get_derived_value(), 30)
            // Derived2.get_base_value_from_derived2 calls super.get_base_value = Derived.get_base_value = super.get_value = Base.get_value = x
            t |> equal(d2->get_base_value_from_derived2(), 10)
        }
    }
    t |> run("super.method(args) in 3-level hierarchy") @(t : T?) {
        unsafe {
            var d2 = Derived2()
            // Derived2.add calls super.add(a,b) + z
            // super.add = Derived.add = super.add(a,b) * 2
            // Derived's super.add = Base.add = a + b
            // so: (3+4)*2 + 30 = 44
            t |> equal(d2->add(3, 4), 44)
        }
    }
    t |> run("super via pointer with virtual dispatch") @(t : T?) {
        var d = new Derived()
        // virtual dispatch on pointer
        t |> equal(d->get_value(), 30)
        t |> equal(d->get_base_value(), 10)
        t |> equal(d->add(3, 4), 14)
        // cast to base pointer — virtual dispatch still calls Derived methods
        var b = cast<Base?> d
        t |> equal(b->get_value(), 30)
        t |> equal(b->add(3, 4), 14)
        unsafe {
            delete d
        }
    }
    t |> run("super() walks past empty intermediate class") @(t : T?) {
        unsafe {
            // SkipMid has no constructor of its own; super() in SkipLeaf must reach Base.
            let s = SkipLeaf()
            t |> equal(s.x, 10)   // Base default ctor ran
            t |> equal(s.z, 100)
        }
    }
    t |> run("super(args) walks past empty intermediate class") @(t : T?) {
        unsafe {
            let s = SkipLeaf(7)
            t |> equal(s.x, 7)
            t |> equal(s.z, 107)
        }
    }
    t |> run("super.method() walks past empty intermediate class") @(t : T?) {
        unsafe {
            var s = SkipLeaf()
            // SkipLeaf.get_super_value calls super.get_value; SkipMid has no own override,
            // so the call must resolve to Base.get_value (returns x).
            t |> equal(s->get_super_value(), 10)
        }
    }
    t |> run("super.method(args) walks past empty intermediate class") @(t : T?) {
        unsafe {
            var s = SkipLeaf()
            // SkipLeaf.add calls super.add(a,b) + z; SkipMid has no add, walks to Base: a+b
            t |> equal(s->add(3, 4), 107)
        }
    }
    t |> run("super called on every branch of if/else (CFG-aware lint)") @(t : T?) {
        var c1 = new CondSuper(true)
        // flag=true → super(7) → x=7, z=100
        t |> equal(c1.x, 7)
        t |> equal(c1.z, 100)
        var c2 = new CondSuper(false)
        // flag=false → super() → x=10, z=200
        t |> equal(c2.x, 10)
        t |> equal(c2.z, 200)
        unsafe {
            delete c1
            delete c2
        }
    }
    t |> run("super in try block (CFG-aware lint accepts)") @(t : T?) {
        var ts = new TrySuper()
        // normal path: try runs super(5), z=500
        t |> equal(ts.x, 5)
        t |> equal(ts.z, 500)
        unsafe { delete ts; }
    }
    t |> run("synth default ctor chains super to Base") @(t : T?) {
        unsafe {
            // No def ImplicitChain(); the compiler synthesizes one that calls Base`Base(self).
            let ic = ImplicitChain()
            t |> equal(ic.x, 10)   // Base default ctor ran
            t |> equal(ic.z, 999)
        }
        var icp = new ImplicitChain()
        t |> equal(icp.x, 10)
        t |> equal(icp.z, 999)
        unsafe { delete icp; }
    }
    t |> run("synth default ctor chains through two levels") @(t : T?) {
        unsafe {
            // Neither MidImplicit nor LeafImplicit declare a ctor.
            // LeafImplicit's synth chains → MidImplicit's synth → Base.
            let li = LeafImplicit()
            t |> equal(li.x, 10)    // Base ran
            t |> equal(li.m, 50)
            t |> equal(li.leaf, 70)
        }
    }
    t |> run("synth chain resolves to parent ctor with default arg") @(t : T?) {
        unsafe {
            // BaseWithDefault has def BaseWithDefault(v : int = 99); synth chain
            // must resolve to a 0-arg-callable signature.
            let d = DerivedWithDefault()
            t |> equal(d.bx, 99)    // default arg used
            t |> equal(d.dx, 5)
        }
    }
    t |> run("super before early return (both paths have super=1)") @(t : T?) {
        var e1 = new EarlyReturnSuper(true)
        // early=true → super ran, then returned before z=999; z stays default 0
        t |> equal(e1.x, 10)
        t |> equal(e1.z, 0)
        var e2 = new EarlyReturnSuper(false)
        // early=false → super, no return, z=999
        t |> equal(e2.x, 10)
        t |> equal(e2.z, 999)
        unsafe {
            delete e1
            delete e2
        }
    }
}

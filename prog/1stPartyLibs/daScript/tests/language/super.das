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

[test]
def test_super(t : T?) {
    t |> run("super() calls parent default constructor") @(t : T?) {
        unsafe {
            var d = Derived()
            t |> equal(d.x, 10)
            t |> equal(d.y, 20)
        }
    }
    t |> run("super(args) calls parent parameterized constructor") @(t : T?) {
        unsafe {
            var d = Derived(5)
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
}

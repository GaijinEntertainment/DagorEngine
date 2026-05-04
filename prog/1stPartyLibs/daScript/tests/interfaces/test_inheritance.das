options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/interfaces

// ═══════════════════════════════════════
// Interface hierarchy for testing
// ═══════════════════════════════════════

// Base interface
[interface]
class IAnimal {
    def abstract name : string
    def abstract sound : string
}

// Derived interface — inherits name() and sound(), adds move()
[interface]
class IPet : IAnimal {
    def abstract move(distance : float) : void
}

// Two-level hierarchy: IGuideDog : IPet : IAnimal
[interface]
class IGuideDog : IPet {
    def abstract guide(destination : string) : void
}

// Separate interface, not in the hierarchy
[interface]
class ISerializable {
    def abstract serialize : string
}

// ═══════════════════════════════════════
// Interface with default methods
// ═══════════════════════════════════════

[interface]
class IGreeter {
    def abstract greet_name : string
    // Default method — does not need to be overridden
    def greet : string {
        return "Hello, {self->greet_name()}!"
    }
}

// Interface with ALL methods having defaults
[interface]
class IDefaults {
    def default_value : int {
        return 42
    }
    def default_message : string {
        return "default"
    }
}

// ═══════════════════════════════════════
// Implementing structs
// ═══════════════════════════════════════

// Implements the full chain: IGuideDog (which includes IPet and IAnimal)
[implements(IGuideDog), implements(ISerializable)]
class Dog {
    breed : string
    total_distance : float = 0.0
    guided_to : string = ""
    def Dog(b : string) {
        breed = b
    }
    // IAnimal methods (via IGuideDog → IPet → IAnimal)
    def IGuideDog`name() : string {
        return "Dog({breed})"
    }
    def IGuideDog`sound() : string {
        return "Woof"
    }
    // IPet method (via IGuideDog → IPet)
    def IGuideDog`move(distance : float) {
        total_distance += distance
    }
    // IGuideDog's own method
    def IGuideDog`guide(destination : string) {
        guided_to = destination
    }
    // ISerializable (separate hierarchy)
    def ISerializable`serialize() : string {
        return "Dog:{breed}"
    }
}

// Implements only IPet (middle of the chain)
[implements(IPet)]
class Cat {
    cat_name : string
    moved : float = 0.0
    def Cat(n : string) {
        cat_name = n
    }
    def IPet`name() : string {
        return cat_name
    }
    def IPet`sound() : string {
        return "Meow"
    }
    def IPet`move(distance : float) {
        moved += distance
    }
}

// Implements IGreeter — only provides greet_name, uses default greet()
[implements(IGreeter)]
class SimpleGreeter {
    the_name : string
    def SimpleGreeter(n : string) {
        the_name = n
    }
    def IGreeter`greet_name() : string {
        return the_name
    }
    // greet() NOT overridden — uses default from IGreeter
}

// Implements IGreeter — overrides both methods
[implements(IGreeter)]
class FormalGreeter {
    the_name : string
    def FormalGreeter(n : string) {
        the_name = n
    }
    def IGreeter`greet_name() : string {
        return the_name
    }
    def IGreeter`greet() : string {
        return "Good day, {the_name}. How do you do?"
    }
}

// Implements IDefaults — overrides nothing (all methods have defaults)
[implements(IDefaults)]
class AllDefaults {
    def AllDefaults() {
        pass
    }
    // No method overrides needed!
}

// Implements IDefaults — overrides one of two
[implements(IDefaults)]
class PartialOverride {
    def PartialOverride() {
        pass
    }
    def IDefaults`default_value() : int {
        return 99
    }
    // default_message() NOT overridden — uses default
}

// ═══════════════════════════════════════
// Tests: interface inheritance — is operator
// ═══════════════════════════════════════

[test]
def test_inheritance_is(t : T?) {

    t |> run("Dog is IGuideDog") @(t : T?) {
        var d = new Dog("Labrador")
        t |> equal(d is IGuideDog, true)
        unsafe { delete d; }
    }

    t |> run("Dog is IPet (ancestor)") @(t : T?) {
        var d = new Dog("Labrador")
        t |> equal(d is IPet, true)
        unsafe { delete d; }
    }

    t |> run("Dog is IAnimal (grandparent)") @(t : T?) {
        var d = new Dog("Labrador")
        t |> equal(d is IAnimal, true)
        unsafe { delete d; }
    }

    t |> run("Dog is ISerializable") @(t : T?) {
        var d = new Dog("Labrador")
        t |> equal(d is ISerializable, true)
        unsafe { delete d; }
    }

    t |> run("Cat is IPet") @(t : T?) {
        var c = new Cat("Whiskers")
        t |> equal(c is IPet, true)
        unsafe { delete c; }
    }

    t |> run("Cat is IAnimal (via IPet)") @(t : T?) {
        var c = new Cat("Whiskers")
        t |> equal(c is IAnimal, true)
        unsafe { delete c; }
    }

    t |> run("Cat is not IGuideDog") @(t : T?) {
        var c = new Cat("Whiskers")
        t |> equal(c is IGuideDog, false)
        unsafe { delete c; }
    }

    t |> run("Cat is not ISerializable") @(t : T?) {
        var c = new Cat("Whiskers")
        t |> equal(c is ISerializable, false)
        unsafe { delete c; }
    }
}

// ═══════════════════════════════════════
// Tests: interface inheritance — as operator
// ═══════════════════════════════════════

[test]
def test_inheritance_as(t : T?) {

    t |> run("Dog as IAnimal — name dispatches") @(t : T?) {
        var d = new Dog("Labrador")
        var animal = d as IAnimal
        t |> equal(animal != null, true)
        t |> equal(animal->name(), "Dog(Labrador)")
        unsafe { delete d; }
    }

    t |> run("Dog as IAnimal — sound dispatches") @(t : T?) {
        var d = new Dog("Poodle")
        var animal = d as IAnimal
        t |> equal(animal->sound(), "Woof")
        unsafe { delete d; }
    }

    t |> run("Dog as IPet — move dispatches") @(t : T?) {
        var d = new Dog("Labrador")
        var pet = d as IPet
        pet->move(5.0)
        pet->move(3.0)
        t |> equal(d.total_distance, 8.0)
        unsafe { delete d; }
    }

    t |> run("Dog as IGuideDog — guide dispatches") @(t : T?) {
        var d = new Dog("Labrador")
        var guide = d as IGuideDog
        guide->guide("park")
        t |> equal(d.guided_to, "park")
        unsafe { delete d; }
    }

    t |> run("Cat as IAnimal — dispatches via ancestor") @(t : T?) {
        var c = new Cat("Whiskers")
        var animal = c as IAnimal
        t |> equal(animal->name(), "Whiskers")
        t |> equal(animal->sound(), "Meow")
        unsafe { delete c; }
    }

    t |> run("Cat as IPet — move dispatches") @(t : T?) {
        var c = new Cat("Whiskers")
        var pet = c as IPet
        pet->move(10.0)
        t |> equal(c.moved, 10.0)
        unsafe { delete c; }
    }
}

// ═══════════════════════════════════════
// Tests: interface inheritance — ?as operator
// ═══════════════════════════════════════

[test]
def test_inheritance_safe_as(t : T?) {

    t |> run("Dog ?as IAnimal — non-null") @(t : T?) {
        var d = new Dog("Labrador")
        var animal = d ?as IAnimal
        t |> equal(animal != null, true)
        t |> equal(animal->name(), "Dog(Labrador)")
        unsafe { delete d; }
    }

    t |> run("null Dog ?as IAnimal") @(t : T?) {
        var nothing : Dog?
        var animal = nothing ?as IAnimal
        t |> equal(animal == null, true)
    }

    t |> run("null Cat ?as IPet") @(t : T?) {
        var nothing : Cat?
        var pet = nothing ?as IPet
        t |> equal(pet == null, true)
    }

    t |> run("null Cat ?as IAnimal") @(t : T?) {
        var nothing : Cat?
        var animal = nothing ?as IAnimal
        t |> equal(animal == null, true)
    }
}

// ═══════════════════════════════════════
// Tests: default method implementations
// ═══════════════════════════════════════

[test]
def test_default_methods(t : T?) {

    t |> run("SimpleGreeter uses default greet()") @(t : T?) {
        var g = new SimpleGreeter("Alice")
        var iface = g as IGreeter
        t |> equal(iface->greet(), "Hello, Alice!")
        unsafe { delete g; }
    }

    t |> run("SimpleGreeter greet_name() works directly") @(t : T?) {
        var g = new SimpleGreeter("Bob")
        var iface = g as IGreeter
        t |> equal(iface->greet_name(), "Bob")
        unsafe { delete g; }
    }

    t |> run("FormalGreeter overrides default greet()") @(t : T?) {
        var g = new FormalGreeter("Alice")
        var iface = g as IGreeter
        t |> equal(iface->greet(), "Good day, Alice. How do you do?")
        unsafe { delete g; }
    }

    t |> run("AllDefaults — no overrides needed") @(t : T?) {
        var a = new AllDefaults()
        var iface = a as IDefaults
        t |> equal(iface->default_value(), 42)
        t |> equal(iface->default_message(), "default")
        unsafe { delete a; }
    }

    t |> run("PartialOverride — one overridden, one default") @(t : T?) {
        var p = new PartialOverride()
        var iface = p as IDefaults
        t |> equal(iface->default_value(), 99)
        t |> equal(iface->default_message(), "default")
        unsafe { delete p; }
    }
}

// ═══════════════════════════════════════
// Tests: mixed scenarios
// ═══════════════════════════════════════

[test]
def test_mixed_scenarios(t : T?) {

    t |> run("Dog accessible via all three interfaces in hierarchy") @(t : T?) {
        var d = new Dog("Retriever")

        // Access via most-derived
        var guide = d as IGuideDog
        guide->guide("store")
        t |> equal(d.guided_to, "store")

        // Access via middle interface
        var pet = d as IPet
        pet->move(7.5)
        t |> equal(d.total_distance, 7.5)

        // Access via base interface
        var animal = d as IAnimal
        t |> equal(animal->name(), "Dog(Retriever)")

        // Also serializable (separate hierarchy)
        var ser = d as ISerializable
        t |> equal(ser->serialize(), "Dog:Retriever")

        unsafe { delete d; }
    }

    t |> run("Cat — only IPet and IAnimal, not IGuideDog or ISerializable") @(t : T?) {
        var c = new Cat("Felix")
        t |> equal(c is IPet, true)
        t |> equal(c is IAnimal, true)
        t |> equal(c is IGuideDog, false)
        t |> equal(c is ISerializable, false)
        unsafe { delete c; }
    }
}

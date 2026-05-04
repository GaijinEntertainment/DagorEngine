// Tutorial 43: Interfaces
//
// This tutorial covers:
//   - Defining interfaces with [interface]
//   - Implementing interfaces with [implements(IFoo)]
//   - The is / as / ?as operators for interface types
//   - Multiple interfaces on a single struct
//   - Interface inheritance (IChild : IParent)
//   - Default method implementations
//   - Completeness checking (compile-time error on missing methods)
//   - Polymorphic dispatch via interface pointers
//   - Const-only interfaces (as/is/?as on const pointers)
//
// Prerequisites: Tutorial 39 (Dynamic Type Checking)
//
// Run: daslang.exe tutorials/language/43_interfaces.das

options gen2

require daslib/interfaces

// ============================================================
// Section 1: Defining and implementing an interface
// ============================================================
// [interface] marks a class as an interface — it may only contain
// function-typed fields (abstract or default).
// [implements(IFoo)] generates all the plumbing: a proxy class,
// a cached getter, and method delegates.

[interface]
class IGreeter {
    def abstract greet(name : string) : string
}

[implements(IGreeter)]
class FriendlyGreeter {
    def FriendlyGreeter() {
        pass
    }
    // Method names are prefixed with the interface name and a backtick:
    //   IGreeter`greet  — implements IGreeter.greet
    def IGreeter`greet(name : string) : string {
        return "Hey, {name}!"
    }
}

def demo_basic_interface() {
    print("=== basic interface ===\n")
    var g = new FriendlyGreeter()

    // Direct getter call (original pattern)
    let greet_alice = g->get`IGreeter()->greet("Alice")
    print("  getter:  {greet_alice}\n")

    // `as` — returns the interface proxy
    var iface = g as IGreeter
    let greet_bob = iface->greet("Bob")
    print("  as:      {greet_bob}\n")
}

// ============================================================
// Section 2: is / as / ?as operators
// ============================================================
// The InterfaceAsIs variant macro enables three operators:
//   ptr is IFoo   — compile-time boolean (true if struct implements IFoo)
//   ptr as IFoo   — returns the interface proxy
//   ptr ?as IFoo  — null-safe: returns null when the pointer is null

[interface]
class IDrawable {
    def abstract draw(x, y : int) : void
}

[implements(IDrawable)]
class Circle {
    radius : float
    def Circle(r : float) {
        radius = r
    }
    def IDrawable`draw(x, y : int) {
        print("    Circle(r={radius}) at ({x},{y})\n")
    }
}

def demo_is_as_operators() {
    print("\n=== is / as / ?as ===\n")

    var c = new Circle(5.0)

    // is — compile-time check
    print("  c is IDrawable  = {c is IDrawable}\n")
    print("  c is IGreeter   = {c is IGreeter}\n")

    // as — get proxy
    var drawable = c as IDrawable
    drawable->draw(10, 20)

    // ?as — null-safe
    var nothing : Circle?
    var safe = nothing ?as IDrawable
    print("  null ?as IDrawable = {safe}\n")
}

// ============================================================
// Section 3: Multiple interfaces
// ============================================================
// A struct can implement any number of interfaces by listing
// multiple [implements(...)] annotations.

[interface]
class ISerializable {
    def abstract serialize : string
}

[implements(IDrawable), implements(ISerializable)]
class Sprite {
    name : string
    def Sprite(n : string) {
        name = n
    }
    def IDrawable`draw(x, y : int) {
        print("    Sprite \"{name}\" at ({x},{y})\n")
    }
    def ISerializable`serialize() : string {
        return "sprite:{name}"
    }
}

def demo_multiple_interfaces() {
    print("\n=== multiple interfaces ===\n")

    var s = new Sprite("hero")

    print("  is IDrawable     = {s is IDrawable}\n")
    print("  is ISerializable = {s is ISerializable}\n")
    print("  is IGreeter      = {s is IGreeter}\n")

    var d = s as IDrawable
    d->draw(0, 0)

    var ser = s as ISerializable
    print("  serialize = {ser->serialize()}\n")
}

// ============================================================
// Section 4: Polymorphic dispatch
// ============================================================
// Interfaces let you write functions that accept any struct
// implementing a given interface.  Pass the proxy pointer.

def draw_all(var objects : array<IDrawable?>) {
    for (obj in objects) {
        obj->draw(0, 0)
    }
}

def demo_polymorphic_dispatch() {
    print("\n=== polymorphic dispatch ===\n")

    var c = new Circle(3.0)
    var s = new Sprite("tree")

    var drawables : array<IDrawable?>
    drawables |> push(c as IDrawable)
    drawables |> push(s as IDrawable)

    draw_all(drawables)
}

// ============================================================
// Section 5: Interface inheritance
// ============================================================
// Interfaces can extend other interfaces using normal class
// inheritance syntax.  A struct that implements a derived
// interface automatically supports is/as/?as for all ancestors.

[interface]
class IAnimal {
    def abstract name : string
    def abstract sound : string
}

[interface]
class IPet : IAnimal {
    def abstract owner : string
}

[implements(IPet)]
class Dog {
    dog_name : string
    owner_name : string
    def Dog(n, o : string) {
        dog_name = n
        owner_name = o
    }
    def IPet`name() : string {
        return dog_name
    }
    def IPet`sound() : string {
        return "Woof"
    }
    def IPet`owner() : string {
        return owner_name
    }
}

def demo_interface_inheritance() {
    print("\n=== interface inheritance ===\n")

    var d = new Dog("Rex", "Alice")

    // Dog implements IPet, which extends IAnimal.
    // Both interfaces are accessible:
    print("  d is IPet    = {d is IPet}\n")
    print("  d is IAnimal = {d is IAnimal}\n")

    // as IPet — gets the derived interface proxy
    var pet = d as IPet
    print("  pet->name()  = {pet->name()}\n")
    print("  pet->owner() = {pet->owner()}\n")

    // as IAnimal — gets the ancestor interface proxy
    var animal = d as IAnimal
    print("  animal->name()  = {animal->name()}\n")
    print("  animal->sound() = {animal->sound()}\n")
}

// ============================================================
// Section 6: Default method implementations
// ============================================================
// Non-abstract methods in an interface provide defaults.
// Implementing structs may override them or leave them as-is.

[interface]
class ILogger {
    def abstract log_name : string
    // Default implementation — not required to override
    def format(message : string) : string {
        return "[{self->log_name()}] {message}"
    }
}

[implements(ILogger)]
class SimpleLogger {
    def SimpleLogger() {
        pass
    }
    def ILogger`log_name() : string {
        return "simple"
    }
    // format() uses the default from ILogger
}

[implements(ILogger)]
class FancyLogger {
    def FancyLogger() {
        pass
    }
    def ILogger`log_name() : string {
        return "fancy"
    }
    // Override the default
    def ILogger`format(message : string) : string {
        return "*** [fancy] {message} ***"
    }
}

def demo_default_methods() {
    print("\n=== default methods ===\n")

    var s = new SimpleLogger()
    var f = new FancyLogger()

    var sl = s as ILogger
    var fl = f as ILogger

    let simple_msg = sl->format("hello")
    let fancy_msg = fl->format("hello")
    print("  simple: {simple_msg}\n")
    print("  fancy:  {fancy_msg}\n")
}

// ============================================================
// Section 7: Completeness checking
// ============================================================
// If you forget to implement an abstract method, the compiler
// reports an error at compile time.  Try commenting out one of
// the interface methods above to see the error:
//
//   error[30111]: Foo does not implement IBar.method
//
// Methods with default implementations are optional — the proxy
// inherits the default from the interface class.

def demo_completeness_note() {
    print("\n=== completeness checking ===\n")
    print("  (compile-time only — no demo output)\n")
    print("  Try removing a method to see error 30111\n")
}

// ============================================================
// Section 8: Const-only interfaces
// ============================================================
// When all methods in an interface are `def abstract const`,
// the generated getter also works on const pointers.
// This means `is`, `as`, and `?as` work without requiring a
// mutable (var) pointer.

[interface]
class IReadable {
    def abstract const read() : string
}

[implements(IReadable)]
class Document {
    text : string
    def Document(t : string) {
        text = t
    }
    def IReadable`read() : string {
        return text
    }
}

def print_content(doc : Document? const) {
    // `as` works on a const pointer because IReadable is const-only
    var reader = doc as IReadable
    print("  content: {reader->read()}\n")
}

def demo_const_interfaces() {
    print("\n=== const-only interfaces ===\n")

    var doc = new Document("Hello from a const pointer!")

    // is / as / ?as all work on const pointers
    let c : Document? const = doc
    print("  c is IReadable = {c is IReadable}\n")

    var reader = c as IReadable
    print("  reader->read() = {reader->read()}\n")

    // ?as with null const pointer
    var nothing : Document? const
    var safe = nothing ?as IReadable
    print("  null const ?as IReadable = {safe}\n")

    // Pass const pointer to a function
    print_content(doc)
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_basic_interface()
    demo_is_as_operators()
    demo_multiple_interfaces()
    demo_polymorphic_dispatch()
    demo_interface_inheritance()
    demo_default_methods()
    demo_completeness_note()
    demo_const_interfaces()
    print("\ndone\n")
}

// expected output:
// === basic interface ===
//   getter:  Hey, Alice!
//   as:      Hey, Bob!
//
// === is / as / ?as ===
//   c is IDrawable  = true
//   c is IGreeter   = false
//     Circle(r=5) at (10,20)
//   null ?as IDrawable = null
//
// === multiple interfaces ===
//   is IDrawable     = true
//   is ISerializable = true
//   is IGreeter      = false
//     Sprite "hero" at (0,0)
//   serialize = sprite:hero
//
// === polymorphic dispatch ===
//     Circle(r=3) at (0,0)
//     Sprite "tree" at (0,0)
//
// === interface inheritance ===
//   d is IPet    = true
//   d is IAnimal = true
//   pet->name()  = Rex
//   pet->owner() = Alice
//   animal->name()  = Rex
//   animal->sound() = Woof
//
// === default methods ===
//   simple: [simple] hello
//   fancy:  *** [fancy] hello ***
//
// === completeness checking ===
//   (compile-time only — no demo output)
//   Try removing a method to see error 30111
//
// === const-only interfaces ===
//   c is IReadable = true
//   reader->read() = Hello from a const pointer!
//   null const ?as IReadable = null
//   content: Hello from a const pointer!
//
// done

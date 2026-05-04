options gen2

// Companion script for the "Operators and Properties" tutorial.
// The module "tutorial_09_cpp" exposes Vec3 with overloaded operators
// (+, -, *, ==, !=, unary -) and properties (length, lengthSq, isZero).

require tutorial_09_cpp

[export]
def test() {
    // --- Basic construction and field access ---
    print("=== Vec3 basics ===\n")
    let a = make_vec3(1.0, 2.0, 3.0)
    let b = make_vec3(4.0, 5.0, 6.0)
    print("a = ({a.x}, {a.y}, {a.z})\n")
    print("b = ({b.x}, {b.y}, {b.z})\n")

    // --- Properties (look like fields, call C++ methods) ---
    print("\n=== Properties ===\n")
    print("a.length   = {a.length}\n")
    print("a.lengthSq = {a.lengthSq}\n")
    print("a.isZero   = {a.isZero}\n")
    let zero = make_vec3(0.0, 0.0, 0.0)
    print("zero.isZero = {zero.isZero}\n")

    // --- Const/non-const property (addPropertyExtConst) ---
    // `editable` returns different values depending on mutability:
    //   - const (let) → calls `bool editable() const` → false
    //   - mutable (var) → calls `bool editable()` → true
    print("\n=== Const/non-const property ===\n")

    // Vec3 is POD — var works without unsafe
    print("Vec3 (POD):\n")
    let immutable_v = make_vec3(1.0, 2.0, 3.0)
    print("  let  v.editable = {immutable_v.editable}\n")   // false (const overload)
    var mutable_v = make_vec3(1.0, 2.0, 3.0)
    print("  var  v.editable = {mutable_v.editable}\n")     // true  (non-const overload)

    // Color has a non-trivial C++ constructor, so it is non-POD.
    // Without annotation overrides, local variables require `unsafe`:
    //   let c = Color()       // error[30108] without unsafe!
    // Two ways to work around this:

    // Way 1: `using` pattern — safe, no `unsafe` needed
    // using() $(var c : Color) { ... } constructs on the stack, passes to block
    print("Color (non-POD, using pattern):\n")
    using() $(var c : Color#) {
        c.r = 1.0
        c.g = 0.5
        c.b = 0.0
        print("  using: brightness = {c.brightness}\n")
    }

    // Way 2: `unsafe` block — allows local variables directly
    print("Color (non-POD, unsafe):\n")
    unsafe {
        let c3 = make_color(1.0, 0.5, 0.0, 1.0)
        print("  unsafe let: brightness = {c3.brightness}\n")
    }

    // SafeColor is the SAME C++ type, but its annotation overrides
    // isPod() and hasNonTrivialCtor() — so local variables work
    // without `unsafe` and without `using`.
    print("SafeColor (overridden annotation, no unsafe needed):\n")
    let sc = SafeColor()
    print("  let sc = SafeColor() : brightness = {sc.brightness}\n")
    var sc2 = SafeColor()
    sc2.r = 1.0
    sc2.g = 0.5
    print("  var sc2.brightness    = {sc2.brightness}\n")

    // --- Arithmetic operators ---
    print("\n=== Operators ===\n")
    let c = a + b
    print("a + b = ({c.x}, {c.y}, {c.z})\n")

    let d = b - a
    print("b - a = ({d.x}, {d.y}, {d.z})\n")

    let e = a * 2.0
    print("a * 2 = ({e.x}, {e.y}, {e.z})\n")

    let f = -a
    print("-a    = ({f.x}, {f.y}, {f.z})\n")

    // --- Equality operators ---
    print("\n=== Equality ===\n")
    let a2 = make_vec3(1.0, 2.0, 3.0)
    print("a == a2: {a == a2}\n")
    print("a != b:  {a != b}\n")
    print("a == b:  {a == b}\n")

    // --- Utility functions ---
    print("\n=== Utility functions ===\n")
    print("dot(a, b) = {dot(a, b)}\n")

    let cr = cross(a, b)
    print("cross(a, b) = ({cr.x}, {cr.y}, {cr.z})\n")

    let n = normalize(a)
    print("normalize(a) = ({n.x}, {n.y}, {n.z})\n")
    print("normalize(a).length = {n.length}\n")
}

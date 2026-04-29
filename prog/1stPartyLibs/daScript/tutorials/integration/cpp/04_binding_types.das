options gen2

// Companion script for the "Binding C++ Types" tutorial.
// The module "tutorial_04_cpp" exposes Vec2, Color, and Rect types
// along with helper functions from C++.
//
// NOTE: C++ types bound via ManagedStructureAnnotation are "handled"
// (reference) types.  Creating mutable locals of such types requires
// an `unsafe` block.  Immutable locals (`let`) returned from factory
// functions work without `unsafe`.

require tutorial_04_cpp

[export]
def test() {
    // --- Vec2: via factory function (immutable, no unsafe needed) ---
    let a = make_vec2(3.0, 4.0)
    print("a = ({a.x}, {a.y})\n")
    print("length(a) = {vec2_length(a)}\n")

    let b = make_vec2(1.0, 2.0)

    // --- Functions returning the bound type ---
    let c = vec2_add(a, b)
    print("a + b = ({c.x}, {c.y})\n")

    let scaled = vec2_scale(a, 2.0)
    print("a * 2 = ({scaled.x}, {scaled.y})\n")

    // --- Scalar return from bound types ---
    print("dot(a, b) = {vec2_dot(a, b)}\n")

    // --- Mutating function requires an unsafe mutable local ---
    unsafe {
        var d = make_vec2(3.0, 4.0)
        vec2_normalize(d)
        print("normalize(3,4) = ({d.x}, {d.y})\n")
    }

    // --- Color: integer fields via factory ---
    let col = make_color(255, 128, 0, 255)
    print("color = ({int(col.r)}, {int(col.g)}, {int(col.b)}, {int(col.a)})\n")

    // --- Rect: nested bound types ---
    let r = make_rect(10.0, 20.0, 100.0, 50.0)
    print("rect area = {rect_area(r)}\n")

    // --- Checking point containment ---
    let inside  = make_vec2(50.0, 30.0)
    let outside = make_vec2(200.0, 200.0)
    print("contains(50,30) = {rect_contains(r, inside)}\n")
    print("contains(200,200) = {rect_contains(r, outside)}\n")
}

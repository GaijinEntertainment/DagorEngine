// Tutorial 39: Dynamic Type Checking
//
// This tutorial covers:
//   - Runtime type checking for class hierarchies
//   - is_instance_of: test if pointer is an instance of a type
//   - dynamic_type_cast: safe downcast (returns null on failure)
//   - force_dynamic_type_cast: panic on failure
//   - ClassAsIs variant macro: `is ClassName` and `as ClassName` syntax
//   - ?as for safe optional downcasting
//   - Practical example: polymorphic processing
//
// requires: daslib/dynamic_cast_rtti
// Run: daslang.exe tutorials/language/39_dynamic_type_checking.das

options gen2
options rtti

require daslib/dynamic_cast_rtti

// === Class hierarchy ===

// We define a small shape hierarchy to demonstrate runtime type checks.

class Shape {
    name : string
    def abstract const area() : float
}

class Circle : Shape {
    radius : float
    def override const area() : float {
        return 3.14159265 * radius * radius
    }
}

class Rectangle : Shape {
    width : float
    height : float
    def override const area() : float {
        return width * height
    }
}

class Square : Rectangle {
    def Square(s : float) {
        width = s
        height = s
    }
}

// === is_instance_of ===

// is_instance_of checks whether a class pointer is an instance of a type
// at runtime, walking the RTTI chain.  Works with base and derived types.

def instance_of_demo() {
    print("\n=== is_instance_of ===\n")

    var c = new Circle(name = "circle", radius = 5.0)
    var r = new Rectangle(name = "rect", width = 3.0, height = 4.0)
    var sq = new Square(name = "square")
    sq.width = 2.0
    sq.height = 2.0

    let s_c : Shape? = c
    let s_r : Shape? = r
    let s_sq : Shape? = sq

    // Check exact type
    print("circle is Circle:    {is_instance_of(s_c, type<Circle>)}\n")
    print("circle is Rectangle: {is_instance_of(s_c, type<Rectangle>)}\n")

    // Check derived type
    print("square is Rectangle: {is_instance_of(s_sq, type<Rectangle>)}\n")
    print("square is Shape:     {is_instance_of(s_sq, type<Shape>)}\n")

    // Null is never an instance of anything
    var np : Shape? = null
    print("null is Shape:       {is_instance_of(np, type<Shape>)}\n")

    unsafe {
        delete c
        delete r
        delete sq
    }
}

// === dynamic_type_cast ===

// dynamic_type_cast returns a pointer to the target type if the cast
// succeeds, or null if it fails.  This is the "safe" downcast.

// With `def const` on area(), we can call it through const pointers.
def describe_shape(s : Shape?) {
    let circle = dynamic_type_cast(s, type<Circle>)
    if (circle != null) {
        print("  Circle: radius={circle.radius}, area={s->area()}\n")
        return
    }
    let rect = dynamic_type_cast(s, type<Rectangle>)
    if (rect != null) {
        print("  Rectangle: {rect.width}x{rect.height}, area={s->area()}\n")
        return
    }
    print("  Unknown shape: area={s->area()}\n")
}

def dynamic_cast_demo() {
    print("\n=== dynamic_type_cast ===\n")

    var c2 = new Circle(name = "c", radius = 3.0)
    var r2 = new Rectangle(name = "r", width = 5.0, height = 2.0)

    describe_shape(c2)
    describe_shape(r2)

    // Failed cast returns null
    let sh : Shape? = c2
    let result = dynamic_type_cast(sh, type<Rectangle>)
    if (result == null) {
        print("  circle as Rectangle: null\n")
    } else {
        print("  circle as Rectangle: ok\n")
    }

    unsafe {
        delete c2
        delete r2
    }
}

// === force_dynamic_type_cast ===

// force_dynamic_type_cast panics if the cast fails.  Use when you are
// certain of the type and want a clear error if your assumption is wrong.

def force_cast_demo() {
    print("\n=== force_dynamic_type_cast ===\n")

    var c = new Circle(name = "c", radius = 7.0)
    let s : Shape? = c

    // This succeeds:
    let fc = force_dynamic_type_cast(s, type<Circle>)
    print("  forced cast to Circle: radius={fc.radius}\n")

    // This would panic:
    // let fr = force_dynamic_type_cast(s, type<Rectangle>)

    unsafe {
        delete c
    }
}

// === ClassAsIs variant macro ===

// After requiring daslib/dynamic_cast_rtti, you get syntactic sugar:
//   ptr is ClassName    → is_instance_of(ptr, type<ClassName>)
//   ptr as ClassName    → force_dynamic_type_cast(ptr, type<ClassName>)
//   ptr ?as ClassName   → dynamic_type_cast(ptr, type<ClassName>)

def is_as_syntax_demo() {
    print("\n=== is / as / ?as syntax ===\n")

    var c = new Circle(name = "c", radius = 4.0)
    var r = new Rectangle(name = "r", width = 6.0, height = 3.0)
    let s_c : Shape? = c
    let s_r : Shape? = r

    // `is` checks type at runtime
    print("s_c is Circle: {s_c is Circle}\n")
    print("s_c is Rectangle: {s_c is Rectangle}\n")
    print("s_r is Rectangle: {s_r is Rectangle}\n")

    // `as` performs force cast (panics on failure)
    let circle = s_c as Circle
    print("as Circle: radius={circle.radius}\n")

    // `?as` performs safe cast (returns null on failure)
    let maybe_rect = s_c ?as Rectangle
    if (maybe_rect == null) {
        print("?as Rectangle: null (correct)\n")
    } else {
        print("?as Rectangle: unexpected\n")
    }

    let rect = s_r ?as Rectangle
    if (rect != null) {
        print("?as Rectangle: {rect.width}x{rect.height}\n")
    } else {
        print("?as Rectangle: null\n")
    }

    unsafe {
        delete c
        delete r
    }
}

// === Practical: polymorphic processing ===

def process_shapes() {
    print("\n=== polymorphic processing ===\n")

    var shapes : array<Shape?>
    shapes |> push(new Circle(name = "sun", radius = 10.0))
    shapes |> push(new Rectangle(name = "wall", width = 8.0, height = 3.0))
    shapes |> push(new Circle(name = "ball", radius = 2.5))
    var sq = new Square(name = "tile")
    sq.width = 4.0
    sq.height = 4.0
    shapes |> push(sq)

    var total_area = 0.0
    for (s in shapes) {
        if (s is Circle) {
            let c = s as Circle
            print("  {c.name}: circle r={c.radius}\n")
        } elif (s is Square) {
            // Square check must come before Rectangle (Square extends Rectangle)
            let sq2 = s as Square
            print("  {sq2.name}: square side={sq2.width}\n")
        } elif (s is Rectangle) {
            let r = s as Rectangle
            print("  {r.name}: rect {r.width}x{r.height}\n")
        }
        total_area += s->area()
    }
    print("  total area: {total_area}\n")

    for (s in shapes) {
        unsafe {
            delete s
        }
    }
}


// === Summary ===
//
// +-------------------------------+-------------------------------------------+
// | Function / Syntax             | Description                               |
// +-------------------------------+-------------------------------------------+
// | is_instance_of(ptr, type<T>)  | true if ptr is instance of T via RTTI     |
// | dynamic_type_cast(ptr, T)     | Returns T? or null on failure             |
// | force_dynamic_type_cast(ptr,T)| Returns T? or panics on failure           |
// | ptr is ClassName              | Syntactic sugar for is_instance_of        |
// | ptr as ClassName              | Syntactic sugar for force cast            |
// | ptr ?as ClassName             | Syntactic sugar for safe cast             |
// +-------------------------------+-------------------------------------------+
//
// Note: requires `options rtti` at the top of the file.

[export]
def main() {
    instance_of_demo()
    dynamic_cast_demo()
    force_cast_demo()
    is_as_syntax_demo()
    process_shapes()
    print("done\n")
}

// output:
//
// === is_instance_of ===
// circle is Circle:    true
// circle is Rectangle: false
// square is Rectangle: true
// square is Shape:     true
// null is Shape:       false
//
// === dynamic_type_cast ===
//   Circle: radius=3, area=28.274334
//   Rectangle: 5x2, area=10
//   circle as Rectangle: null
//
// === force_dynamic_type_cast ===
//   forced cast to Circle: radius=7
//
// === is / as / ?as syntax ===
// s_c is Circle: true
// s_c is Rectangle: false
// s_r is Rectangle: true
// as Circle: radius=4
// ?as Rectangle: null (correct)
// ?as Rectangle: 6x3
//
// === polymorphic processing ===
//   sun: circle r=10
//   wall: rect 8x3
//   ball: circle r=2.5
//   tile: square side=4
//   total area: 373.79422
// done

// Tutorial 8: Structs
//
// This tutorial covers:
//   - Declaring structs
//   - Field types and default values
//   - Construction with gen2 syntax: Foo(x=1, y=2)
//   - Nested structs
//   - Heap allocation with new
//   - The 'with' block for field access
//   - Structs are pure data (no methods)
//
// Run: daslang.exe tutorials/language/08_structs.das

options gen2

// === Declaring a struct ===
// Structs define a named collection of fields.
// Fields can have default values.

struct Point {
    x : float = 0.0
    y : float = 0.0
}

struct Color {
    r : uint8
    g : uint8
    b : uint8
    a : uint8 = uint8(255)
}

// Fields can use type inference when given a default
struct Config {
    width : int = 800
    height : int = 600
    title : string = "My App"
    fullscreen : bool = false
}

// === Nested structs ===
struct Rect {
    topLeft : Point = Point()
    bottomRight : Point = Point()
}

// === Functions that work with structs ===
// Structs are pure data — they have no methods.
// Write free functions that take them as parameters.

def area(r : Rect) : float {
    let w = r.bottomRight.x - r.topLeft.x
    let h = r.bottomRight.y - r.topLeft.y
    return w * h
}

def describe(p : Point) : string {
    return "({p.x}, {p.y})"
}

// Pass by mutable reference to modify
def translate(var p : Point&; dx, dy : float) {
    p.x += dx
    p.y += dy
}

[export]
def main {

    // === Construction ===

    // Default initialization — creates with field defaults
    var p1 = Point()     // x=0.0, y=0.0
    print("p1 = {describe(p1)}\n")

    // Named field construction
    var p2 = Point(x = 3.0, y = 4.0)
    print("p2 = {describe(p2)}\n")

    // Partial initialization — unspecified fields get their defaults
    var cfg = Config(title = "Tutorial")  // width=800, height=600, fullscreen=false
    print("config: {cfg.width}x{cfg.height} \"{cfg.title}\" fullscreen={cfg.fullscreen}\n")

    // === Field access and modification ===
    var player = Point(x = 10.0, y = 20.0)
    print("player = {describe(player)}\n")
    translate(player, 5.0, -3.0)
    print("after translate = {describe(player)}\n")

    // === Nested structs ===
    var rect = Rect(
        topLeft = Point(x = 0.0, y = 0.0),
        bottomRight = Point(x = 10.0, y = 5.0)
    )
    print("rect area = {area(rect)}\n")

    // === The 'with' block ===
    // Inside a 'with' block, struct fields can be accessed without the variable name.

    var hero = Point()
    with (hero) {
        x = 100.0
        y = 200.0
    }
    print("hero = {describe(hero)}\n")

    // 'with' is especially useful for structs with many fields
    var settings = Config()
    with (settings) {
        width = 1920
        height = 1080
        title = "Game"
        fullscreen = true
    }
    print("settings: {settings.width}x{settings.height} \"{settings.title}\" fullscreen={settings.fullscreen}\n")

    // === Heap allocation with new ===
    // new creates a pointer to a heap-allocated struct.
    // Useful when you need the struct to outlive the current scope.
    var p3 = new Point(x = 7.0, y = 8.0)
    // p3 is of type Point? (pointer to Point)
    print("p3 = ({p3.x}, {p3.y})\n")

    // p3 will be garbage collected when no longer referenced.
    // For explicit cleanup, see the dedicated tutorial on delete and inscope.
}

// output:
// p1 = (0, 0)
// p2 = (3, 4)
// config: 800x600 "Tutorial" fullscreen=false
// player = (10, 20)
// after translate = (15, 17)
// rect area = 50
// hero = (100, 200)
// settings: 1920x1080 "Game" fullscreen=true
// p3 = (7, 8)

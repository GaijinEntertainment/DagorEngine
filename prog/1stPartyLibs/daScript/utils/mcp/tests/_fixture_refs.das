options gen2

enum Color {
    Red
    Green
    Blue
}

struct Point {
    x : float
    y : float
}

def add_points(a, b : Point) : Point {
    return Point(x = a.x + b.x, y = a.y + b.y)
}

def use_point(p : Point?) : float {
    return p?.x ?? 0.0
}

def apply(f : function<(a : Point; b : Point) : Point>; a, b : Point) : Point {
    return invoke(f, a, b)
}

bitfield Flags : uint {
    readable
    writable
}

variant Value {
    i : int
    f : float
}

let GLOBAL_SCALE = 2.0

def use_flags(f : Flags) : bool {
    return (f & Flags.readable) != bitfield(0u)
}

class Shape {
    def area() : float => 0.0
}

class Square : Shape {
    def override area() : float => 4.0
    def finalize() {
        print("square\n")
    }
}

[export]
def main() {
    let p1 = Point(x = 1.0, y = 2.0)
    let p2 = Point(x = 3.0, y = 4.0)
    let p3 = add_points(p1, p2)
    let safe_x = use_point(new Point(x = 5.0))
    let via_invoke = apply(@@add_points, p1, p2)
    let c = Color.Red
    let fl = Flags.readable
    let v = Value(i = 10)
    let has_r = use_flags(fl)
    let scaled = p3.x * GLOBAL_SCALE
    print("{p3.x} {p3.y} {safe_x} {via_invoke.x} {c} {fl} {v} {has_r} {scaled}\n")
    var shape = new Square()
    print("{shape->area()}\n")
    for (i in range(2)) {
        print("{i}\n")
    }
}

def paint(c : Color) : int {
    return int(c)
}

def area_of(var s : Square?) : float {
    return s->area()
}

struct Other {
    x : int
}

def other_x(o : Other) : int {
    return o.x
}

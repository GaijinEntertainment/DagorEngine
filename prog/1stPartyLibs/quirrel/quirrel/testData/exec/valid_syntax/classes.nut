// Valid syntax: Classes and inheritance

// Basic class
class Point {
    x = 0
    y = 0
}

let p = Point()
assert(p.x == 0)
assert(p.y == 0)
p.x = 10
p.y = 20
assert(p.x == 10)

// Class with constructor
class Point2 {
    x = 0
    y = 0
    constructor(x, y) {
        this.x = x
        this.y = y
    }
}

let p2 = Point2(3, 4)
assert(p2.x == 3)
assert(p2.y == 4)

// Class with methods
class Circle {
    r = 0
    constructor(r) { this.r = r }
    function area() {
        return 3.14159 * this.r * this.r
    }
    function perimeter() {
        return 2.0 * 3.14159 * this.r
    }
}

let c = Circle(5)
assert(c.r == 5)

// Class with static members
class Config {
    static VERSION = "1.0"
    static COUNT = 0
}
assert(Config.VERSION == "1.0")

// Inheritance using parenthesis syntax
class Shape {
    function area() { return 0 }
    function name() { return "shape" }
}

class Rect(Shape) {
    w = 0
    h = 0
    constructor(w, h) {
        this.w = w
        this.h = h
    }
    function area() {
        return this.w * this.h
    }
    function name() { return "rect" }
}

let r = Rect(3, 4)
assert(r.area() == 12)
assert(r.name() == "rect")
assert(r instanceof Rect)
assert(r instanceof Shape)

// Inheritance with base call
class Base {
    value = 0
    constructor(v = 0) { this.value = v }
    function get() { return this.value }
}

class Derived(Base) {
    extra = 0
    constructor(v, e) {
        base.constructor(v)
        this.extra = e
    }
    function get() {
        return base.get() + this.extra
    }
}

let d = Derived(10, 5)
assert(d.get() == 15)
assert(d.value == 10)
assert(d.extra == 5)

// getclass and getbase
assert(d.getclass() == Derived)
assert(Derived.getbase() == Base)

// Class as expression
let MyClass = class {
    value = 42
    function get() { return this.value }
}
let mc = MyClass()
assert(mc.get() == 42)

// Class with metamethods - use getclass() for self-reference
class Vector {
    x = 0
    y = 0
    constructor(x, y) {
        this.x = x
        this.y = y
    }
    function _add(other) {
        return this.getclass()(this.x + other.x, this.y + other.y)
    }
    function _sub(other) {
        return this.getclass()(this.x - other.x, this.y - other.y)
    }
    function _cmp(other) {
        let d1 = this.x * this.x + this.y * this.y
        let d2 = other.x * other.x + other.y * other.y
        if (d1 < d2) return -1
        if (d1 > d2) return 1
        return 0
    }
    function _tostring() {
        return $"Vector({this.x}, {this.y})"
    }
    function _typeof() {
        return "Vector"
    }
}

let v1 = Vector(1, 2)
let v2 = Vector(3, 4)
let v3 = v1 + v2
assert(v3.x == 4)
assert(v3.y == 6)

let v4 = v2 - v1
assert(v4.x == 2)
assert(v4.y == 2)

assert(typeof v1 == "Vector")
assert(v1 < v2)
assert(!(v2 < v1))

// Class modification before instantiation
class Modifiable {
    x = 1
}
Modifiable.y <- 2
let mod_inst = Modifiable()
assert(mod_inst.x == 1)
assert(mod_inst.y == 2)

// Constructor with default params
class WithDefaults {
    x = 0
    y = 0
    constructor(x = 10, y = 20) {
        this.x = x
        this.y = y
    }
}
let wd1 = WithDefaults()
assert(wd1.x == 10)
assert(wd1.y == 20)
let wd2 = WithDefaults(1, 2)
assert(wd2.x == 1)
assert(wd2.y == 2)

// Multiple inheritance levels
class A { function who() { return "A" } }
class B(A) { function who() { return "B" } }
class C(B) { function who() { return "C" } }

let obj_c = C()
assert(obj_c.who() == "C")
assert(obj_c instanceof A)
assert(obj_c instanceof B)
assert(obj_c instanceof C)

// rawin/rawget/rawset on instances
let inst = Point2(1, 2)
assert(inst.rawin("x"))
assert(inst.rawget("x") == 1)
inst.rawset("x", 99)
assert(inst.x == 99)

print("classes: OK\n")

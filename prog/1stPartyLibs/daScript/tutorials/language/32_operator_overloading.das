// Tutorial 32: Operator Overloading
//
// This tutorial covers:
//   - Overloading arithmetic operators (+, -, *, /)
//   - Overloading comparison operators (==, !=, <, >, <=, >=)
//   - Overloading unary operators (!, -, ~, ++, --)
//   - Overloading compound assignment operators (+=, -=, etc.)
//   - Overloading the [] index operator (read, write, compound)
//   - Overloading the . and ?. dot operators
//   - Property accessors with operator . name and operator . name :=
//   - Overloading clone (:=) and finalize (delete)
//   - Struct method operators vs free-function operators
//   - Generic operator overloading
//
// requires: nothing beyond the language core
// Run: daslang.exe tutorials/language/32_operator_overloading.das

options gen2
options persistent_heap

require math

// === Arithmetic Operators ===
// Define custom +, -, * and / for your own types.
// Operators are regular functions named "operator <op>".

struct Vec2 {
    x : float
    y : float
}

def operator +(a, b : Vec2) : Vec2 {
    return Vec2(x = a.x + b.x, y = a.y + b.y)
}

def operator -(a, b : Vec2) : Vec2 {
    return Vec2(x = a.x - b.x, y = a.y - b.y)
}

def operator *(a : Vec2; s : float) : Vec2 {
    return Vec2(x = a.x * s, y = a.y * s)
}

def operator *(s : float; a : Vec2) : Vec2 {
    return a * s
}

def operator /(a : Vec2; s : float) : Vec2 {
    return Vec2(x = a.x / s, y = a.y / s)
}

def arithmetic_operators() {
    print("=== Arithmetic operators ===\n")

    let a = Vec2(x = 1.0, y = 2.0)
    let b = Vec2(x = 3.0, y = 4.0)

    let sum = a + b
    print("a + b = ({sum.x}, {sum.y})\n")
    // output: a + b = (4, 6)

    let diff = a - b
    print("a - b = ({diff.x}, {diff.y})\n")
    // output: a - b = (-2, -2)

    let scaled = a * 3.0
    print("a * 3 = ({scaled.x}, {scaled.y})\n")
    // output: a * 3 = (3, 6)

    let halved = b / 2.0
    print("b / 2 = ({halved.x}, {halved.y})\n")
    // output: b / 2 = (1.5, 2)
}

// === Comparison Operators ===
// Overload ==, !=, <, >, <=, >= for custom ordering.

def operator ==(a, b : Vec2) : bool {
    return a.x == b.x && a.y == b.y
}

def operator !=(a, b : Vec2) : bool {
    return !(a == b)
}

// Order by magnitude (length squared, to avoid sqrt)
def operator <(a, b : Vec2) : bool {
    return (a.x * a.x + a.y * a.y) < (b.x * b.x + b.y * b.y)
}

def comparison_operators() {
    print("\n=== Comparison operators ===\n")

    let a = Vec2(x = 1.0, y = 2.0)
    let b = Vec2(x = 1.0, y = 2.0)
    let c = Vec2(x = 3.0, y = 4.0)

    print("a == b: {a == b}\n")
    // output: a == b: true
    print("a != c: {a != c}\n")
    // output: a != c: true
    print("a < c: {a < c}\n")
    // output: a < c: true
}

// === Unary Operators ===
// Overload unary -, !, ~, prefix ++/-- and postfix ++/--.

def operator -(a : Vec2) : Vec2 {
    return Vec2(x = -a.x, y = -a.y)
}

// A simple counter to demonstrate increment/decrement.
struct Counter {
    value : int
}

// prefix ++: called before the value is used
def operator ++(var c : Counter) : Counter {
    c.value += 1
    return c
}

// postfix ++: called after the value is used (note: "operator ++" for prefix, "operator ++" for postfix)
// In daslang, prefix is "++operator" and postfix is "operator ++".
// Since postfix returns a copy, the syntax is:
//   def operator ++(var c : Counter; postfix : int) — not supported this way in daslang
// Actually in daslang:
//   ++operator → prefix increment (modifies and returns)
//   operator ++ → postfix increment
// Both take a single mutable argument.

def unary_operators() {
    print("\n=== Unary operators ===\n")

    let a = Vec2(x = 1.0, y = 2.0)
    let neg = -a
    print("-a = ({neg.x}, {neg.y})\n")
    // output: -a = (-1, -2)
}

// === Compound Assignment ===
// Overload +=, -=, *=, /= for in-place modification.

def operator +=(var a : Vec2&; b : Vec2) {
    a.x += b.x
    a.y += b.y
}

def operator -=(var a : Vec2&; b : Vec2) {
    a.x -= b.x
    a.y -= b.y
}

def operator *=(var a : Vec2&; s : float) {
    a.x *= s
    a.y *= s
}

def compound_assignment() {
    print("\n=== Compound assignment ===\n")

    var a = Vec2(x = 1.0, y = 2.0)
    let b = Vec2(x = 0.5, y = 0.5)

    a += b
    print("a += b: ({a.x}, {a.y})\n")
    // output: a += b: (1.5, 2.5)

    a -= b
    print("a -= b: ({a.x}, {a.y})\n")
    // output: a -= b: (1, 2)

    a *= 10.0
    print("a *= 10: ({a.x}, {a.y})\n")
    // output: a *= 10: (10, 20)
}

// === Index Operators ===
// operator []     — read access
// operator [] =   — write access
// operator [] +=  — compound index assignment
// These can be free functions or struct methods.

struct Matrix2x2 {
    data : float[4]
}

// Free-function index operators: read and write by linear index.
def operator [] (m : Matrix2x2; i : int) : float {
    return m.data[i]
}

def operator [] =(var m : Matrix2x2&; i : int; v : float) {
    m.data[i] = v
}

def operator [] +=(var m : Matrix2x2&; i : int; v : float) {
    m.data[i] += v
}

def index_operators() {
    print("\n=== Index operators ===\n")

    var m : Matrix2x2
    m.data[0] = 1.0
    m.data[1] = 2.0
    m.data[2] = 3.0
    m.data[3] = 4.0
    print("m[0] = {m[0]}\n")
    // output: m[0] = 1
    print("m[3] = {m[3]}\n")
    // output: m[3] = 4

    m[0] = 10.0
    print("after m[0]=10: {m[0]}\n")
    // output: after m[0]=10: 10

    m[1] += 5.0
    print("after m[1]+=5: {m[1]}\n")
    // output: after m[1]+=5: 7
}

// === Dot Operator ===
// operator .          — generic field access (takes field name as string)
// operator . name     — named property getter
// operator . name :=  — named property setter
// "." works on structs, ". ." bypasses overloads to access real fields.

struct Particle {
    pos_x : float
    pos_y : float
}

// Named property: "speed" (computed from pos_x and pos_y as magnitude)
def operator . speed(p : Particle) : float {
    return sqrt(p.pos_x * p.pos_x + p.pos_y * p.pos_y)
}

// Named property setter
def operator . speed :=(var p : Particle&; value : float) {
    let mag = sqrt(p.pos_x * p.pos_x + p.pos_y * p.pos_y)
    if (mag > 0.0) {
        let scale = value / mag
        p.pos_x *= scale
        p.pos_y *= scale
    }
}

def dot_operators() {
    print("\n=== Dot operators ===\n")

    var p = Particle(pos_x = 3.0, pos_y = 4.0)
    print("speed = {p.speed}\n")
    // output: speed = 5

    // Set speed to 10 (scales direction)
    p.speed := 10.0
    print("after speed := 10: ({p.pos_x}, {p.pos_y})\n")
    // output: after speed := 10: (6, 8)

    // ". ." bypasses the overload to access real fields
    print("real pos_x = {p . . pos_x}\n")
    // output: real pos_x = 6
}

// === Clone and Finalize ===
// operator :=    — custom clone behavior
// operator delete (or finalize) — custom cleanup

struct Resource {
    name : string
    refcount : int
}

// Custom clone: increment reference count
def operator :=(var dst : Resource&; src : Resource) {
    dst.name = src.name
    dst.refcount = src.refcount + 1
}

def clone_and_finalize() {
    print("\n=== Clone and finalize ===\n")

    var orig = Resource(name = "texture", refcount = 1)
    var copy : Resource
    copy := orig
    print("orig refcount: {orig.refcount}\n")
    // output: orig refcount: 1
    print("copy refcount: {copy.refcount}\n")
    // output: copy refcount: 2
}

// === Struct Method Operators ===
// Operators can be defined as struct methods instead of free functions.
// This is useful for encapsulation — the operator lives with the type.

struct Stack {
    items : array<int>

    // Read: peek at index from bottom
    def const operator [] (index : int) : int {
        return items[index]
    }

    // Write: set value at index
    def operator [] =(index : int; value : int) {
        items[index] = value
    }
}

def push(var s : Stack; value : int) {
    s.items |> push(value)
}

def struct_method_operators() {
    print("\n=== Struct method operators ===\n")

    var s : Stack
    s |> push(10)
    s |> push(20)
    s |> push(30)

    print("s[0] = {s[0]}\n")
    // output: s[0] = 10
    print("s[2] = {s[2]}\n")
    // output: s[2] = 30

    s[1] = 99
    print("after s[1]=99: {s[1]}\n")
    // output: after s[1]=99: 99
}

// === Generic Operators ===
// Operators can be generic using `auto` types with contracts.
// This lets one definition cover multiple types.

struct Vec3 {
    x : float
    y : float
    z : float
}

// A generic dot-product that works with any type that has x and y fields.
// (Both Vec2 and Vec3 qualify, though for Vec3 only x and y are used.)
def dot_2d(a, b : auto) : float {
    return a.x * b.x + a.y * b.y
}

def generic_operators() {
    print("\n=== Generic operators ===\n")

    let v2a = Vec2(x = 1.0, y = 0.0)
    let v2b = Vec2(x = 0.0, y = 1.0)
    print("Vec2 dot: {dot_2d(v2a, v2b)}\n")
    // output: Vec2 dot: 0

    let v3a = Vec3(x = 3.0, y = 4.0, z = 0.0)
    let v3b = Vec3(x = 1.0, y = 0.0, z = 99.0)
    print("Vec3 dot(2d): {dot_2d(v3a, v3b)}\n")
    // output: Vec3 dot(2d): 3
}

// === Complete Operator List ===
// daslang supports overloading the following operators:
//
// Arithmetic:     +  -  *  /  %
// Comparison:     ==  !=  <  >  <=  >=
// Bitwise:        &  |  ^  ~  <<  >>  <<<  >>>
// Logical:        &&  ||  ^^  !
// Unary:          - (negate)  ~ (complement)  ++ (prefix/postfix)  -- (prefix/postfix)
// Assignment:     +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=  <<<=  >>>=  &&=  ||=  ^^=
// Index:          []  []=  []<-  []:=  []+=  []-=  []*=  etc.
// Safe index:     ?[]
// Dot:            .  ?.  . name  . name :=  . name +=  etc.
// Type:           :=  (clone)  delete  (finalize)  is  as  ?as
// Null-coalesce:  ??
// Interval:       ..

[export]
def main() {
    arithmetic_operators()
    comparison_operators()
    unary_operators()
    compound_assignment()
    index_operators()
    dot_operators()
    clone_and_finalize()
    struct_method_operators()
    generic_operators()
}

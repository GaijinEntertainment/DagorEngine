options gen2

// Tutorial 12 — Typeinfo macros
//
// Demonstrates AstTypeInfoMacro (registered via [typeinfo_macro]).
// A typeinfo macro extends the built-in typeinfo(...) expression with
// custom compile-time type introspection.  The macro's getAstChange
// method receives an ExprTypeInfo node and returns a replacement AST
// expression — typically a constant (string, int, bool, array) that
// the compiler folds into the program.
//
// The companion module (typeinfo_macro_mod) defines:
//   struct_info            — returns a string describing a struct
//   enum_value_strings     — returns an array<string> of enum names
//   has_non_static_method  — returns bool via subtrait parameter
//
// Section 1 shows struct_info: compile-time struct description.
// Section 2 shows enum_value_strings: compile-time enum names array.
// Section 3 shows has_non_static_method: compile-time method check.

require typeinfo_macro_mod

// ---- Section 1: struct_info -------------------------------------------------

struct Vec3 {
    x : float
    y : float
    z : float
}

struct Person {
    name : string
    age : int
}

def section1() {
    print("--- Section 1: struct_info ---\n")
    // typeinfo struct_info(type<T>) is resolved entirely at compile time.
    // The macro builds a string like "Name(field:type, ...)" from the
    // struct's field declarations.
    let v = typeinfo struct_info(type<Vec3>)
    print("  Vec3: {v}\n")
    let p = typeinfo struct_info(type<Person>)
    print("  Person: {p}\n")
}

// ---- Section 2: enum_value_strings ------------------------------------------

enum Color {
    Red
    Green
    Blue
}

enum Direction {
    North
    South
    East
    West
}

def section2() {
    print("--- Section 2: enum_value_strings ---\n")
    // typeinfo enum_value_strings(type<E>) returns an array of strings
    // with all enum value names, built at compile time.
    let colors = typeinfo enum_value_strings(type<Color>)
    print("  Color values ({length(colors)}):\n")
    for (c in colors) {
        print("    {c}\n")
    }
    let dirs = typeinfo enum_value_strings(type<Direction>)
    print("  Direction values ({length(dirs)}):\n")
    for (d in dirs) {
        print("    {d}\n")
    }
}

// ---- Section 3: has_non_static_method ---------------------------------------

class Animal {
    name : string
    def speak() {
        print("    {name} says hello\n")
    }
}

class Rock {
    weight : float
}

def section3() {
    print("--- Section 3: has_non_static_method ---\n")
    // typeinfo has_non_static_method<name>(type<T>) uses the subtrait
    // (the <name> parameter) to check whether a class has a non-static
    // method with that name.
    let animal_can_speak = typeinfo has_non_static_method < speak > (type<Animal>)
    print("  Animal has 'speak': {animal_can_speak}\n")
    let animal_can_fly = typeinfo has_non_static_method < fly > (type<Animal>)
    print("  Animal has 'fly': {animal_can_fly}\n")
    let rock_can_speak = typeinfo has_non_static_method < speak > (type<Rock>)
    print("  Rock has 'speak': {rock_can_speak}\n")
    // Compile-time boolean — usable in static_if for conditional code.
    static_if (typeinfo has_non_static_method < speak > (type<Animal>)) {
        print("  (static_if confirmed: Animal can speak)\n")
    }
}

// ---- expected output --------------------------------------------------------
// --- Section 1: struct_info ---
//   Vec3: Vec3(x:float, y:float, z:float)
//   Person: Person(name:string, age:int)
// --- Section 2: enum_value_strings ---
//   Color values (3):
//     Red
//     Green
//     Blue
//   Direction values (4):
//     North
//     South
//     East
//     West
// --- Section 3: has_non_static_method ---
//   Animal has 'speak': true
//   Animal has 'fly': false
//   Rock has 'speak': false
//   (static_if confirmed: Animal can speak)

[export]
def main() {
    section1()
    section2()
    section3()
}

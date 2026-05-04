// Tutorial 48: Compile-Time Field Iteration with apply
//
// This tutorial covers:
//   - Using apply to iterate struct fields at compile time
//   - Compile-time dispatch with static_if on field names
//   - Mutating fields through apply
//   - Iterating tuple fields (unnamed and named)
//   - Iterating variant alternatives
//   - Building a generic describe function
//   - Reading field annotations with the 3-argument form
//
// Prerequisites: None (basic daScript knowledge only)
//
// Key concepts:
//   - apply(value) $(name, field) { ... } iterates all fields of a struct, tuple, or variant
//   - name is a compile-time string constant — static_if can branch on it
//   - apply generates specialized code per field — no runtime reflection overhead
//   - The 3-arg form $(name, field, annotations) gives access to per-field metadata
//   - apply works on structs, tuples (named and unnamed), and variants
//
// Run: daslang.exe tutorials/language/48_apply.das

options gen2
options rtti

require daslib/rtti
require daslib/apply
require daslib/strings_boost


// ============================================================
// Section 1: Basic struct iteration
// ============================================================
//
// apply(value) $(name, field) { ... } visits every field of a struct.
// `name` is the field name (string constant), `field` is the value.
// The block is generated once per field at compile time, so every
// field gets its own type-correct code path.

struct Hero {
    name   : string
    health : int
    speed  : float
}

def demo_basic_struct() {
    print("\n=== Section 1: Basic struct iteration ===\n")

    let hero = Hero(name = "Archer", health = 100, speed = 3.5)

    // apply iterates every field — no reflection, pure compile-time expansion
    apply(hero) $(name, field) {
        print("  {name} = {field}\n")
    }
    // output:
    //   name = Archer
    //   health = 100
    //   speed = 3.5
}


// ============================================================
// Section 2: Compile-time dispatch with static_if
// ============================================================
//
// Because `name` is known at compile time, you can branch on it
// with static_if.  Only the matching branch is compiled for each
// field — the others are discarded entirely.

struct Config {
    width     : int
    height    : int
    title     : string
    fullscreen : bool
}

def demo_static_if() {
    print("\n=== Section 2: Compile-time dispatch with static_if ===\n")

    let cfg = Config(width = 1920, height = 1080, title = "My Game", fullscreen = true)

    apply(cfg) $(name, field) {
        static_if (name == "title") {
            print("  Title (special handling): \"{field}\"\n")
        } else {
            print("  {name} = {field}\n")
        }
    }
    // output:
    //   width = 1920
    //   height = 1080
    //   Title (special handling): "My Game"
    //   fullscreen = true
}


// ============================================================
// Section 3: Mutating fields
// ============================================================
//
// If you pass a mutable variable (var), apply gives you mutable
// references to each field, allowing in-place modification.

struct Stats {
    attack  : int
    defense : int
    magic   : int
}

def demo_mutation() {
    print("\n=== Section 3: Mutating fields ===\n")

    var stats = Stats(attack = 10, defense = 5, magic = 8)
    print("  Before: attack={stats.attack}, defense={stats.defense}, magic={stats.magic}\n")

    // Double every integer field
    apply(stats) $(name, field) {
        field *= 2
    }

    print("  After:  attack={stats.attack}, defense={stats.defense}, magic={stats.magic}\n")
    // output:
    //   Before: attack=10, defense=5, magic=8
    //   After:  attack=20, defense=10, magic=16
}


// ============================================================
// Section 4: Tuples — unnamed and named
// ============================================================
//
// apply also works on tuples.  For unnamed tuples the field names
// are "_0", "_1", etc.  Named tuples use their declared names.

def demo_tuples() {
    print("\n=== Section 4: Tuples ===\n")

    // Unnamed tuple
    let pair : tuple<int; string> = (42, "hello")
    print("  Unnamed tuple:\n")
    apply(pair) $(name, field) {
        print("    {name} = {field}\n")
    }
    // output:
    //   Unnamed tuple:
    //     _0 = 42
    //     _1 = hello

    // Named tuple
    let point : tuple<x : float; y : float> = (1.0, 2.0)
    print("  Named tuple:\n")
    apply(point) $(name, field) {
        print("    {name} = {field}\n")
    }
    // output:
    //   Named tuple:
    //     x = 1
    //     y = 2
}


// ============================================================
// Section 5: Variants
// ============================================================
//
// For variants, apply visits only the currently active alternative.
// The block fires for the one alternative that is set.

variant Shape {
    circle   : float     // radius
    rect     : float2    // width, height
    triangle : float3    // three side lengths
}

def demo_variants() {
    print("\n=== Section 5: Variants ===\n")

    var shapes : array<Shape>
    shapes |> emplace(Shape(circle = 5.0))
    shapes |> emplace(Shape(rect = float2(3.0, 4.0)))
    shapes |> emplace(Shape(triangle = float3(3.0, 4.0, 5.0)))

    for (s in shapes) {
        apply(s) $(name, field) {
            print("  Shape is {name}: {field}\n")
        }
    }
    // output:
    //   Shape is circle: 5
    //   Shape is rect: 3,4
    //   Shape is triangle: 3,4,5
}


// ============================================================
// Section 6: Practical example — generic describe
// ============================================================
//
// apply is perfect for building generic utilities that work on
// any struct without knowing its fields in advance.

struct Weapon {
    name   : string
    damage : int
    weight : float
}

struct Potion {
    name   : string
    effect : string
    uses   : int
}

def describe(value) {
    //! Print a human-readable one-line description of any struct.
    var first = true
    print("\{")
    apply(value) $(name, field) {
        if (!first) {
            print(", ")
        }
        first = false
        // Use static_if on the type to add quotes around strings
        static_if (typeinfo stripped_typename(field) == "string") {
            print("{name}=\"{field}\"")
        } else {
            print("{name}={field}")
        }
    }
    print("\}")
}

def demo_describe() {
    print("\n=== Section 6: Generic describe ===\n")

    let sword = Weapon(name = "Excalibur", damage = 50, weight = 3.2)
    let potion = Potion(name = "Heal", effect = "restore_hp", uses = 3)
    let hero = Hero(name = "Knight", health = 200, speed = 2.0)

    print("  ")
    describe(sword)
    print("\n  ")
    describe(potion)
    print("\n  ")
    describe(hero)
    print("\n")
    // output:
    //   {name="Excalibur", damage=50, weight=3.2}
    //   {name="Heal", effect="restore_hp", uses=3}
    //   {name="Knight", health=200, speed=2}
}


// ============================================================
// Section 7: Field annotations (3-argument form)
// ============================================================
//
// Struct fields can carry metadata via annotations:
//   @annotation_name field : type          (bool, defaults to true)
//   @annotation_name=value field : type    (int, float, or string)
//   @annotation_name="text" field : type   (string with quotes)
//
// The 3-argument form of apply receives these annotations as
// array<tuple<name:string; data:RttiValue>> for each field.
// This enables custom serialization, validation, or display logic
// driven by declarative metadata.

struct DbRecord {
    @column = "user_name"  name  : string
    @column = "user_email" email : string
    @skip                id    : int
    @column = "age"        age   : int
}

def demo_annotations() {
    print("\n=== Section 7: Field annotations ===\n")

    let record = DbRecord(name = "Alice", email = "alice@example.com", id = 42, age = 30)

    // Build a pseudo-SQL insert using field annotations
    var columns : array<string>
    var values  : array<string>

    apply(record) $(name : string; field; annotations) {
        var column_name = name
        var skip = false
        for (ann in annotations) {
            if (ann.name == "skip") {
                skip = true
            } elif (ann.name == "column") {
                column_name = ann.data as tString
            }
        }
        if (!skip) {
            columns |> push(column_name)
            static_if (typeinfo stripped_typename(field) == "string") {
                values |> push("'{field}'")
            } else {
                values |> push("{field}")
            }
        }
    }

    let sep = ", "
    print("  INSERT INTO users ({join(columns, sep)})\n")
    print("  VALUES ({join(values, sep)})\n")
    // output:
    //   INSERT INTO users (user_name, user_email, age)
    //   VALUES ('Alice', 'alice@example.com', 30)
}


// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_basic_struct()
    demo_static_if()
    demo_mutation()
    demo_tuples()
    demo_variants()
    demo_describe()
    demo_annotations()
}

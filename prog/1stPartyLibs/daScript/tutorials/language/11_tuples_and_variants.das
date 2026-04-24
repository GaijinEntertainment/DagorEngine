// Tutorial 11: Tuples and Variants
//
// This tutorial covers:
//   - Tuple creation and field access (_0, _1, or named)
//   - The => operator for 2-element tuple construction
//   - Tuple destructuring in var and for
//   - Named tuple aliases with 'tuple Name { ... }'
//   - Variant types (tagged unions)
//   - Checking variants with 'is', extracting with 'as', safe access with '?as'
//   - Variant aliases with 'variant Name { ... }'
//   - Type aliases with typedef
//
// Run: daslang.exe tutorials/language/11_tuples_and_variants.das

options gen2

// === Tuple alias (shorthand for typedef) ===
// A named tuple with labeled fields.
tuple Point2D {
    x : float
    y : float
}

// === Variant alias ===
// A variant (tagged union) can hold exactly one of several named alternatives.
variant Value {
    i : int
    f : float
    s : string
}

// === typedef for simple aliases ===
typedef StringArray = array<string>

// Helper to describe a Value variant
def describeValue(v : Value) : string {
    if (v is i) {
        return "int({v as i})"
    } elif (v is f) {
        return "float({v as f})"
    } elif (v is s) {
        return "string(\"{v as s}\")"
    }
    return "unknown"
}

[export]
def main {

    // === Part 1: Tuples ===

    // --- Anonymous tuples ---
    // Create a tuple with parentheses and commas.
    var pair = (42, 3.14)
    print("pair = ({pair._0}, {pair._1})\n")

    // Modify fields using _0, _1, etc.
    pair._0 = 100
    print("modified pair._0 = {pair._0}\n")

    // --- Named tuples ---
    // Use the tuple alias declared above.
    var p = Point2D(x = 3.0, y = 4.0)
    print("point = ({p.x}, {p.y})\n")

    // Named fields are also accessible via _0, _1
    print("point._0 = {p._0}\n")

    // --- Tuple with tuple() constructor ---
    var triple = tuple(1, 2.0, "three")
    print("triple = ({triple._0}, {triple._1}, {triple._2})\n")

    // --- The => operator ---
    // The => operator creates a 2-element tuple from its operands.
    // It works in any expression, not just table literals.
    var kv = "age" => 25
    print("kv = ({kv._0}, {kv._1})\n")

    // This is handy for building arrays of key-value pairs.
    var entries <- ["one" => 1, "two" => 2, "three" => 3]
    for (entry in entries) {
        print("  {entry._0} => {entry._1}\n")
    }

    // --- Destructuring ---
    // Unpack a tuple into individual variables.
    var (a, b, c) = tuple(10, 20.0, "hello")
    print("destructured: a={a}, b={b}, c=\"{c}\"\n")

    // Destructuring works with named tuples too
    var (px, py) = Point2D(x = 5.0, y = 6.0)
    print("destructured point: px={px}, py={py}\n")

    // --- Destructuring in for loops ---
    var pairs <- [(1, "one"), (2, "two"), (3, "three")]
    print("pairs:\n")
    for ((num, name) in pairs) {
        print("  {num} = {name}\n")
    }

    // --- Tuples as return values ---
    // (Already covered in tutorial 05 with divmod.)
    // Tuples are great for returning multiple values.
    var (q, r) = divmod_tuple(17, 5)
    print("17 / 5 = {q} remainder {r}\n")

    // === Part 2: Variants ===

    // --- Creating variants ---
    // Construct by specifying which alternative is active.
    var v1 = Value(i = 42)
    var v2 = Value(f = 3.14)
    var v3 = Value(s = "hello")

    // --- Checking the active case with 'is' ---
    print("v1 is i: {v1 is i}\n")
    print("v1 is f: {v1 is f}\n")

    // --- Extracting with 'as' ---
    // 'as' returns the value if the case matches, panics otherwise.
    print("v1 as i = {v1 as i}\n")
    print("v2 as f = {v2 as f}\n")
    print("v3 as s = {v3 as s}\n")

    // --- Safe access with '?as' ---
    // '?as' returns the value if case matches, or the fallback after '??'
    let safeInt = v1 ?as i ?? -1
    let safeFlt = v1 ?as f ?? 0.0     // v1 is 'i', so returns fallback
    print("v1 ?as i ?? -1 = {safeInt}\n")
    print("v1 ?as f ?? 0.0 = {safeFlt}\n")

    // --- Using the describe function ---
    print("v1: {describeValue(v1)}\n")
    print("v2: {describeValue(v2)}\n")
    print("v3: {describeValue(v3)}\n")

    // --- Changing the active case ---
    // Assign a new variant value to switch cases.
    var v = Value(i = 10)
    print("before: {describeValue(v)}\n")
    v = Value(s = "switched!")
    print("after:  {describeValue(v)}\n")

    // --- variant_index ---
    // Get the current case index (0-based).
    var vi = Value(i = 1)
    var vf = Value(f = 2.0)
    print("index of i case: {variant_index(vi)}\n")
    print("index of f case: {variant_index(vf)}\n")

    // === Part 3: Type Aliases ===

    // typedef creates a name for any type.
    // StringArray was declared above as array<string>.
    var names : StringArray
    names |> push("Alice")
    names |> push("Bob")
    print("names: {names[0]}, {names[1]}\n")

    // You can also use typedef for function types, tables, etc.
    // typedef Lookup = table<string; int>
    // typedef Callback = function<(x:int):void>
}

def divmod_tuple(a, b : int) : tuple<int; int> {
    return (a / b, a % b)
}

// output:
// pair = (42, 3.14)
// modified pair._0 = 100
// point = (3, 4)
// point._0 = 3
// triple = (1, 2, three)
// kv = (age, 25)
//   one => 1
//   two => 2
//   three => 3
// destructured: a=10, b=20, c="hello"
// destructured point: px=5, py=6
// pairs:
//   1 = one
//   2 = two
//   3 = three
// 17 / 5 = 3 remainder 2
// v1 is i: true
// v1 is f: false
// v1 as i = 42
// v2 as f = 3.14
// v3 as s = hello
// v1 ?as i ?? -1 = 42
// v1 ?as f ?? 0.0 = 0
// v1: int(42)
// v2: float(3.14)
// v3: string("hello")
// before: int(10)
// after:  string("switched!")
// index of i case: 0
// index of f case: 1
// names: Alice, Bob

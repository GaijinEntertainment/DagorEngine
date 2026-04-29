options gen2

// Companion script for the "Low-Level Interop" tutorial.
// The module "tutorial_06_cpp" exposes interop functions that accept
// any daslang type and inspect TypeInfo at runtime.

require tutorial_06_cpp

// A pure daslang struct — TypeInfo has type==tStructure with StructInfo
// (unlike C++ handled types which have type==tHandle)
struct MyPoint {
    x : float
    y : float
    tag : string
}

[export]
def test() {
    // --- describe_type: works with any type ---
    print("=== describe_type ===\n")
    print("{describe_type(42)}\n")
    print("{describe_type(3.14)}\n")
    print("{describe_type("hello")}\n")
    print("{describe_type(true)}\n")

    // Handled type (bound via ManagedStructureAnnotation)
    let p = make_particle(1.0, 2.0, 0.5, -0.3)
    print("{describe_type(p)}\n")

    // Pure daslang struct
    let pt = MyPoint(x = 10.0, y = 20.0, tag = "origin")
    print("{describe_type(pt)}\n")

    // --- debug_print: format any value ---
    print("\n=== debug_print ===\n")
    print("int:    {debug_print(42)}\n")
    print("float:  {debug_print(3.14)}\n")
    print("string: {debug_print("hello")}\n")
    print("bool:   {debug_print(true)}\n")

    // --- any_hash: hash any value ---
    print("\n=== any_hash ===\n")
    let h1 = any_hash(42)
    let h2 = any_hash("hello")
    let h3 = any_hash(42)
    print("hash(42)      = {h1}\n")
    print("hash(\"hello\") = {h2}\n")
    print("hash(42) again = {h3}\n")
    print("same hash? {h1 == h3}\n")

    // --- struct_field_names: inspect pure daslang struct fields ---
    print("\n=== struct_field_names ===\n")
    print("MyPoint fields: {struct_field_names(pt)}\n")

    // --- call_site_info: source location of the call ---
    print("\n=== call_site_info ===\n")
    print("called from: {call_site_info()}\n")
}

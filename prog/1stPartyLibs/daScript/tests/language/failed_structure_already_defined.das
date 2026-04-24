// duplicate struct definition
// verifies compiler rejects defining the same struct name twice
options gen2
expect 30206    // structure_already_defined

struct Foo {
    a : int
}

struct Foo {
    a : int
}


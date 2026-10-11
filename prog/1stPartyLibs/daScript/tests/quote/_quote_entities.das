options gen2
options indenting = 4

// Shared entities for the quote-lowering A/B fixtures: declared in ONE named module so
// describe output is module-qualified identically on both sides of the comparison.

module _quote_entities shared

enum public Color {
    red
    green
    blue
}

struct public Pair {
    a : int
    b : string
}

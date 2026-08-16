options gen2

module _aot_so_mod

// #3212 fixture: Outer is declared BEFORE Inner and holds Inner by value.
struct public Outer {
    inner : Inner
    tag   : int
}

struct public Inner {
    x : int
    y : int
}

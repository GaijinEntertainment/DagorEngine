options gen2

require dastest/testing_boost public

struct Cell {
    raw : int
}

def operator . value(c : Cell) : int {
    return c . . raw * 2
}

def operator . value :=(var c : Cell; v : int) {
    c . . raw = v / 2
}

def operator . value +=(var c : Cell; v : int) {
    c . . raw += v / 2
}

def operator .(c : Cell; name : string) : int {
    return name == "twice" ? c . . raw * 2 : -1
}

[test]
def test_property_promotion_paths(t : T?) {
    t |> run("named getter") @(t : T?) {
        var c = Cell(raw = 21)
        var pc = unsafe(addr(c))
        t |> equal(c.value, 42)
        t |> equal(pc.value, 42)
    }
    t |> run("with-name getter") @(t : T?) {
        var c = Cell(raw = 4)
        var pc = unsafe(addr(c))
        t |> equal(c.twice, 8)
        t |> equal(pc.twice, 8)
        t |> equal(c.anything_else, -1)
    }
    t |> run("setter and op-assign") @(t : T?) {
        var c = Cell(raw = 0)
        var pc = unsafe(addr(c))
        c.value := 10
        t |> equal(c . . raw, 5)
        pc.value := 30
        t |> equal(c . . raw, 15)
        c.value += 4
        t |> equal(c . . raw, 17)
        pc.value += 6
        t |> equal(c . . raw, 20)
    }
    t |> run("getters in a with block") @(t : T?) {
        var c = Cell(raw = 3)
        var pc = unsafe(addr(c))
        with (c) {
            t |> equal(value, 6)
            t |> equal(twice, 6)
        }
        with (pc) {
            t |> equal(value, 6)
            t |> equal(twice, 6)
        }
    }
    t |> run("setter in a with block") @(t : T?) {
        var c = Cell(raw = 0)
        with (c) {
            value := 8
        }
        t |> equal(c . . raw, 4)
    }
}

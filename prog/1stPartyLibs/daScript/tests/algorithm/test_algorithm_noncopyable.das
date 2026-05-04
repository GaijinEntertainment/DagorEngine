options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require daslib/algorithm
require dastest/testing_boost public

// A non-copyable struct: the array<int> field prevents = (copy),
// but clone (:=), move (<-), and swap all work.

struct NoCopy {
    id   : int
    data : array<int>
}

def operator ==(a, b : NoCopy) : bool {
    return a.id == b.id
}

def operator !=(a, b : NoCopy) : bool {
    return a.id != b.id
}

def operator <(a, b : NoCopy) : bool {
    return a.id < b.id
}

// ---- erase_all with non-copyable type ----

[test]
def test_erase_all_non_copyable(t : T?) {
    t |> run("erases matching non-copyable elements") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1, data <- [10]))
        a |> emplace(NoCopy(id = 2, data <- [20]))
        a |> emplace(NoCopy(id = 3, data <- [30]))
        a |> emplace(NoCopy(id = 2, data <- [40]))
        a |> emplace(NoCopy(id = 4, data <- [50]))
        erase_all(a, NoCopy(id = 2))
        t |> equal(length(a), 3)
        t |> equal(a[0].id, 1)
        t |> equal(a[1].id, 3)
        t |> equal(a[2].id, 4)
        // verify data was preserved (swapped, not lost)
        t |> equal(a[0].data[0], 10)
        t |> equal(a[1].data[0], 30)
        t |> equal(a[2].data[0], 50)
    }
    t |> run("erases all non-copyable elements") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1))
        a |> emplace(NoCopy(id = 1))
        erase_all(a, NoCopy(id = 1))
        t |> equal(length(a), 0)
    }
    t |> run("no match leaves array intact") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1, data <- [10]))
        a |> emplace(NoCopy(id = 2, data <- [20]))
        erase_all(a, NoCopy(id = 99))
        t |> equal(length(a), 2)
        t |> equal(a[0].data[0], 10)
        t |> equal(a[1].data[0], 20)
    }
}

// ---- fill with non-copyable type ----

[test]
def test_fill_non_copyable(t : T?) {
    t |> run("fills with cloned non-copyable values") @(t : T?) {
        var a : array<NoCopy>
        resize(a, 3)
        var proto : NoCopy
        proto.id = 42
        proto.data |> push(1)
        proto.data |> push(2)
        fill(a, proto)
        // each element should be an independent clone
        for (i in range(3)) {
            t |> equal(a[i].id, 42)
            t |> equal(length(a[i].data), 2)
            t |> equal(a[i].data[0], 1)
            t |> equal(a[i].data[1], 2)
        }
        // verify independence: mutate one, others unaffected
        a[0].data[0] = 999
        t |> equal(a[1].data[0], 1)
        t |> equal(a[2].data[0], 1)
    }
}

// ---- combine with non-copyable type ----

[test]
def test_combine_non_copyable(t : T?) {
    t |> run("combines arrays of non-copyable type") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1, data <- [10]))
        a |> emplace(NoCopy(id = 2, data <- [20]))
        var b : array<NoCopy>
        b |> emplace(NoCopy(id = 3, data <- [30]))
        var c <- combine(a, b)
        t |> equal(length(c), 3)
        t |> equal(c[0].id, 1)
        t |> equal(c[1].id, 2)
        t |> equal(c[2].id, 3)
        // verify data cloned properly
        t |> equal(c[0].data[0], 10)
        t |> equal(c[1].data[0], 20)
        t |> equal(c[2].data[0], 30)
        // verify source arrays still intact
        t |> equal(length(a), 2)
        t |> equal(a[0].data[0], 10)
    }
}

// ---- unique with non-copyable type ----

[test]
def test_unique_non_copyable(t : T?) {
    t |> run("unique on sorted non-copyable array") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1, data <- [10]))
        a |> emplace(NoCopy(id = 1, data <- [11]))
        a |> emplace(NoCopy(id = 2, data <- [20]))
        a |> emplace(NoCopy(id = 2, data <- [21]))
        a |> emplace(NoCopy(id = 3, data <- [30]))
        var b <- unique(a)
        t |> equal(length(b), 3)
        t |> equal(b[0].id, 1)
        t |> equal(b[1].id, 2)
        t |> equal(b[2].id, 3)
    }
}

// ---- reverse with non-copyable type ----

[test]
def test_reverse_non_copyable(t : T?) {
    t |> run("reverses non-copyable array") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1, data <- [10]))
        a |> emplace(NoCopy(id = 2, data <- [20]))
        a |> emplace(NoCopy(id = 3, data <- [30]))
        reverse(a)
        t |> equal(a[0].id, 3)
        t |> equal(a[1].id, 2)
        t |> equal(a[2].id, 1)
        t |> equal(a[0].data[0], 30)
        t |> equal(a[2].data[0], 10)
    }
}

// ---- rotate with non-copyable type ----

[test]
def test_rotate_non_copyable(t : T?) {
    t |> run("rotates non-copyable array") @(t : T?) {
        var a : array<NoCopy>
        a |> emplace(NoCopy(id = 1, data <- [10]))
        a |> emplace(NoCopy(id = 2, data <- [20]))
        a |> emplace(NoCopy(id = 3, data <- [30]))
        a |> emplace(NoCopy(id = 4, data <- [40]))
        rotate(a, 2)
        t |> equal(a[0].id, 3)
        t |> equal(a[1].id, 4)
        t |> equal(a[2].id, 1)
        t |> equal(a[3].id, 2)
        t |> equal(a[0].data[0], 30)
        t |> equal(a[3].data[0], 20)
    }
}

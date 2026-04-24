options gen2
options strict_smart_pointers

require dastest/testing_boost public

class StaticCounter {
    static count : int = 1
    static mount : array<int> <- array<int>(1, 2, 3, 4)
    def next {
        return count++
    }
    def get {
        return count
    }
    def static next_func {
        return count++
    }
}

[test]
def test_static_members(t : T?) {
    t |> run("variables") @(t : T?) {
        var c1 = new StaticCounter()
        t |> equal(1, c1->get())
        StaticCounter`count = 13
        t |> equal(13, c1->get())
        t |> equal(13, c1->next())
        t |> equal(14, c1->next())
        var c2 = new StaticCounter()
        t |> equal(15, c2->next())
        t |> equal(16, StaticCounter`count)
        t |> equal(16, c1->get())
        t |> equal(4, length(StaticCounter`mount))
    }
    t |> run("functions") @(t : T?) {
        StaticCounter`count = 13
        t |> equal(13, StaticCounter`next_func())
        t |> equal(14, StaticCounter`next_func())
    }
}


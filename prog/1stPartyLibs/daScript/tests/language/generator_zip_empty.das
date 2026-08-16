options gen2

require dastest/testing_boost public

// Regression: the generator for-loop lowering emitted `loop &&= _builtin_iterator_first(...)`,
// short-circuiting first() on later sources when an earlier one came up empty — but the
// end-of-loop path closes ALL sources, and closing a never-opened container iterator unlocks
// a container whose lock magic was already cleared ("magic mismatch on unlock").
// first() must run for every source, matching SimNode_ForWithIterator.

[test]
def test_generator_zip_empty_source(t : T?) {
    t |> run("two array iterators, first empty") @(t : T?) {
        let a : array<int>
        var b : array<int>
        b |> push(1)
        var ait <- unsafe(each(a))
        var bit <- unsafe(each(b))
        var g <- generator<int -const> capture(<- ait, <- bit) {
            for (x, y in ait, bit) {
                yield x + y
            }
            return false
        }
        var n = 0
        for (_v in g) {
            n++
        }
        t |> equal(n, 0)
    }
    t |> run("two array iterators, second empty") @(t : T?) {
        var a : array<int>
        let b : array<int>
        a |> push(1)
        var ait <- unsafe(each(a))
        var bit <- unsafe(each(b))
        var g <- generator<int -const> capture(<- ait, <- bit) {
            for (x, y in ait, bit) {
                yield x + y
            }
            return false
        }
        var n = 0
        for (_v in g) {
            n++
        }
        t |> equal(n, 0)
    }
    t |> run("two table iterators, table empty") @(t : T?) {
        let tab : table<int; int>
        var kit <- unsafe(keys(tab))
        var vit <- unsafe(values(tab))
        var g <- generator<tuple<k : int; v : int> -const> capture(<- kit, <- vit) {
            for (k, v in kit, vit) {
                yield (k = k, v = v)
            }
            return false
        }
        var n = 0
        for (_kv in g) {
            n++
        }
        t |> equal(n, 0)
    }
}

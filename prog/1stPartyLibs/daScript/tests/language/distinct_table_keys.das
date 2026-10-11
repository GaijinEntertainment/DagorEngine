options gen2

require dastest/testing_boost public

typedef distinct Meters = int
typedef distinct Feet = int
typedef distinct Label = string

[test]
def test_distinct_table_keys(t : T?) {
    t |> run("two tables keyed by different distinct types over int stay apart") @(t : T?) {
        var by_meters : table<Meters; string>
        var by_feet : table<Feet; string>
        by_meters[Meters(3)] = "three meters"
        by_feet[Feet(3)] = "three feet"
        by_feet[Feet(10)] = "ten feet"
        t |> equal(by_meters?[Meters(3)] ?? "missing", "three meters")
        t |> equal(by_feet?[Feet(3)] ?? "missing", "three feet")
        t |> equal(by_feet?[Feet(10)] ?? "missing", "ten feet")
        t |> equal(by_meters?[Meters(10)] ?? "missing", "missing")
        t |> equal(length(by_meters), 1)
        t |> equal(length(by_feet), 2)
        t |> success(key_exists(by_feet, Feet(10)), "key_exists sees the feet key")
        t |> success(!key_exists(by_meters, Meters(10)), "the meters table never saw 10")
    }
    t |> run("erase and re-insert through the distinct key") @(t : T?) {
        var tab : table<Meters; int>   // nolint:STYLE031 - the arm pins the index-insert path, not the literal
        tab[Meters(1)] = 10
        tab[Meters(2)] = 20
        t |> success(erase(tab, Meters(1)), "erase of a present key answers true")
        t |> success(!erase(tab, Meters(1)), "erase of an absent key answers false")
        t |> equal(length(tab), 1)
        tab[Meters(1)] = 11
        t |> equal(tab?[Meters(1)] ?? 0, 11)
    }
    t |> run("iteration and keys() yield the distinct key") @(t : T?) {
        var tab <- { Meters(4) => "four", Meters(6) => "six" }
        var total = 0
        for (k, v in keys(tab), values(tab)) {
            total += *k
            t |> success(v == "four" || v == "six", "the value rides with its key")
        }
        t |> equal(total, 10)
        var keys_only_total = 0
        for (k in keys(tab)) {
            keys_only_total += *k
        }
        t |> equal(keys_only_total, 10)
    }
    t |> run("get and a pointer index reach the key through the distinct") @(t : T?) {
        var tab : table<Meters; string>
        tab[Meters(1)] = "one"
        let found = tab |> get(Meters(1)) $(v) {
            t |> equal(v, "one")
        }
        t |> success(found, "get visits the present key")
        t |> success(!(tab |> get(Meters(2)) $(v) {}), "get skips the absent key")
        var ptab = unsafe(addr(tab))
        t |> equal(ptab?[Meters(1)] ?? "missing", "one")
        t |> equal(ptab?[Meters(2)] ?? "missing", "missing")
    }
    t |> run("a distinct over string keys a table too") @(t : T?) {
        let tab <- { Label("alpha") => 1, Label("beta") => 2 }
        t |> equal(tab?[Label("beta")] ?? 0, 2)
        t |> equal(tab?[Label("gamma")] ?? 0, 0)
        t |> success(key_exists(tab, Label("alpha")), "the string underneath hashes the key")
    }
    t |> run("a set keyed by a distinct type") @(t : T?) {
        var seen : table<Feet>   // nolint:STYLE031 - the repeated key is the point; a literal with it is a compile error
        seen |> insert(Feet(7))
        seen |> insert(Feet(7))
        seen |> insert(Feet(8))
        t |> equal(length(seen), 2)
        t |> success(key_exists(seen, Feet(8)), "set membership through the distinct key")
    }
}

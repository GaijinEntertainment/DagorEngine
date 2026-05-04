options gen2
require dastest/testing_boost public

[test]
def test_table_literals_still_work(t : T?) {
    t |> run("table in variable declaration") @(t : T?) {
        var tab = {"one" => 1, "two" => 2}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, "one"))
        t |> success(key_exists(tab, "two"))
    }
    t |> run("table in parenthesized expression") @(t : T?) {
        var x = ({"x" => 10})
        t |> equal(length(x), 1)
        t |> success(key_exists(x, "x"))
    }
    t |> run("table in function argument") @(t : T?) {
        var tab = {"a" => 100, "b" => 200}
        t |> equal(length(tab), 2)
    }
    t |> run("table after return keyword") @(t : T?) {
        let result = get_table()
        t |> success(key_exists(result, "hello"))
    }
}

def get_table() : table<string; int> {
    return {"hello" => 42}
}

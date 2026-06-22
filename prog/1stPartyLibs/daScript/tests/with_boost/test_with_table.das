options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/with_boost

struct A {
    f1 : int
    f2 : int
}

[test]
def test_existing_key(t : T?) {
    //! Mutation on an existing key persists in the table.
    var tab : table<string; A>
    tab |> insert("k", A(f1 = 1, f2 = 2))
    with_(tab["k"]) $(v) {
        v.f1 = 99
        v.f2 = 100
    }
    t |> equal(tab["k"].f1, 99, "key mutation persists")
    t |> equal(tab["k"].f2, 100, "second field mutation persists")
}

[test]
def test_missing_key_creates_default(t : T?) {
    //! tab[k] is upsert: missing key gets a default entry, then the
    //! body's mutations persist on that newly-created entry.
    var tab : table<string; A>
    with_(tab["new"]) $(v) {
        v.f1 = 42
    }
    t |> success(key_exists(tab, "new"), "missing key was created")
    t |> equal(tab["new"].f1, 42, "mutation on new entry persists")
}

[test]
def test_table_default_name(t : T?) {
    //! Default `_` binding works for tables too.
    var tab : table<string; A>
    tab |> insert("x", A(f1 = 5, f2 = 6))
    with_(tab["x"]) {
        _.f1 = 77
    }
    t |> equal(tab["x"].f1, 77, "default _ binding mutation persists")
}

[test]
def test_table_workhorse_value(t : T?) {
    //! Table with workhorse value type — block-arg is `int&`, mutation
    //! persists.
    var tab : table<string; int>
    tab |> insert("alpha", 10)
    tab |> insert("beta", 20)
    with_(tab["alpha"]) $(v) {
        v = 200
    }
    t |> equal(tab["alpha"], 200, "workhorse value mutation persists")
    t |> equal(tab["beta"], 20, "other key untouched")
}

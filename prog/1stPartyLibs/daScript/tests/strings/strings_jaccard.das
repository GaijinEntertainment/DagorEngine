options gen2
require dastest/testing_boost
require daslib/strings_boost

def make_set(items : array<string>) : table<string> {
    var s : table<string>
    for (x in items) {
        s |> insert(x)
    }
    return <- s
}

[test]
def test_jaccard_table(t : T?) {
    t |> run("equal sets") @@(t : T?) {
        let a <- make_set(["foo", "bar", "baz"])
        let b <- make_set(["foo", "bar", "baz"])
        t |> equal(jaccard(a, b), 1.0f)
    }

    t |> run("disjoint sets") @@(t : T?) {
        let a <- make_set(["foo", "bar"])
        let b <- make_set(["baz", "qux"])
        t |> equal(jaccard(a, b), 0.0f)
    }

    t |> run("partial overlap") @@(t : T?) {
        let a <- make_set(["foo", "bar", "baz"])
        let b <- make_set(["foo", "bar", "qux"])
        t |> equal(jaccard(a, b), 0.5f)
    }

    t |> run("subset") @@(t : T?) {
        let a <- make_set(["foo", "bar"])
        let b <- make_set(["foo", "bar", "baz", "qux"])
        t |> equal(jaccard(a, b), 0.5f)
    }

    t |> run("empty either side returns 0") @@(t : T?) {
        let a <- make_set(["foo"])
        let b : table<string>
        t |> equal(jaccard(a, b), 0.0f)
        t |> equal(jaccard(b, a), 0.0f)
    }

    t |> run("both empty returns 0") @@(t : T?) {
        let a : table<string>
        let b : table<string>
        t |> equal(jaccard(a, b), 0.0f)
    }
}

[test]
def test_jaccard_array(t : T?) {
    t |> run("array overload matches table form") @@(t : T?) {
        let a <- ["foo", "bar", "baz"]
        let b <- ["foo", "bar", "qux"]
        t |> equal(jaccard(a, b), 0.5f)
    }

    t |> run("array dedupes via set conversion") @@(t : T?) {
        let a <- ["foo", "foo", "bar"]   // dedupes to {foo, bar}
        let b <- ["foo", "bar"]
        t |> equal(jaccard(a, b), 1.0f)
    }

    t |> run("array empty returns 0") @@(t : T?) {
        let a <- ["foo"]
        let b : array<string>
        t |> equal(jaccard(a, b), 0.0f)
    }
}

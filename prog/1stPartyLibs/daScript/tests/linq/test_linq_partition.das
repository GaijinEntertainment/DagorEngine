options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_skip(t : T?) {
    t |> run("basic skip") @(t : T?) {
        var query = skip(
            [iterator for(x in 0..5); x],
            2
        )
        for (c, i in query, 0..3) {
            t |> equal(c, i + 2)
        }
    }
    t |> run("skip while") @(t : T?) {
        var query_skip = _skip_while(
            [iterator for(x in 0..5); x],
            _ < 3
        )
        for (c, i in query_skip, 0..3) {
            t |> equal(c, i + 3)
        }
    }
    t |> run("complex skip") @(t : T?) {
        var qcomplex = skip(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            2
        )
        for (c, i in qcomplex, 0..3) {
            t |> success(c.a.Equal([i + 2, (i + 2) * 10]))
        }
    }
    t |> run("complex skip while") @(t : T?) {
        var qcomplex_skip_while = _skip_while(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            _.a[0] < 3
        )
        for (c, i in qcomplex_skip_while, 0..2) {
            t |> success(c.a.Equal([i + 3, (i + 3) * 10]))
        }
    }
    t |> run("skip to array") @(t : T?) {
        var query_array = skip_to_array(
            [iterator for(x in 0..5); x],
            2
        )
        static_assert(typeinfo is_array(query_array))
        for (c, i in query_array, 0..3) {
            t |> equal(c, i + 2)
        }
    }
    t |> run("skip in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4, 5]
        skip_inplace(arr, 2)
        t |> equal(4, length(arr))
        for (c, i in arr, 0..3) {
            t |> equal(c, i + 2)
        }
    }
}

[test]
def test_take(t : T?) {
    t |> run("basic take") @(t : T?) {
        var query = take(
            [iterator for(x in 0..5); x],
            3
        )
        for (c, i in query, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("complex take while") @(t : T?) {
        var query_take_while = _take_while(
            [iterator for(x in 0..5); x],
            _ < 3
        )
        for (c, i in query_take_while, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("complex take range") @(t : T?) {
        var query_take_range = take(
            [for (x in 0..5); x],
            1..4
        )
        t |> equal(3, length(query_take_range))
        for (c, i in query_take_range, 0..3) {
            t |> equal(c, i + 1)
        }
    }
    t |> run("complex take") @(t : T?) {
        var qcomplex = take(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            3
        )
        for (c, i in qcomplex, 0..3) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("complex take while") @(t : T?) {
        var qcomplex_take_while = _take_while(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            _.a[0] < 3
        )
        for (c, i in qcomplex_take_while, 0..3) {
            t |> success(c.a.Equal([i, i * 10]))
        }
    }
    t |> run("take from array") @(t : T?) {
        let arr = [for (x in 0..5); x]
        var result = take(arr, 3)
        t |> equal(3, length(result))
        for (c, i in result, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("take to array") @(t : T?) {
        var query_array = take_to_array(
            [iterator for(x in 0..5); x],
            3
        )
        static_assert(typeinfo is_array(query_array))
        for (c, i in query_array, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("take in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4, 5]
        take_inplace(arr, 3)
        t |> equal(3, length(arr))
        for (c, i in arr, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("take range in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4, 5]
        take_inplace(arr, 1..4)
        t |> equal(3, length(arr))
        for (c, i in arr, 0..3) {
            t |> equal(c, i + 1)
        }
    }
    t |> run("take range to array") @(t : T?) {
        var query_array = take_to_array(
            [iterator for(x in 0..5); x],
            1..4
        )
        static_assert(typeinfo is_array(query_array))
        t |> equal(3, length(query_array))
        for (c, i in query_array, 0..3) {
            t |> equal(c, i + 1)
        }
    }
}

[test]
def test_skip_last(t : T?) {
    t |> run("skip last from iterator") @(t : T?) {
        var query = skip_last(
            [iterator for(x in 0..5); x],
            2
        )
        var result : array<int>
        for (c in query) {
            result |> push(c)
        }
        t |> equal(3, length(result))
        t |> equal(result[0], 0)
        t |> equal(result[1], 1)
        t |> equal(result[2], 2)
    }
    t |> run("skip last from array") @(t : T?) {
        var arr = [0, 1, 2, 3, 4]
        var result = skip_last(arr, 2)
        t |> equal(3, length(result))
        for (c, i in result, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("skip last in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4]
        skip_last_inplace(arr, 2)
        t |> equal(3, length(arr))
        for (c, i in arr, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("skip last to array") @(t : T?) {
        var result = skip_last_to_array(
            [iterator for(x in 0..5); x],
            2
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        for (c, i in result, 0..3) {
            t |> equal(c, i)
        }
    }
    t |> run("skip last all") @(t : T?) {
        var arr = [0, 1, 2]
        var result = skip_last(arr, 5)
        t |> equal(0, length(result))
    }
    t |> run("skip last zero") @(t : T?) {
        var arr = [0, 1, 2]
        var result = skip_last(arr, 0)
        t |> equal(3, length(result))
    }
    t |> run("skip last with _fold") @(t : T?) {
        var result = (
            [0, 1, 2, 3, 4]
            .skip_last(2)
            ._fold()
        )
        t |> equal(typeinfo typename(result), "array<int>")
        t |> equal(3, length(result))
        for (c, i in result, 0..3) {
            t |> equal(c, i)
        }
    }
}

[test]
def test_take_last(t : T?) {
    t |> run("take last from iterator") @(t : T?) {
        var query = take_last(
            [iterator for(x in 0..5); x],
            2
        )
        var result : array<int>
        for (c in query) {
            result |> push(c)
        }
        t |> equal(2, length(result))
        t |> equal(result[0], 3)
        t |> equal(result[1], 4)
    }
    t |> run("take last from array") @(t : T?) {
        var arr = [0, 1, 2, 3, 4]
        var result = take_last(arr, 2)
        t |> equal(2, length(result))
        t |> equal(result[0], 3)
        t |> equal(result[1], 4)
    }
    t |> run("take last in place") @(t : T?) {
        var arr = [0, 1, 2, 3, 4]
        take_last_inplace(arr, 2)
        t |> equal(2, length(arr))
        t |> equal(arr[0], 3)
        t |> equal(arr[1], 4)
    }
    t |> run("take last to array") @(t : T?) {
        var result = take_last_to_array(
            [iterator for(x in 0..5); x],
            2
        )
        static_assert(typeinfo is_array(result))
        t |> equal(2, length(result))
        t |> equal(result[0], 3)
        t |> equal(result[1], 4)
    }
    t |> run("take last all") @(t : T?) {
        var arr = [0, 1, 2]
        var result = take_last(arr, 5)
        t |> equal(3, length(result))
    }
    t |> run("take last zero") @(t : T?) {
        var arr = [0, 1, 2]
        var result = take_last(arr, 0)
        t |> equal(0, length(result))
    }
    t |> run("take last with _fold") @(t : T?) {
        var result = (
            [0, 1, 2, 3, 4]
            .take_last(2)
            ._fold()
        )
        t |> equal(typeinfo typename(result), "array<int>")
        t |> equal(2, length(result))
        t |> equal(result[0], 3)
        t |> equal(result[1], 4)
    }
}

[test]
def text_chunk(t : T?) {
    t |> run("basic chunk") @(t : T?) {
        var query = chunk(
            [iterator for(x in 0..6); x],
            2
        )
        var expected = [
            [0, 1],
            [2, 3],
            [4, 5]
        ]
        for (c, e in query, expected) {
            for (i, v in c, e) {
                t |> equal(i, v)
            }
        }
    }
    t |> run("complex chunk") @(t : T?) {
        var qcomplex = chunk(
            [iterator for(x in 0..6); ComplexType(a = [x, x * 10])],
            2
        )
        var expected_complex = [
            [(0, 0), (1, 10)],
            [(2, 20), (3, 30)],
            [(4, 40), (5, 50)]
        ]
        for (c, e in qcomplex, expected_complex) {
            for (i, v in c, e) {
                t |> equal(i.a[0], v._0)
                t |> equal(i.a[1], v._1)
            }
        }
    }
    t |> run("chunk to array") @(t : T?) {
        var query_array = chunk_to_array(
            [iterator for(x in 0..6); x],
            2
        )
        static_assert(typeinfo is_array(query_array))
        var expected = [
            [0, 1],
            [2, 3],
            [4, 5]
        ]
        for (c, e in query_array, expected) {
            for (i, v in c, e) {
                t |> equal(i, v)
            }
        }
    }
}

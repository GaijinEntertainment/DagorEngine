options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

// ============================================================================
// Tests for BUG 1: unique_inplace / unique_by_inplace on empty arrays
// Previously crashed by resizing empty array to 1
// ============================================================================

[test]
def test_unique_inplace_empty(t : T?) {
    t |> run("unique_inplace on empty array") @(t : T?) {
        var arr : array<int>
        unique_inplace(arr)
        t |> equal(length(arr), 0)
    }
    t |> run("unique_inplace on single element") @(t : T?) {
        var arr = [42]
        unique_inplace(arr)
        t |> equal(length(arr), 1)
        t |> equal(arr[0], 42)
    }
    t |> run("unique_inplace on two identical elements") @(t : T?) {
        var arr = [7, 7]
        unique_inplace(arr)
        t |> equal(length(arr), 1)
        t |> equal(arr[0], 7)
    }
    t |> run("unique_inplace on two different elements") @(t : T?) {
        var arr = [3, 5]
        unique_inplace(arr)
        t |> equal(length(arr), 2)
        t |> equal(arr[0], 3)
        t |> equal(arr[1], 5)
    }
    t |> run("unique_inplace normal case") @(t : T?) {
        var arr = [1, 1, 2, 2, 3, 3, 3, 4]
        unique_inplace(arr)
        t |> equal(length(arr), 4)
        t |> equal(arr[0], 1)
        t |> equal(arr[1], 2)
        t |> equal(arr[2], 3)
        t |> equal(arr[3], 4)
    }
    t |> run("unique_inplace all identical") @(t : T?) {
        var arr = [5, 5, 5, 5, 5]
        unique_inplace(arr)
        t |> equal(length(arr), 1)
        t |> equal(arr[0], 5)
    }
    t |> run("unique_inplace no duplicates") @(t : T?) {
        var arr = [1, 2, 3, 4, 5]
        unique_inplace(arr)
        t |> equal(length(arr), 5)
        for (i in 0..5) {
            t |> equal(arr[i], i + 1)
        }
    }
}

[test]
def test_unique_by_inplace_empty(t : T?) {
    t |> run("unique_by_inplace on empty array") @(t : T?) {
        var arr : array<int>
        unique_by_inplace(arr, $(_ : int) => _)
        t |> equal(length(arr), 0)
    }
    t |> run("unique_by_inplace on single element") @(t : T?) {
        var arr = [42]
        unique_by_inplace(arr, $(_ : int) => _)
        t |> equal(length(arr), 1)
        t |> equal(arr[0], 42)
    }
    t |> run("unique_by_inplace with complex key") @(t : T?) {
        var arr = [
            ComplexType(a = [1, 10]),
            ComplexType(a = [1, 10]),
            ComplexType(a = [2, 20])
        ]
        unique_by_inplace(arr, $(c : ComplexType) => c.a[0])
        t |> equal(length(arr), 2)
        t |> equal(arr[0].a[0], 1)
        t |> equal(arr[1].a[0], 2)
    }
    t |> run("unique_by_inplace empty complex array") @(t : T?) {
        var arr : array<ComplexType>
        unique_by_inplace(arr, $(c : ComplexType) => c.a[0])
        t |> equal(length(arr), 0)
    }
}

[test]
def test_unique_empty_via_unique(t : T?) {
    t |> run("unique on empty array") @(t : T?) {
        var arr : array<int>
        var result = unique(arr)
        t |> equal(length(result), 0)
    }
    t |> run("unique_by on empty array") @(t : T?) {
        var arr : array<int>
        var result = _unique_by(arr, _)
        t |> equal(length(result), 0)
    }
}

// ============================================================================
// Tests for BUG 2: select_impl (indexed) had inverted lock flags
// ============================================================================

[test]
def test_select_indexed(t : T?) {
    t |> run("select indexed from iterator") @(t : T?) {
        var query = select(
            [iterator for(x in 0..5); x * 10]
        )
        var count = 0
        for (q in query) {
            t |> equal(q._0, count)
            t |> equal(q._1, count * 10)
            count ++
        }
        t |> equal(count, 5)
    }
    t |> run("select indexed from array") @(t : T?) {
        var arr = [10, 20, 30, 40, 50]
        var query = select(arr)
        t |> equal(length(query), 5)
        for (i in 0..5) {
            t |> equal(query[i]._0, i)
            t |> equal(query[i]._1, (i + 1) * 10)
        }
    }
    t |> run("select indexed to array") @(t : T?) {
        var query = select_to_array(
            [iterator for(x in 0..3); x * 100]
        )
        t |> equal(length(query), 3)
        t |> equal(query[0]._0, 0)
        t |> equal(query[0]._1, 0)
        t |> equal(query[1]._0, 1)
        t |> equal(query[1]._1, 100)
        t |> equal(query[2]._0, 2)
        t |> equal(query[2]._1, 200)
    }
    t |> run("select indexed empty") @(t : T?) {
        var query = select(
            [iterator for(x in 0..0); x]
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("select indexed complex type") @(t : T?) {
        var query = select(
            [iterator for(x in 0..3); ComplexType(a = [x, x * 10])]
        )
        var count = 0
        for (q in query) {
            t |> equal(q._0, count)
            t |> equal(q._1.a[0], count)
            t |> equal(q._1.a[1], count * 10)
            count ++
        }
        t |> equal(count, 3)
    }
}

// ============================================================================
// Tests for BUG 3: sequence_equal didn't check lengths
// [1,2,3] was considered equal to [1,2]
// ============================================================================

[test]
def test_sequence_equal_length(t : T?) {
    t |> run("sequence_equal different lengths - first longer") @(t : T?) {
        var result = sequence_equal(
            [iterator for(x in 0..5); x],
            [iterator for(x in 0..3); x]
        )
        t |> success(!result)
    }
    t |> run("sequence_equal different lengths - second longer") @(t : T?) {
        var result = sequence_equal(
            [iterator for(x in 0..3); x],
            [iterator for(x in 0..5); x]
        )
        t |> success(!result)
    }
    t |> run("sequence_equal empty vs non-empty") @(t : T?) {
        var result = sequence_equal(
            [iterator for(x in 0..0); x],
            [iterator for(x in 0..3); x]
        )
        t |> success(!result)
    }
    t |> run("sequence_equal non-empty vs empty") @(t : T?) {
        var result = sequence_equal(
            [iterator for(x in 0..3); x],
            [iterator for(x in 0..0); x]
        )
        t |> success(!result)
    }
    t |> run("sequence_equal both empty") @(t : T?) {
        var result = sequence_equal(
            [iterator for(x in 0..0); x],
            [iterator for(x in 0..0); x]
        )
        t |> success(result)
    }
    t |> run("sequence_equal same content same length") @(t : T?) {
        var result = sequence_equal(
            [iterator for(x in 0..5); x * 2],
            [iterator for(x in 0..5); x * 2]
        )
        t |> success(result)
    }
    t |> run("sequence_equal array different lengths") @(t : T?) {
        var a = [1, 2, 3]
        var b = [1, 2, 3, 4, 5]
        var result = sequence_equal(a, b)
        t |> success(!result)
    }
    t |> run("sequence_equal array same") @(t : T?) {
        var a = [1, 2, 3]
        var b = [1, 2, 3]
        var result = sequence_equal(a, b)
        t |> success(result)
    }
    t |> run("sequence_equal prefix match should fail") @(t : T?) {
        // This was the main bug: [1,2] is a prefix of [1,2,3] but they are NOT equal
        var result = sequence_equal(
            [iterator for(x in 0..3); x],
            [iterator for(x in 0..5); x]
        )
        t |> success(!result)
    }
}

[test]
def test_sequence_equal_by_length(t : T?) {
    t |> run("sequence_equal_by different lengths") @(t : T?) {
        var result = _sequence_equal_by(
            [iterator for(x in 0..5); x],
            [iterator for(x in 0..3); x],
            _
        )
        t |> success(!result)
    }
    t |> run("sequence_equal_by both empty") @(t : T?) {
        var result = _sequence_equal_by(
            [iterator for(x in 0..0); x],
            [iterator for(x in 0..0); x],
            _
        )
        t |> success(result)
    }
    t |> run("sequence_equal_by same content") @(t : T?) {
        var result = _sequence_equal_by(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            _.a[0]
        )
        t |> success(result)
    }
    t |> run("sequence_equal_by array different lengths") @(t : T?) {
        var a = [1, 2, 3]
        var b = [1, 2]
        var result = _sequence_equal_by(a, b, _)
        t |> success(!result)
    }
}

// ============================================================================
// Tests for BUG 4: intersect yielded duplicates from second source
// ============================================================================

[test]
def test_intersect_duplicates(t : T?) {
    t |> run("intersect with duplicates in second source") @(t : T?) {
        // srca = [1,2,3], srcb = [2,2,3,3,3]
        // should yield [2,3] not [2,2,3,3,3]
        var arr1 = [1, 2, 3]
        var arr2 = [2, 2, 3, 3, 3]
        var result = intersect(arr1, arr2)
        t |> equal(length(result), 2)
        t |> equal(result[0], 2)
        t |> equal(result[1], 3)
    }
    t |> run("intersect with duplicates in first source") @(t : T?) {
        // srca = [1,1,2,2,3], srcb = [2,3]
        // seen from srca: {1,2,3}. srcb: 2→match(erase 2), 3→match(erase 3)
        // result: [2,3]
        var arr1 = [1, 1, 2, 2, 3]
        var arr2 = [2, 3]
        var result = intersect(arr1, arr2)
        t |> equal(length(result), 2)
        t |> equal(result[0], 2)
        t |> equal(result[1], 3)
    }
    t |> run("intersect with duplicates in both sources") @(t : T?) {
        var arr1 = [1, 1, 2, 2, 3]
        var arr2 = [2, 2, 3, 3, 4]
        var result = intersect(arr1, arr2)
        t |> equal(length(result), 2)
        t |> equal(result[0], 2)
        t |> equal(result[1], 3)
    }
    t |> run("intersect no overlap") @(t : T?) {
        var arr1 = [1, 2, 3]
        var arr2 = [4, 5, 6]
        var result = intersect(arr1, arr2)
        t |> equal(length(result), 0)
    }
    t |> run("intersect_by with duplicates") @(t : T?) {
        var arr1 = [1, 2, 3]
        var arr2 = [12, 12, 13, 13]
        // key: _ % 10, so arr1 keys={1,2,3}, arr2 keys: 12%10=2, 12%10=2, 13%10=3, 13%10=3
        // should yield [12, 13] not [12, 12, 13, 13]
        var result = _intersect_by(arr1, arr2, _ % 10)
        t |> equal(length(result), 2)
        t |> equal(result[0], 12)
        t |> equal(result[1], 13)
    }
}

// ============================================================================
// Tests for BUG 5: except didn't deduplicate first source
// ============================================================================

[test]
def test_except_duplicates(t : T?) {
    t |> run("except with duplicates in source") @(t : T?) {
        // src = [1,1,2,2,3], exclude = [3]
        // should yield [1,2] not [1,1,2,2]
        var arr1 = [1, 1, 2, 2, 3]
        var arr2 = [3]
        var result = except(arr1, arr2)
        t |> equal(length(result), 2)
        t |> equal(result[0], 1)
        t |> equal(result[1], 2)
    }
    t |> run("except with all duplicates excluded") @(t : T?) {
        var arr1 = [1, 1, 2, 2]
        var arr2 = [1, 2]
        var result = except(arr1, arr2)
        t |> equal(length(result), 0)
    }
    t |> run("except with no overlap and duplicates") @(t : T?) {
        var arr1 = [1, 1, 2, 2]
        var arr2 = [3, 4]
        var result = except(arr1, arr2)
        t |> equal(length(result), 2)
        t |> equal(result[0], 1)
        t |> equal(result[1], 2)
    }
    t |> run("except_by with duplicates in source") @(t : T?) {
        // src = [1,11,2,12,3], key = _ % 10
        // exclude = [3], key 3%10=3
        // src keys: 1,1,2,2,3
        // items with key NOT in exclude and deduped: 1 (key=1, first), 2 (key=2, first)
        var arr1 = [1, 11, 2, 12, 3]
        var arr2 = [3]
        var result = _except_by(arr1, arr2, _ % 10)
        t |> equal(length(result), 2)
        t |> equal(result[0], 1)
        t |> equal(result[1], 2)
    }
}

// ============================================================================
// Tests for range_sequence
// ============================================================================

[test]
def test_range_sequence_count(t : T?) {
    t |> run("range_sequence produces correct count") @(t : T?) {
        var query = range_sequence(0, 5)
        var count = 0
        for (_ in query) {
            count ++
        }
        t |> equal(count, 5)
    }
    t |> run("range_sequence values") @(t : T?) {
        var query = range_sequence(3, 4)
        var expected = [3, 4, 5, 6]
        for (q, e in query, expected) {
            t |> equal(q, e)
        }
    }
    t |> run("range_sequence zero count") @(t : T?) {
        var query = range_sequence(0, 0)
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("range_sequence count 1") @(t : T?) {
        var query = range_sequence(10, 1)
        var count = 0
        var val = -1
        for (q in query) {
            val = q
            count ++
        }
        t |> equal(count, 1)
        t |> equal(val, 10)
    }
    t |> run("range_sequence negative start") @(t : T?) {
        var query = range_sequence(-3, 5)
        var expected = [-3, -2, -1, 0, 1]
        for (q, e in query, expected) {
            t |> equal(q, e)
        }
    }
}

// ============================================================================
// Additional coverage: empty sequences for aggregation functions
// ============================================================================

[test]
def test_aggregation_empty(t : T?) {
    t |> run("count empty") @(t : T?) {
        var c = count(empty(type<int>))
        t |> equal(c, 0)
    }
    t |> run("long_count empty") @(t : T?) {
        var c = long_count(empty(type<int>))
        t |> equal(c, 0l)
    }
    t |> run("sum empty") @(t : T?) {
        var s = sum(empty(type<int>))
        t |> equal(s, 0)
    }
    t |> run("average from single element") @(t : T?) {
        var avg = average(
            [iterator for(x in 0..1); 42]
        )
        t |> equal(avg, 42)
    }
}

// ============================================================================
// Additional coverage: aggregate edge cases
// ============================================================================

[test]
def test_aggregate_edge_cases(t : T?) {
    t |> run("aggregate empty sequence") @(t : T?) {
        var result = aggregate(
            empty(type<int>),
            100,
            $(acc, x) => acc + x
        )
        t |> equal(result, 100)
    }
    t |> run("aggregate single element") @(t : T?) {
        var result = aggregate(
            [iterator for(x in 0..1); 42],
            0,
            $(acc, x) => acc + x
        )
        t |> equal(result, 42)
    }
}

// ============================================================================
// Additional coverage: where edge cases
// ============================================================================

[test]
def test_where_edge_cases(t : T?) {
    t |> run("where no match") @(t : T?) {
        var query = _where(
            [iterator for(x in 0..5); x],
            _ > 100
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("where all match") @(t : T?) {
        var query = _where(
            [iterator for(x in 0..5); x],
            _ >= 0
        )
        var count = 0
        for (_ in query) {
            count ++
        }
        t |> equal(count, 5)
    }
    t |> run("where empty sequence") @(t : T?) {
        var query = _where(
            empty(type<int>),
            _ > 0
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
}

// ============================================================================
// Additional coverage: take/skip edge cases
// ============================================================================

[test]
def test_take_skip_edge_cases(t : T?) {
    t |> run("take zero") @(t : T?) {
        var query = take(
            [iterator for(x in 0..5); x],
            0
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("take more than available") @(t : T?) {
        var query = take(
            [iterator for(x in 0..3); x],
            100
        )
        var count = 0
        for (_ in query) {
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("skip all") @(t : T?) {
        var query = skip(
            [iterator for(x in 0..5); x],
            5
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("skip more than available") @(t : T?) {
        var query = skip(
            [iterator for(x in 0..3); x],
            100
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("skip zero") @(t : T?) {
        var query = skip(
            [iterator for(x in 0..5); x],
            0
        )
        var count = 0
        for (_ in query) {
            count ++
        }
        t |> equal(count, 5)
    }
}

// ============================================================================
// Additional coverage: element_at edge cases
// ============================================================================

[test]
def test_element_at_edge_cases(t : T?) {
    t |> run("element_at first") @(t : T?) {
        var result = element_at(
            [iterator for(x in 0..5); x * 10],
            0
        )
        t |> equal(result, 0)
    }
    t |> run("element_at last") @(t : T?) {
        var result = element_at(
            [iterator for(x in 0..5); x * 10],
            4
        )
        t |> equal(result, 40)
    }
    t |> run("element_at_or_default out of range") @(t : T?) {
        var result = element_at_or_default(
            [iterator for(x in 0..5); x],
            100
        )
        t |> equal(result, 0) // default int value
    }
    t |> run("element_at_or_default negative index") @(t : T?) {
        var result = element_at_or_default(
            [iterator for(x in 0..5); x],
            -1
        )
        t |> equal(result, 0)
    }
}

// ============================================================================
// Additional coverage: concat edge cases
// ============================================================================

[test]
def test_concat_edge_cases(t : T?) {
    t |> run("concat empty and non-empty") @(t : T?) {
        var query = concat(
            empty(type<int>),
            [iterator for(x in 0..3); x]
        )
        var count = 0
        for (q, e in query, 0..3) {
            t |> equal(q, e)
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("concat non-empty and empty") @(t : T?) {
        var query = concat(
            [iterator for(x in 0..3); x],
            empty(type<int>)
        )
        var count = 0
        for (q, e in query, 0..3) {
            t |> equal(q, e)
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("concat both empty") @(t : T?) {
        var query = concat(
            empty(type<int>),
            empty(type<int>)
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
}

// ============================================================================
// Additional coverage: distinct edge cases
// ============================================================================

[test]
def test_distinct_edge_cases(t : T?) {
    t |> run("distinct all same") @(t : T?) {
        var arr = [5, 5, 5, 5, 5]
        var result = distinct(arr)
        t |> equal(length(result), 1)
        t |> equal(result[0], 5)
    }
    t |> run("distinct single element") @(t : T?) {
        var arr = [42]
        var result = distinct(arr)
        t |> equal(length(result), 1)
        t |> equal(result[0], 42)
    }
    t |> run("distinct_by all same key") @(t : T?) {
        var arr = [1, 11, 21, 31]
        // key: _ % 10 → all have key 1
        var result = distinct_by(arr, $(_ : int) => _ % 10)
        t |> equal(length(result), 1)
        t |> equal(result[0], 1) // first element kept
    }
}

// ============================================================================
// Additional coverage: union edge cases
// ============================================================================

[test]
def test_union_edge_cases(t : T?) {
    t |> run("union with empty first") @(t : T?) {
        var result = union(
            empty(type<int>),
            [iterator for(x in 0..3); x]
        )
        var count = 0
        for (q, e in result, 0..3) {
            t |> equal(q, e)
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("union with empty second") @(t : T?) {
        var result = union(
            [iterator for(x in 0..3); x],
            empty(type<int>)
        )
        var count = 0
        for (q, e in result, 0..3) {
            t |> equal(q, e)
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("union identical sets") @(t : T?) {
        var result = union(
            [iterator for(x in 0..3); x],
            [iterator for(x in 0..3); x]
        )
        var expected = [0, 1, 2]
        var count = 0
        for (q, e in result, expected) {
            t |> equal(q, e)
            count ++
        }
        t |> equal(count, 3)
    }
}

// ============================================================================
// Additional coverage: repeat and default_empty edge cases
// ============================================================================

[test]
def test_repeat_edge_cases(t : T?) {
    t |> run("repeat zero times") @(t : T?) {
        var result = repeat(42, 0)
        var found = false
        for (_ in result) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("repeat one time") @(t : T?) {
        var result = repeat(42, 1)
        var count = 0
        var val = -1
        for (r in result) {
            val = r
            count ++
        }
        t |> equal(count, 1)
        t |> equal(val, 42)
    }
}

// ============================================================================
// Additional coverage: order edge cases
// ============================================================================

[test]
def test_order_edge_cases(t : T?) {
    t |> run("order empty") @(t : T?) {
        var result = order(
            empty(type<int>),
            $(a, b) => a < b
        )
        var found = false
        for (_ in result) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("order single element") @(t : T?) {
        var result = order(
            [iterator for(x in 0..1); 42],
            $(a, b) => a < b
        )
        var count = 0
        for (r in result) {
            t |> equal(r, 42)
            count ++
        }
        t |> equal(count, 1)
    }
    t |> run("order already sorted") @(t : T?) {
        var arr = [1, 2, 3, 4, 5]
        var result = order(
            arr.to_sequence(),
            $(a, b) => a < b
        )
        for (r, e in result, 1..6) {
            t |> equal(r, e)
        }
    }
    t |> run("order reverse sorted") @(t : T?) {
        var arr = [5, 4, 3, 2, 1]
        var result = order(
            arr.to_sequence(),
            $(a, b) => a < b
        )
        for (r, e in result, 1..6) {
            t |> equal(r, e)
        }
    }
}

// ============================================================================
// Additional coverage: reverse edge cases
// ============================================================================

[test]
def test_reverse_edge_cases(t : T?) {
    t |> run("reverse empty") @(t : T?) {
        var result = reverse(empty(type<int>))
        var found = false
        for (_ in result) {
            found = true
        }
        t |> success(!found)
    }
    t |> run("reverse single element") @(t : T?) {
        var result = reverse(
            [iterator for(x in 0..1); 42]
        )
        var count = 0
        for (r in result) {
            t |> equal(r, 42)
            count ++
        }
        t |> equal(count, 1)
    }
    t |> run("reverse_inplace empty") @(t : T?) {
        var arr : array<int>
        reverse_inplace(arr)
        t |> equal(length(arr), 0)
    }
}

// ============================================================================
// Additional coverage: zip edge cases
// ============================================================================

[test]
def test_zip_edge_cases(t : T?) {
    t |> run("zip different lengths") @(t : T?) {
        var result = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 0..5); x * 10]
        )
        var count = 0
        for (r in result) {
            t |> equal(r._0, count)
            t |> equal(r._1, count * 10)
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("zip with empty first") @(t : T?) {
        var result = zip(
            empty(type<int>),
            [iterator for(x in 0..5); x]
        )
        var found = false
        for (_ in result) {
            found = true
        }
        t |> success(!found)
    }
}

// ============================================================================
// Additional coverage: chunk edge cases
// ============================================================================

[test]
def test_chunk_edge_cases(t : T?) {
    t |> run("chunk with fewer elements than chunk size") @(t : T?) {
        var query = chunk(
            [iterator for(x in 0..2); x],
            5
        )
        var chunk_count = 0
        for (c in query) {
            t |> equal(length(c), 2)
            t |> equal(c[0], 0)
            t |> equal(c[1], 1)
            chunk_count ++
        }
        t |> equal(chunk_count, 1)
    }
    t |> run("chunk exact division") @(t : T?) {
        var query = chunk(
            [iterator for(x in 0..6); x],
            3
        )
        var chunk_count = 0
        for (c in query) {
            t |> equal(length(c), 3)
            chunk_count ++
        }
        t |> equal(chunk_count, 2)
    }
}

// ============================================================================
// Additional coverage: first/last with empty sequences
// ============================================================================

[test]
def test_first_last_defaults(t : T?) {
    t |> run("first_or_default from empty") @(t : T?) {
        var result = first_or_default(
            empty(type<int>),
            -999
        )
        t |> equal(result, -999)
    }
    t |> run("last_or_default from empty") @(t : T?) {
        var result = last_or_default(
            empty(type<int>),
            -999
        )
        t |> equal(result, -999)
    }
    t |> run("single_or_default from empty") @(t : T?) {
        var result = single_or_default(
            empty(type<int>),
            -999
        )
        t |> equal(result, -999)
    }
    t |> run("single_or_default from multiple") @(t : T?) {
        var result = single_or_default(
            [iterator for(x in 0..5); x],
            -999
        )
        t |> equal(result, -999)
    }
}

// ============================================================================
// Additional coverage: to_table edge cases
// ============================================================================

[test]
def test_to_table_edge_cases(t : T?) {
    t |> run("to_table empty") @(t : T?) {
        var result = to_table(
            empty(type<int>),
            $(x : int) => x,
            $(x : int) => x * 10
        )
        t |> equal(length(result), 0)
    }
    t |> run("to_table single element") @(t : T?) {
        var result = to_table(
            [iterator for(x in 0..1); 42],
            $(x : int) => x,
            $(x : int) => x * 10
        )
        t |> equal(length(result), 1)
        let found = result.get(unique_key(42), $(v) {
            t |> equal(v, 420)
        })
        t |> success(found)
    }
}

// ============================================================================
// Additional coverage: select_many edge cases
// ============================================================================

[test]
def test_select_many_edge_cases(t : T?) {
    t |> run("select_many empty outer") @(t : T?) {
        var seq_seq : array<iterator<int>>
        var query = select_many(
            seq_seq.to_sequence(),
            $(x : int) => x * 2
        )
        var found = false
        for (_ in query) {
            found = true
        }
        t |> success(!found)
    }
}

// ============================================================================
// Additional coverage: contains edge cases
// ============================================================================

[test]
def test_contains_edge_cases(t : T?) {
    t |> run("contains in empty") @(t : T?) {
        var result = contains(empty(type<int>), 42)
        t |> success(!result)
    }
    t |> run("contains first element") @(t : T?) {
        var result = contains(
            [iterator for(x in 0..5); x],
            0
        )
        t |> success(result)
    }
    t |> run("contains last element") @(t : T?) {
        var result = contains(
            [iterator for(x in 0..5); x],
            4
        )
        t |> success(result)
    }
    t |> run("contains missing element") @(t : T?) {
        var result = contains(
            [iterator for(x in 0..5); x],
            99
        )
        t |> success(!result)
    }
}

// ============================================================================
// Additional coverage: any/all edge cases
// ============================================================================

[test]
def test_any_all_edge_cases(t : T?) {
    t |> run("any with predicate on empty") @(t : T?) {
        var result = _any(empty(type<int>), _ > 0)
        t |> success(!result)
    }
    t |> run("all on empty is true") @(t : T?) {
        var result = _all(empty(type<int>), _ > 0)
        t |> success(result) // vacuous truth
    }
    t |> run("all with failing predicate") @(t : T?) {
        var result = _all(
            [iterator for(x in 0..5); x],
            _ < 3
        )
        t |> success(!result)
    }
    t |> run("any with all matching") @(t : T?) {
        var result = _any(
            [iterator for(x in 0..5); x],
            _ >= 0
        )
        t |> success(result)
    }
}

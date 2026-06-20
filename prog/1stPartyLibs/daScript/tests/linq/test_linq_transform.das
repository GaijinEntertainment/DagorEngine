options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost
require math


require _common

[test]
def test_select_transform(t : T?) {
    t |> run("basic select_transform") @(t : T?) {
        let chars = ["a", "b", "c", "d", "e"]
        var query = _select(
            chars.to_sequence(),
            (_, _ + _)
        )
        for (ch, q in chars, query) {
            t |> equal(ch, q._0)
            t |> equal(ch + ch, q._1)
        }
    }
    t |> run("complex select_transform") @(t : T?) {
        var qcomplex = _select(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            (_.a[0], _.a[1])
        )
        for (i, q in 0..5, qcomplex) {
            t |> equal(q._0, i)
            t |> equal(q._1, i * 10)
        }
    }
    t |> run("select_transform to array") @(t : T?) {
        let chars = ["a", "b", "c", "d", "e"]
        var query = _select_to_array(
            chars.to_sequence(),
            (_, _ + _)
        )
        for (ch, q in chars, query) {
            t |> equal(ch, q._0)
            t |> equal(ch + ch, q._1)
        }
    }
}

[test]
def test_select(t : T?) {
    t |> run("basic select") @(t : T?) {
        var chars = ["a", "b", "c", "d", "e"]
        var query = select(chars.to_sequence())
        for (i, ch, q in 0..5, chars, query) {
            t |> equal(i, q._0)
            t |> equal(ch, q._1)
        }
    }
    t |> run("complex select") @(t : T?) {
        var qcomplex = select(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        )
        for (i, q in 0..5, qcomplex) {
            t |> equal(q._0, i)
            t |> equal(q._1.a[0], i)
            t |> equal(q._1.a[1], i * 10)
        }
    }
    t |> run("select to array") @(t : T?) {
        var chars = ["a", "b", "c", "d", "e"]
        var query = select(chars.to_sequence()).to_array()
        for (i, ch, q in 0..5, chars, query) {
            t |> equal(i, q._0)
            t |> equal(ch, q._1)
        }
    }
    t |> run("complex select to array") @(t : T?) {
        var qcomplex = select(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])]
        ).to_array()
        for (i, q in 0..5, qcomplex) {
            t |> equal(q._0, i)
            t |> equal(q._1.a[0], i)
            t |> equal(q._1.a[1], i * 10)
        }
    }
    t |> run("select from array") @(t : T?) {
        let nums = [10, 20, 30, 40, 50]
        var query = select(nums)
        for (i, q in 0..5, query) {
            t |> equal(q._0, i)
            t |> equal(q._1, (i + 1) * 10)
        }
    }
    t |> run("select from array with result selector") @(t : T?) {
        let nums = [10, 20, 30]
        var query = _select(
            nums,
            (_, _ * 2)
        )
        for (i, q in 0..3, query) {
            t |> equal(q._0, (i + 1) * 10)
            t |> equal(q._1, (i + 1) * 20)
        }
    }
    t |> run("select to sequence") @(t : T?) {
        var chars = ["a", "b", "c", "d", "e"]
        var query = select(chars).to_sequence()
        for (i, ch, q in 0..5, chars, query) {
            t |> equal(i, q._0)
            t |> equal(ch, q._1)
        }
    }
    t |> run("complex select to sequence") @(t : T?) {
        var qcomplex = select(
            [for (x in 0..5); ComplexType(a = [x, x * 10])]
        ).to_sequence()
        for (i, q in 0..5, qcomplex) {
            t |> equal(q._0, i)
            t |> equal(q._1.a[0], i)
            t |> equal(q._1.a[1], i * 10)
        }
    }
}

[test]
def test_selectmany(t : T?) {
    t |> run("basic selectmany") @(t : T?) {
        var seq_seq = [
            ["a", "b", "c"].to_sequence(),
            ["d", "e", "f"].to_sequence(),
            ["g", "h", "i"].to_sequence()
        ]
        var query = select_many(
            seq_seq.to_sequence(),
            $(s : string) => "{s}_{s}"
        )
        var expected = [
            "a_a", "b_b", "c_c",
            "d_d", "e_e", "f_f",
            "g_g", "h_h", "i_i"
        ]
        for (i, q in expected, query) {
            t |> equal(i, q)
        }
    }
    t |> run("complex selectmany") @(t : T?) {
        var seq_seq = [
            [ComplexType(a = [1, 10]), ComplexType(a = [2, 20])].to_sequence(),
            [ComplexType(a = [3, 30]), ComplexType(a = [4, 40])].to_sequence(),
            [ComplexType(a = [5, 50]), ComplexType(a = [6, 60])].to_sequence()
        ]
        var query = select_many(
            seq_seq.to_sequence(),
            $(c : ComplexType) => (c.a[0], c.a[1])
        )
        var expected = [
            (1, 10), (2, 20),
            (3, 30), (4, 40),
            (5, 50), (6, 60)
        ]
        for (i, q in expected, query) {
            t |> equal(i._0, q._0)
            t |> equal(i._1, q._1)
        }
    }
    t |> run("selectmany to array") @(t : T?) {
        var seq_seq = [
            ["a", "b", "c"].to_sequence(),
            ["d", "e", "f"].to_sequence(),
            ["g", "h", "i"].to_sequence()
        ]
        var query = select_many_to_array(
            seq_seq.to_sequence(),
            $(s : string) => "{s}_{s}"
        )
        static_assert(typeinfo is_array(query))
        var expected = [
            "a_a", "b_b", "c_c",
            "d_d", "e_e", "f_f",
            "g_g", "h_h", "i_i"
        ]
        for (i, q in expected, query) {
            t |> equal(i, q)
        }
    }
    t |> run("selectmany from array") @(t : T?) {
        let seq_seq = [
            ["a", "b", "c"],
            ["d", "e", "f"],
            ["g", "h", "i"]
        ]
        var query = select_many(
            seq_seq,
            $(s : string) => "{s}_{s}"
        )
        var expected = [
            "a_a", "b_b", "c_c",
            "d_d", "e_e", "f_f",
            "g_g", "h_h", "i_i"
        ]
        for (i, q in expected, query) {
            t |> equal(i, q)
        }
    }
}

[test]
def test_selectmany_collsel(t : T?) {
    t |> run("basic selectmany with collection selector") @(t : T?) {
        var chars = ["a", "b", "c", "d", "e"]
        var query = select_many(
            chars.to_sequence(),
            $(s : string) => [s, s + s].to_sequence(),
            $(s : string) => "{s}_{s}"
        )
        var expected = [
            "a_a", "aa_aa",
            "b_b", "bb_bb",
            "c_c", "cc_cc",
            "d_d", "dd_dd",
            "e_e", "ee_ee"
        ]
        for (i, q in expected, query) {
            t |> equal(i, q)
        }
    }
    t |> run("complex selectmany with collection selector") @(t : T?) {
        var seq_seq = [
            ComplexType(a = [1, 10]),
            ComplexType(a = [2, 20]),
            ComplexType(a = [3, 30])
        ]
        var query = select_many(
            seq_seq.to_sequence(),
            $(c : ComplexType) => [
                ComplexType(a = [c.a[0], c.a[1]]),
                ComplexType(a = [c.a[0] * 10, c.a[1] * 10])
            ].to_sequence(),
            $(c : ComplexType) => (c.a[0], c.a[1])
        )
        var expected = [
            (1, 10), (10, 100),
            (2, 20), (20, 200),
            (3, 30), (30, 300)
        ]
        for (i, q in expected, query) {
            t |> equal(i._0, q._0)
            t |> equal(i._1, q._1)
        }
    }
}

[test]
def test_zip(t : T?) {
    t |> run("basic zip") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..15); x]
        )
        var expected = [
            (0, 10),
            (1, 11),
            (2, 12),
            (3, 13),
            (4, 14),
            (5, 15)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("zip to array") @(t : T?) {
        var query = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..15); x]
        )
        static_assert(typeinfo is_array(query))
        var expected = [
            (0, 10),
            (1, 11),
            (2, 12),
            (3, 13),
            (4, 14),
            (5, 15)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("complex zip") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..15); x * 2],
            $(x, y : int) => (x, y)
        )
        var expected_complex = [
            (0, 20),
            (1, 22),
            (2, 24),
            (3, 26),
            (4, 28)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("complex zip with complex types") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 10..15); ComplexType(a = [x, x * 20])],
            $(x, y : ComplexType) => (x.a[0], x.a[1], y.a[0], y.a[1])
        )
        var expected_complex = [
            (0, 0,  10, 200),
            (1, 10, 11, 220),
            (2, 20, 12, 240),
            (3, 30, 13, 260),
            (4, 40, 14, 280)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
    }
    t |> run("complex zip to array") @(t : T?) {
        var qcomplex = zip_to_array(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 10..15); ComplexType(a = [x, x * 20])],
            $(x, y : ComplexType) => (x.a[0], x.a[1], y.a[0], y.a[1])
        )
        static_assert(typeinfo is_array(qcomplex))
        var expected_complex = [
            (0, 0,  10, 200),
            (1, 10, 11, 220),
            (2, 20, 12, 240),
            (3, 30, 13, 260),
            (4, 40, 14, 280)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
    }
}

[test]
def test_zip3(t : T?) {
    t |> run("basic zip 3 iterators") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x]
        )
        var idx = 0
        for (c in query) {
            t |> equal(c._0, idx)
            t |> equal(c._1, idx + 10)
            t |> equal(c._2, idx + 100)
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 3 arrays") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        var result = zip(a, b, c)
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._1, i + 10)
            t |> equal(r._2, i + 100)
        }
    }
    t |> run("zip 3 to array") @(t : T?) {
        var result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x]
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._1, i + 10)
            t |> equal(r._2, i + 100)
        }
    }
    t |> run("zip 3 with unequal lengths") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
}

[test]
def test_zip4(t : T?) {
    t |> run("basic zip 4 iterators") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x]
        )
        var idx = 0
        for (c in query) {
            t |> equal(c._0, idx)
            t |> equal(c._1, idx + 10)
            t |> equal(c._2, idx + 100)
            t |> equal(c._3, idx + 1000)
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 4 arrays") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        var result = zip(a, b, c, d)
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._1, i + 10)
            t |> equal(r._2, i + 100)
            t |> equal(r._3, i + 1000)
        }
    }
    t |> run("zip 4 to array") @(t : T?) {
        var result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x]
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._1, i + 10)
            t |> equal(r._2, i + 100)
            t |> equal(r._3, i + 1000)
        }
    }
    t |> run("zip 4 with unequal lengths") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x],
            [iterator for(x in 1000..1006); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
    t |> run("zip 4 with result selector") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            $(p : int; q : int; r : int; s : int) => p + q + r + s
        )
        let expected = [1110, 1114, 1118]
        var idx = 0
        for (sum in qcomplex) {
            t |> equal(sum, expected[idx])
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 4 arrays with result selector") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let result = zip(a, b, c, d, $(aa : int; bb : int; cc : int; dd : int) => aa + bb + cc + dd)
        t |> equal(3, length(result))
        t |> equal(result[0], 1110)
        t |> equal(result[1], 1114)
        t |> equal(result[2], 1118)
    }
    t |> run("zip 4 to array with result selector") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            $(aa : int; bb : int; cc : int; dd : int) => aa + bb + cc + dd
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        t |> equal(result[0], 1110)
        t |> equal(result[1], 1114)
        t |> equal(result[2], 1118)
    }
}

[test]
def test_zip5(t : T?) {
    t |> run("basic zip 5 iterators") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x]
        )
        var idx = 0
        for (c in query) {
            t |> equal(c._0, idx)
            t |> equal(c._1, idx + 10)
            t |> equal(c._2, idx + 100)
            t |> equal(c._3, idx + 1000)
            t |> equal(c._4, idx + 10000)
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 5 arrays") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let result = zip(a, b, c, d, e)
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._1, i + 10)
            t |> equal(r._2, i + 100)
            t |> equal(r._3, i + 1000)
            t |> equal(r._4, i + 10000)
        }
    }
    t |> run("zip 5 to array") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x]
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
    }
    t |> run("zip 5 with unequal lengths") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x],
            [iterator for(x in 1000..1006); x],
            [iterator for(x in 10000..10003); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
    t |> run("zip 5 with result selector") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int) => aa + bb + cc + dd + ee
        )
        let expected = [11110, 11115, 11120]
        var idx = 0
        for (sum in qcomplex) {
            t |> equal(sum, expected[idx])
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 5 arrays with result selector") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let result = zip(a, b, c, d, e, $(aa : int; bb : int; cc : int; dd : int; ee : int) => aa + bb + cc + dd + ee)
        t |> equal(3, length(result))
        t |> equal(result[0], 11110)
        t |> equal(result[1], 11115)
        t |> equal(result[2], 11120)
    }
    t |> run("zip 5 to array with result selector") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int) => aa + bb + cc + dd + ee
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        t |> equal(result[0], 11110)
        t |> equal(result[1], 11115)
        t |> equal(result[2], 11120)
    }
}

[test]
def test_zip6(t : T?) {
    t |> run("basic zip 6 iterators") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x]
        )
        var idx = 0
        for (c in query) {
            t |> equal(c._0, idx)
            t |> equal(c._1, idx + 10)
            t |> equal(c._2, idx + 100)
            t |> equal(c._3, idx + 1000)
            t |> equal(c._4, idx + 10000)
            t |> equal(c._5, idx + 100000)
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 6 arrays") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let f = [100000, 100001, 100002]
        let result = zip(a, b, c, d, e, f)
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._1, i + 10)
            t |> equal(r._2, i + 100)
            t |> equal(r._3, i + 1000)
            t |> equal(r._4, i + 10000)
            t |> equal(r._5, i + 100000)
        }
    }
    t |> run("zip 6 to array") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x]
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
    }
    t |> run("zip 6 with unequal lengths") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x],
            [iterator for(x in 1000..1006); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100007); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
    t |> run("zip 6 with result selector") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int) => aa + bb + cc + dd + ee + ff
        )
        let expected = [111110, 111116, 111122]
        var idx = 0
        for (sum in qcomplex) {
            t |> equal(sum, expected[idx])
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 6 arrays with result selector") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let f = [100000, 100001, 100002]
        let result = zip(a, b, c, d, e, f, $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int) => aa + bb + cc + dd + ee + ff)
        t |> equal(3, length(result))
        t |> equal(result[0], 111110)
        t |> equal(result[1], 111116)
        t |> equal(result[2], 111122)
    }
    t |> run("zip 6 to array with result selector") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int) => aa + bb + cc + dd + ee + ff
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        t |> equal(result[0], 111110)
        t |> equal(result[1], 111116)
        t |> equal(result[2], 111122)
    }
}

[test]
def test_zip7(t : T?) {
    t |> run("basic zip 7 iterators") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x]
        )
        var idx = 0
        for (c in query) {
            t |> equal(c._0, idx)
            t |> equal(c._1, idx + 10)
            t |> equal(c._2, idx + 100)
            t |> equal(c._3, idx + 1000)
            t |> equal(c._4, idx + 10000)
            t |> equal(c._5, idx + 100000)
            t |> equal(c._6, idx + 1000000)
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 7 arrays") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let f = [100000, 100001, 100002]
        let g = [1000000, 1000001, 1000002]
        let result = zip(a, b, c, d, e, f, g)
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._6, i + 1000000)
        }
    }
    t |> run("zip 7 to array") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x]
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
    }
    t |> run("zip 7 with unequal lengths") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x],
            [iterator for(x in 1000..1006); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100007); x],
            [iterator for(x in 1000000..1000004); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
    t |> run("zip 7 with result selector") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int; gg : int) => aa + bb + cc + dd + ee + ff + gg
        )
        let expected = [1111110, 1111117, 1111124]
        var idx = 0
        for (sum in qcomplex) {
            t |> equal(sum, expected[idx])
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 7 arrays with result selector") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let f = [100000, 100001, 100002]
        let g = [1000000, 1000001, 1000002]
        let result = zip(a, b, c, d, e, f, g, $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int; gg : int) => aa + bb + cc + dd + ee + ff + gg)
        t |> equal(3, length(result))
        t |> equal(result[0], 1111110)
        t |> equal(result[1], 1111117)
        t |> equal(result[2], 1111124)
    }
    t |> run("zip 7 to array with result selector") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int; gg : int) => aa + bb + cc + dd + ee + ff + gg
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        t |> equal(result[0], 1111110)
        t |> equal(result[1], 1111117)
        t |> equal(result[2], 1111124)
    }
}

[test]
def test_zip8(t : T?) {
    t |> run("basic zip 8 iterators") @(t : T?) {
        var query = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x],
            [iterator for(x in 10000000..10000003); x]
        )
        var idx = 0
        for (c in query) {
            t |> equal(c._0, idx)
            t |> equal(c._1, idx + 10)
            t |> equal(c._2, idx + 100)
            t |> equal(c._3, idx + 1000)
            t |> equal(c._4, idx + 10000)
            t |> equal(c._5, idx + 100000)
            t |> equal(c._6, idx + 1000000)
            t |> equal(c._7, idx + 10000000)
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 8 arrays") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let f = [100000, 100001, 100002]
        let g = [1000000, 1000001, 1000002]
        let h = [10000000, 10000001, 10000002]
        let result = zip(a, b, c, d, e, f, g, h)
        t |> equal(3, length(result))
        for (r, i in result, 0..3) {
            t |> equal(r._0, i)
            t |> equal(r._7, i + 10000000)
        }
    }
    t |> run("zip 8 to array") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x],
            [iterator for(x in 10000000..10000003); x]
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
    }
    t |> run("zip 8 with unequal lengths") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x],
            [iterator for(x in 1000..1006); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100007); x],
            [iterator for(x in 1000000..1000004); x],
            [iterator for(x in 10000000..10000005); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
    t |> run("zip 8 with result selector") @(t : T?) {
        var qcomplex = zip(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x],
            [iterator for(x in 10000000..10000003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int; gg : int; hh : int) => aa + bb + cc + dd + ee + ff + gg + hh
        )
        let expected = [11111110, 11111118, 11111126]
        var idx = 0
        for (sum in qcomplex) {
            t |> equal(sum, expected[idx])
            idx ++
        }
        t |> equal(idx, 3)
    }
    t |> run("zip 8 arrays with result selector") @(t : T?) {
        let a = [0, 1, 2]
        let b = [10, 11, 12]
        let c = [100, 101, 102]
        let d = [1000, 1001, 1002]
        let e = [10000, 10001, 10002]
        let f = [100000, 100001, 100002]
        let g = [1000000, 1000001, 1000002]
        let h = [10000000, 10000001, 10000002]
        let result = zip(a, b, c, d, e, f, g, h, $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int; gg : int; hh : int) => aa + bb + cc + dd + ee + ff + gg + hh)
        t |> equal(3, length(result))
        t |> equal(result[0], 11111110)
        t |> equal(result[1], 11111118)
        t |> equal(result[2], 11111126)
    }
    t |> run("zip 8 to array with result selector") @(t : T?) {
        let result = zip_to_array(
            [iterator for(x in 0..3); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..103); x],
            [iterator for(x in 1000..1003); x],
            [iterator for(x in 10000..10003); x],
            [iterator for(x in 100000..100003); x],
            [iterator for(x in 1000000..1000003); x],
            [iterator for(x in 10000000..10000003); x],
            $(aa : int; bb : int; cc : int; dd : int; ee : int; ff : int; gg : int; hh : int) => aa + bb + cc + dd + ee + ff + gg + hh
        )
        static_assert(typeinfo is_array(result))
        t |> equal(3, length(result))
        t |> equal(result[0], 11111110)
        t |> equal(result[1], 11111118)
        t |> equal(result[2], 11111126)
    }
}

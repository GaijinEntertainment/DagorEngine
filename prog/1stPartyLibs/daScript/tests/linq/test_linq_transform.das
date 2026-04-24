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
        var chars = ["a", "b", "c", "d", "e"]
        var query = _select(
            chars.to_sequence(),
            (_, _ + _)
        )
        for (i, ch, q in 0..5, chars, query) {
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
        var chars = ["a", "b", "c", "d", "e"]
        var query = _select_to_array(
            chars.to_sequence(),
            (_, _ + _)
        )
        for (i, ch, q in 0..5, chars, query) {
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
        var nums = [10, 20, 30, 40, 50]
        var query = select(nums)
        for (i, q in 0..5, query) {
            t |> equal(q._0, i)
            t |> equal(q._1, (i + 1) * 10)
        }
    }
    t |> run("select from array with result selector") @(t : T?) {
        var nums = [10, 20, 30]
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
        var seq_seq = [
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
        var a = [0, 1, 2]
        var b = [10, 11, 12]
        var c = [100, 101, 102]
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
        var result = zip_to_array(
            [iterator for(x in 0..5); x],
            [iterator for(x in 10..13); x],
            [iterator for(x in 100..104); x]
        )
        t |> equal(3, length(result))  // stops at shortest
    }
}

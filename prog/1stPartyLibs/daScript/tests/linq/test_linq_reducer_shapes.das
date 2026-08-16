options gen2
require daslib/linq
require dastest/testing_boost public

require daslib/linq_boost

// Reducer-shape coverage (LINQ_TO_TABLE.md "Late stage" items 1+2):
// selector overloads sum/min/max/average(src, selector), untyped lambda params
// on the group-bucket surface, and identity-lambda canonicalization.

[test]
def test_selector_reducers_tier2(t : T?) {
    t |> run("max with selector over array returns projected max") @(tt : T?) {
        tt |> equal(6, [3, 1, 2] |> max(@(v : int) => v * 2))
    }
    t |> run("min with selector over array") @(tt : T?) {
        tt |> equal(2, [3, 1, 2] |> min(@(v : int) => v * 2))
    }
    t |> run("sum with selector over array") @(tt : T?) {
        tt |> equal(9, [3, 1, 2] |> sum(@(v : int) => v + 1))
    }
    t |> run("average with selector over array returns double") @(tt : T?) {
        tt |> equal(4.0lf, [1, 2, 3] |> average(@(v : int) => v * 2))
    }
    t |> run("selector reducers over iterator sources") @(tt : T?) {
        let arr312 = [3, 1, 2]
        let arr123 = [1, 2, 3]
        tt |> equal(6, unsafe(each(arr312)) |> max(@(v : int) => v * 2))
        tt |> equal(9, unsafe(each(arr312)) |> sum(@(v : int) => v + 1))
        tt |> equal(2, unsafe(each(arr312)) |> min(@(v : int) => v * 2))
        tt |> equal(4.0lf, unsafe(each(arr123)) |> average(@(v : int) => v * 2))
    }
    t |> run("selector projecting to a different type") @(tt : T?) {
        tt |> equal(3.5lf, [3, 1, 2, 8] |> average(@(v : int) => v))
    }
}

[test]
def test_inner_select_untyped_param(t : T?) {
    t |> run("untyped inner-select lambda infers (fused bare form)") @(tt : T?) {
        let got <- _fold([10, 20, 30, 11, 21, 31, 12]._group_by(_ % 3)._select(_._1 |> select(@(x) => x * 2) |> sum))
        tt |> equal(3, length(got))
        var total = 0
        for (s in got) {
            total += s
        }
        tt |> equal(270, total)
    }
    t |> run("untyped inner-select matches typed spelling (named tuple, fused + unfused)") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12, 22]
        let foldGot  <- _fold(arr._group_by(_ % 4)._select((K = _._0, S = _._1 |> select(@(x) => x + 1) |> sum)))
        let typedRef <- (arr._group_by(_ % 4)._select((K = _._0, S = _._1 |> select(@(x : int) => x + 1) |> sum)))
        tt |> equal(length(typedRef), length(foldGot))
        var refMap : table<int; int>
        for (r in typedRef) {
            refMap[r.K] = r.S
        }
        for (f in foldGot) {
            tt |> equal(refMap[f.K], f.S)
        }
    }
    t |> run("untyped inner-select over a table source (kv bucket)") @(tt : T?) {
        var prices <- { "apple" => 10, "banana" => 21, "cherry" => 30 }
        var got <- _fold(each_kv(prices)._group_by(_.value % 2)._select(_._1 |> select(@(c) => c.value) |> sum))
        var total = 0
        for (s in got) {
            total += s
        }
        tt |> equal(61, total)
    }
}

[test]
def test_two_arg_reducers_over_bucket(t : T?) {
    t |> run("2-arg max over bucket == select-then-max spelling (fused)") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        let a <- _fold(arr._group_by(_ % 3)._select((K = _._0, M = _._1 |> max(@(v) => v * 2))))
        let b <- _fold(arr._group_by(_ % 3)._select((K = _._0, M = _._1 |> select(@(x : int) => x * 2) |> max)))
        tt |> equal(length(b), length(a))
        var refMap : table<int; int>
        for (r in b) {
            refMap[r.K] = r.M
        }
        for (f in a) {
            tt |> equal(refMap[f.K], f.M)
        }
    }
    t |> run("2-arg sum/min/average over bucket (fused bare form)") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        let sums <- _fold(arr._group_by(_ % 3)._select(_._1 |> sum(@(v) => v + 1)))
        var total = 0
        for (s in sums) {
            total += s
        }
        tt |> equal(135 + 7, total)   // sum of all elements + one per element
        let avgs <- _fold(arr._group_by(_ % 3)._select(_._1 |> average(@(v) => v * 2)))
        tt |> equal(3, length(avgs))
    }
    t |> run("2-arg reducer matches unfused parity") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12, 22]
        let foldGot <- _fold(arr._group_by(_ % 4)._select((K = _._0, M = _._1 |> max(@(v) => v + 5))))
        let refGot  <- (arr._group_by(_ % 4)._select((K = _._0, M = _._1 |> max(@(v) => v + 5))))
        tt |> equal(length(refGot), length(foldGot))
        var refMap : table<int; int>
        for (r in refGot) {
            refMap[r.K] = r.M
        }
        for (f in foldGot) {
            tt |> equal(refMap[f.K], f.M)
        }
    }
}

[test]
def test_identity_lambda_canonicalizes(t : T?) {
    t |> run("identity-lambda reducer == bare reducer (fused)") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        let a <- _fold(arr._group_by(_ % 3)._select((K = _._0, M = _._1 |> max(@(v) => v))))
        let b <- _fold(arr._group_by(_ % 3)._select((K = _._0, M = _._1 |> max())))
        tt |> equal(length(b), length(a))
        var refMap : table<int; int>
        for (r in b) {
            refMap[r.K] = r.M
        }
        for (f in a) {
            tt |> equal(refMap[f.K], f.M)
        }
    }
    t |> run("identity-lambda min and sum (fused bare form)") @(tt : T?) {
        let arr = [10, 20, 30, 11, 21, 31, 12]
        let mins  <- _fold(arr._group_by(_ % 3)._select(_._1 |> min(@(v) => v)))
        let bare  <- _fold(arr._group_by(_ % 3)._select(_._1 |> min()))
        tt |> equal(length(bare), length(mins))
        var total = 0
        let sums <- _fold(arr._group_by(_ % 3)._select(_._1 |> sum(@(v) => v)))
        for (s in sums) {
            total += s
        }
        tt |> equal(135, total)
    }
}

options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq_boost

require _common

[test]
def test_group_by_lazy(t : T?) {
    t |> run("basic _group_by_lazy on array — primitive key") @(t : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        var groups <- nums._group_by_lazy(_ % 3)
        var counts : table<int; int>
        for (g in groups) {
            counts[g._0] = g._1 |> length
        }
        t |> equal(counts |> length, 3)
        t |> equal(counts[0], 3)   // 3, 6, 9
        t |> equal(counts[1], 4)   // 1, 4, 7, 10
        t |> equal(counts[2], 3)   // 2, 5, 8
    }
    t |> run("basic _group_by_lazy on iterator — primitive key") @(t : T?) {
        var groups <- [iterator for(x in 0..6); x * 2]._group_by_lazy(_ % 3)
        var counts : table<int; int>
        for (g in groups) {
            counts[g._0] = g._1 |> length
        }
        t |> equal(counts |> length, 3)
        t |> equal(counts[0], 2)   // 0, 6
        t |> equal(counts[1], 2)   // 4, 10
        t |> equal(counts[2], 2)   // 2, 8
    }
    t |> run("_group_by_lazy on user struct — group people by name") @(t : T?) {
        var groups <- people._group_by_lazy(_.name)
        var by_name : table<string; int>
        for (g in groups) {
            by_name[g._0] = g._1 |> length
        }
        t |> equal(by_name |> length, 3)
        t |> equal(by_name["Alice"], 2)
        t |> equal(by_name["Bob"], 2)
        t |> equal(by_name["Charlie"], 1)
    }
    t |> run("_group_by_lazy bucket members are accessible") @(t : T?) {
        var groups <- people._group_by_lazy(_.name)
        var alice_ages : array<int>
        alice_ages |> reserve(2)
        for (g in groups) {
            if (g._0 == "Alice") {
                for (p in g._1) {
                    alice_ages |> push(p.age)
                }
            }
        }
        alice_ages |> sort
        t |> equal(alice_ages |> length, 2)
        t |> equal(alice_ages[0], 25)
        t |> equal(alice_ages[1], 30)
    }
    t |> run("_group_by_lazy on empty array yields no groups") @(t : T?) {
        let nums : array<int>
        var groups <- nums._group_by_lazy(_)
        var n = 0
        for (_g in groups) {
            n ++
        }
        t |> equal(n, 0)
    }
    t |> run("_group_by_lazy_to_array on iterator yields plain array") @(t : T?) {
        var groups <- [iterator for(x in 0..6); x]._group_by_lazy_to_array(_ % 2)
        t |> equal(groups |> length, 2)
        var sizes : array<int>
        sizes |> reserve(2)
        for (g in groups) {
            sizes |> push(g._1 |> length)
        }
        sizes |> sort
        t |> equal(sizes[0], 3)
        t |> equal(sizes[1], 3)
    }
    t |> run("_group_by_lazy on non-workhorse (struct) key — group pets by owner") @(t : T?) {
        // Regression: table is keyed on _::unique_key(K) (always workhorse) so
        // non-workhorse user keys still group correctly; original key value is
        // stored in the tuple ._0 slot. Earlier raw-key implementation only
        // compiled for workhorse keys.
        var groups <- pets._group_by_lazy(_.owner)
        var by_owner : table<string; int>
        for (g in groups) {
            by_owner[g._0.name] = g._1 |> length
        }
        t |> equal(by_owner |> length, 3)
        t |> equal(by_owner["Hedlund, Magnus"], 1)
        t |> equal(by_owner["Adams, Terry"], 2)
        t |> equal(by_owner["Weiss, Charlotte"], 1)
    }
}

[test]
def test_having(t : T?) {
    t |> run("basic _having keeps groups above threshold") @(t : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        var big_groups <- nums._group_by_lazy(_ % 3)._having(_._1 |> length >= 4)
        var seen : array<int>
        seen |> reserve(2)
        for (g in big_groups) {
            seen |> push(g._0)
        }
        t |> equal(seen |> length, 1)
        t |> equal(seen[0], 1)   // only the bucket of size 4 (1,4,7,10)
    }
    t |> run("_having drops everything below threshold") @(t : T?) {
        let nums = [1, 2, 3]
        var none_left <- nums._group_by_lazy(_)._having(_._1 |> length > 1)
        var n = 0
        for (_g in none_left) {
            n ++
        }
        t |> equal(n, 0)
    }
    t |> run("_having on user struct groups") @(t : T?) {
        var multi <- people._group_by_lazy(_.name)._having(_._1 |> length > 1)
        var seen_names : array<string>
        seen_names |> reserve(2)
        for (g in multi) {
            seen_names |> push(g._0)
        }
        seen_names |> sort
        t |> equal(seen_names |> length, 2)
        t |> equal(seen_names[0], "Alice")
        t |> equal(seen_names[1], "Bob")
    }
    t |> run("_having pre-filter chained with _where post-filter") @(t : T?) {
        // _where before group, _having after — typical SQL shape.
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        var filtered <- (nums.
            _where(_ > 2).
            _group_by_lazy(_ % 3).
            _having(_._1 |> length >= 3))
        var counts : table<int; int>
        for (g in filtered) {
            counts[g._0] = g._1 |> length
        }
        // 3,4,5,6,7,8,9,10 → buckets %3: 0=[3,6,9](3), 1=[4,7,10](3), 2=[5,8](2)
        // _having keeps only buckets with len >= 3 → buckets 0 and 1
        t |> equal(counts |> length, 2)
        t |> equal(counts[0], 3)
        t |> equal(counts[1], 3)
    }
}

[test]
def test_group_by(t : T?) {
    // _group_by is an alias for _group_by_lazy — same in-memory behavior,
    // distinct macro name so the dasSQLITE _sql translator can pattern-
    // match on intent (will-aggregate vs IGrouping). C#-style aggregate
    // access via _._0 (key) and _._1 (group elements) in the next _select.
    t |> run("_group_by produces same buckets as _group_by_lazy") @(t : T?) {
        let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        var groups <- nums._group_by(_ % 3)
        var counts : table<int; int>
        for (g in groups) {
            counts[g._0] = g._1 |> length
        }
        t |> equal(counts |> length, 3)
        t |> equal(counts[0], 3)   // 3, 6, 9
        t |> equal(counts[1], 4)   // 1, 4, 7, 10
        t |> equal(counts[2], 3)   // 2, 5, 8
    }
    t |> run("_group_by + per-group aggregate — C#-style access") @(t : T?) {
        // No SQL sugar — explicit _._1 |> length and a manual loop sum.
        // The _sql wrapper (dasSQLITE phase 1) would rewrite this chain
        // into GROUP BY ... SUM(...) SQL; in memory the chain runs as a
        // regular pipeline.
        let src = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        var groups <- src._group_by(_ % 3)
        var by_key : table<int; int>
        var totals : table<int; int>
        for (g in groups) {
            by_key[g._0] = g._1 |> length
            var s = 0
            for (e in g._1) {
                s += e
            }
            totals[g._0] = s
        }
        t |> equal(by_key[0], 3)   // 3,6,9 — count 3
        t |> equal(by_key[1], 4)   // 1,4,7,10 — count 4
        t |> equal(by_key[2], 3)   // 2,5,8 — count 3
        t |> equal(totals[0], 18)  // 3+6+9
        t |> equal(totals[1], 22)  // 1+4+7+10
        t |> equal(totals[2], 15)  // 2+5+8
    }
    t |> run("_group_by_to_array yields a plain array") @(t : T?) {
        var groups <- [iterator for(x in 0..6); x]._group_by_to_array(_ % 2)
        t |> equal(groups |> length, 2)
        var sizes : array<int>
        sizes |> reserve(2)
        for (g in groups) {
            sizes |> push(g._1 |> length)
        }
        sizes |> sort
        t |> equal(sizes[0], 3)
        t |> equal(sizes[1], 3)
    }
    t |> run("_group_by chained with _having — full SQL-shape pipeline") @(t : T?) {
        // The classic GROUP BY ... HAVING ... shape, in-memory.
        let src = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        var popular <- (src.
            _group_by(_ % 3).
            _having(_._1 |> length >= 4))
        var seen : array<int>
        seen |> reserve(1)
        for (g in popular) {
            seen |> push(g._0)
        }
        t |> equal(seen |> length, 1)
        t |> equal(seen[0], 1)   // only bucket of size 4 (1,4,7,10)
    }
}

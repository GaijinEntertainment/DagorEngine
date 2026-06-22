options gen2
options rtti
// decs `orderby` / `group` chains have no dedicated decs splice arm for these minimal shapes, so they
// fall to the tier-2 materialize path and emit a `_fold` perf advisory. Correct, just unfused — suppress
// the cosmetic warning here (a decs-fold gap, not a linq_das issue).
options _no_linq_perf_warn = true

require dastest/testing_boost public
require daslib/linq_das
require daslib/decs_boost
require daslib/linq_fold

// linq_das: C#-style `%linq! from c [: Row] in src [where <pred>] [orderby <expr> [descending]]
// ( select <proj> | group c by <key> ) [iterator] %%` reader macro. Array source (untyped) and decs
// source (`from c : CarComp in decs`) are covered here; SQL and XML live with their modules
// (tests/dasSQLITE, tests/dasPUGIXML). The range variable is spliced verbatim as the block parameter;
// a trailing `iterator` keyword yields an `iterator<T>` over the materialized result.

struct Car {
    name  : string
    price : int
    brand : string
}

def private mk_cars() : array<Car> {
    return <- [Car(name = "cheap", price = 50,  brand = "eco"),
               Car(name = "mid",   price = 150, brand = "eco"),
               Car(name = "lux",   price = 300, brand = "lux")]
}

[decs_template(prefix = "ld_")]
struct CarComp {
    name  : string
    price : int
    brand : string
}

def private seed_decs() {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.ld_name := "car{i}"
            cmp.ld_price := 100 * (i + 1)        // 100, 200, 300
            cmp.ld_brand := i < 2 ? "a" : "b"    // a, a, b
        }
    }
    commit()
}

// ===== array source =====

[test]
def test_array_where_select(t : T?) {
    let cars <- mk_cars()
    let names <- %linq! from c in cars where c.price > 100 select c.name %%
    t |> equal(length(names), 2, "where filters, select projects")
    t |> equal(names[0], "mid")
    t |> equal(names[1], "lux")
}

[test]
def test_array_multiple_where(t : T?) {
    let cars <- mk_cars()
    // C# allows several `where` clauses — each emits its own `_where` (AND-folded, source order).
    let names <- %linq! from c in cars where c.price > 100 where c.brand == "eco" select c.name %%
    t |> equal(length(names), 1, "both predicates apply: price>100 (mid,lux) AND eco (mid)")
    t |> equal(names[0], "mid")
}

[test]
def test_array_where_after_orderby(t : T?) {
    let cars <- mk_cars()
    // a `where` after the `orderby` filters the sorted sequence — same result as filtering first (total
    // order), so it emits ahead of the order op. eco → cheap,mid; sorted desc → mid,cheap.
    let names <- %linq! from c in cars orderby c.price descending where c.brand == "eco" select c.name %%
    t |> equal(length(names), 2, "where-after-orderby keeps the eco cars, sorted desc")
    t |> equal(names[0], "mid")
    t |> equal(names[1], "cheap")
}

[test]
def test_array_select_only(t : T?) {
    let cars <- mk_cars()
    let names <- %linq! from c in cars select c.name %%
    t |> equal(length(names), 3, "no where: every element projected")
    t |> equal(names[0], "cheap")
}

[test]
def test_array_identity_select(t : T?) {
    let cars <- mk_cars()
    let kept <- %linq! from c in cars where c.price >= 150 select c %%
    t |> equal(length(kept), 2, "identity select returns filtered structs")
    t |> equal(kept[0].name, "mid")
}

[test]
def test_array_numeric_projection_and_parens(t : T?) {
    let cars <- mk_cars()
    let prices <- %linq! from c in cars where (c.price > 100) && c.price < 300 select c.price %%
    t |> equal(length(prices), 1, "only mid (150)")
    t |> equal(prices[0], 150)
}

[test]
def test_array_string_literal_in_predicate(t : T?) {
    // the keyword 'select' and range var 'c' inside a string literal must NOT be touched
    let cars <- mk_cars()
    let names <- %linq! from c in cars where c.name == "lux" select c.name %%
    t |> equal(length(names), 1, "string-literal predicate matches exactly one")
    t |> equal(names[0], "lux")
}

[test]
def test_array_arbitrary_range_var_name(t : T?) {
    let cars <- mk_cars()
    let names <- %linq! from car in cars where car.price > 100 select car.name %%
    t |> equal(length(names), 2, "multi-char range var splices verbatim")
    t |> equal(names[0], "mid")
}

[test]
def test_array_inline_in_larger_expression(t : T?) {
    let cars <- mk_cars()
    let n = length(%linq! from c in cars where c.price > 100 select c.name %%)
    t |> equal(n, 2, "query result used directly as a call argument")
}

[test]
def test_array_multiline_query(t : T?) {
    let cars <- mk_cars()
    let names <- %linq!
        from c in cars
        where c.price > 100
        select c.name
    %%
    t |> equal(length(names), 2, "multi-line query body parses identically")
}

[test]
def test_comments_in_body_are_stripped(t : T?) {
    // line/block comments are blanked before parsing — a keyword or range var inside a comment must not
    // confuse the clause splitter, and the comment must not leak into the spliced chain.
    let cars <- mk_cars()
    let names <- %linq!
        from c in cars
        // this where / select / c.name inside a comment must be ignored
        where c.price > 100   /* and so must this block: select c here */
        select c.name
    %%
    t |> equal(length(names), 2, "comments stripped, clauses split correctly")
    t |> equal(names[0], "mid")
}

[test]
def test_array_iterator_output(t : T?) {
    // trailing `iterator` keyword → lazy iterator<T> (consumed here by a comprehension)
    let cars <- mk_cars()
    let got <- [for (nm in %linq! from c in cars where c.price > 100 select c.name iterator %%); nm]
    t |> equal(length(got), 2, "iterator output streams projected elements")
    t |> equal(got[0], "mid")
    t |> equal(got[1], "lux")
}

// ===== decs source (typed range variable `: CarComp` + the `decs` keyword marker) =====

[test]
def test_decs_where_select(t : T?) {
    seed_decs()
    let names <- %linq! from c : CarComp in decs where c.price > 100 select c.name %%
    t |> equal(length(names), 2, "decs: where filters, select projects")
}

[test]
def test_decs_multiple_where(t : T?) {
    seed_decs()
    // decs rows: car0(100,a), car1(200,a), car2(300,b). price>100 → car1,car2; brand a → car1.
    let names <- %linq! from c : CarComp in decs where c.price > 100 where c.brand == "a" select c.name %%
    t |> equal(length(names), 1, "decs multi-where AND-folds both predicates")
    t |> equal(names[0], "car1")
}

[test]
def test_decs_identity_select(t : T?) {
    seed_decs()
    let rows <- %linq! from c : CarComp in decs where c.price >= 200 select c %%
    t |> equal(length(rows), 2, "decs identity select returns the row tuples")
}

[test]
def test_decs_iterator_output(t : T?) {
    seed_decs()
    let names <- [for (nm in %linq! from c : CarComp in decs where c.price > 100 select c.name iterator %%); nm]
    t |> equal(length(names), 2, "decs iterator output")
}

// ===== orderby (single key, optional `descending`) =====

[test]
def test_array_orderby_asc(t : T?) {
    let cars <- mk_cars()
    let names <- %linq! from c in cars orderby c.price select c.name %%
    t |> equal(length(names), 3)
    t |> equal(names[0], "cheap")   // 50
    t |> equal(names[1], "mid")     // 150
    t |> equal(names[2], "lux")     // 300
}

[test]
def test_array_orderby_descending(t : T?) {
    let cars <- mk_cars()
    let names <- %linq! from c in cars orderby c.price descending select c.name %%
    t |> equal(names[0], "lux")
    t |> equal(names[1], "mid")
    t |> equal(names[2], "cheap")
}

[test]
def test_array_where_orderby(t : T?) {
    let cars <- mk_cars()
    let names <- %linq! from c in cars where c.price >= 150 orderby c.price descending select c.name %%
    t |> equal(length(names), 2, "where then sort")
    t |> equal(names[0], "lux")
    t |> equal(names[1], "mid")
}

[test]
def test_array_orderby_identity_select(t : T?) {
    let cars <- mk_cars()
    let rows <- %linq! from c in cars orderby c.price select c %%
    t |> equal(length(rows), 3)
    t |> equal(rows[0].name, "cheap")
    t |> equal(rows[2].name, "lux")
}

[test]
def test_array_orderby_iterator(t : T?) {
    let cars <- mk_cars()
    let got <- [for (nm in %linq! from c in cars orderby c.price descending select c.name iterator %%); nm]
    t |> equal(got[0], "lux")
    t |> equal(got[2], "cheap")
}

[test]
def test_decs_orderby(t : T?) {
    seed_decs()
    let names <- %linq! from c : CarComp in decs orderby c.price descending select c.name %%
    t |> equal(length(names), 3)
    t |> equal(names[0], "car2")   // price 300
    t |> equal(names[2], "car0")   // price 100
}

// ===== multi-key orderby (per-key direction; >1 key → _order_by_keys composite stable sort) =====

def private mk_cars_ties() : array<Car> {
    // duplicate (brand, price) pairs exercise secondary keys + stability of equal composite keys
    return <- [Car(name = "a", price = 100, brand = "eco"),
               Car(name = "b", price = 200, brand = "eco"),
               Car(name = "c", price = 100, brand = "lux"),
               Car(name = "d", price = 100, brand = "eco"),
               Car(name = "e", price = 200, brand = "lux")]
}

[test]
def test_array_orderby_multi_asc(t : T?) {
    let cars <- mk_cars_ties()
    let rows <- %linq! from c in cars orderby c.brand, c.price select c.name %%
    t |> equal(length(rows), 5)
    // brand asc (eco<lux), then price asc; equal (eco, 100) keeps input order: a before d
    t |> equal(rows[0], "a")
    t |> equal(rows[1], "d")
    t |> equal(rows[2], "b")
    t |> equal(rows[3], "c")
    t |> equal(rows[4], "e")
}

[test]
def test_array_orderby_mixed_dir(t : T?) {
    let cars <- mk_cars_ties()
    let rows <- %linq! from c in cars orderby c.brand, c.price descending select c.name %%
    // brand asc, price desc; equal (eco, 100) stable: a before d
    t |> equal(rows[0], "b")
    t |> equal(rows[1], "a")
    t |> equal(rows[2], "d")
    t |> equal(rows[3], "e")
    t |> equal(rows[4], "c")
}

[test]
def test_array_orderby_3key(t : T?) {
    // all tie on (brand, price); the 3rd key (name asc) reorders vs input order z, a, m
    let cars <- [Car(name = "z", price = 100, brand = "eco"),
                 Car(name = "a", price = 100, brand = "eco"),
                 Car(name = "m", price = 100, brand = "eco")]
    let rows <- %linq! from c in cars orderby c.brand, c.price, c.name select c.name %%
    t |> equal(rows[0], "a")
    t |> equal(rows[1], "m")
    t |> equal(rows[2], "z")
}

[test]
def test_array_orderby_keys_op_direct(t : T?) {
    // hand-written fold-op form (brand asc, price desc) — exercises _order_by_keys directly
    let cars <- mk_cars_ties()
    let rows <- _fold(each(cars)._order_by_keys((_.brand, _.price), 2u).to_array())
    t |> equal(length(rows), 5)
    t |> equal(rows[0].name, "b")
    t |> equal(rows[4].name, "c")
}

[test]
def test_decs_orderby_multi(t : T?) {
    seed_decs()
    // brand asc, price desc; decs: car0(100,a) car1(200,a) car2(300,b)
    let names <- %linq! from c : CarComp in decs orderby c.brand, c.price descending select c.name %%
    t |> equal(length(names), 3)
    t |> equal(names[0], "car1")   // brand a, price 200
    t |> equal(names[1], "car0")   // brand a, price 100
    t |> equal(names[2], "car2")   // brand b
}

// ===== group … by (terminal; IGrouping `(key, [elems])` per bucket; in-memory sources) =====

[test]
def test_array_group_by(t : T?) {
    let cars <- mk_cars()
    let groups <- %linq! from c in cars group c by c.brand %%
    t |> equal(length(groups), 2, "two brand buckets")
    var ecoN = 0
    var luxN = 0
    for (g in groups) {
        if (g._0 == "eco") { ecoN = length(g._1) }
        if (g._0 == "lux") { luxN = length(g._1) }
    }
    t |> equal(ecoN, 2, "eco bucket has cheap+mid")
    t |> equal(luxN, 1, "lux bucket has lux")
}

[test]
def test_array_group_with_where(t : T?) {
    let cars <- mk_cars()
    // price>=150 keeps mid(eco), lux(lux) → one element each
    let groups <- %linq! from c in cars where c.price >= 150 group c by c.brand %%
    t |> equal(length(groups), 2, "filtered then grouped")
    for (g in groups) { t |> equal(length(g._1), 1) }
}

[test]
def test_array_group_iterator(t : T?) {
    let cars <- mk_cars()
    var n = 0
    for (_g in %linq! from c in cars group c by c.brand iterator %%) { n++ }
    t |> equal(n, 2, "group iterator yields the buckets")
}

[test]
def test_decs_group_by(t : T?) {
    seed_decs()
    let groups <- %linq! from c : CarComp in decs group c by c.brand %%
    t |> equal(length(groups), 2, "decs group buckets a/b")
    for (g in groups) {
        if (g._0 == "a") { t |> equal(length(g._1), 2, "bucket a: car0+car1") }
        if (g._0 == "b") { t |> equal(length(g._1), 1, "bucket b: car2") }
    }
}

// ===== join … on … equals … (single inner equi-join) =====
// Select-terminal pushes both range vars into the result selector; post-join where/orderby/group carries
// `(c, d)` as a transparent identifier. A `where` before `join` filters the left source. (See linq_das.rst.)

struct BrandHQ {
    brand   : string
    country : string
}

def private mk_brands() : array<BrandHQ> {
    return <- [BrandHQ(brand = "eco", country = "USA"),
               BrandHQ(brand = "lux", country = "Italy")]
}

[test]
def test_join_select_terminal_scalar(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let names <- %linq! from c in cars join b in brands on c.brand equals b.brand select c.name %%
    t |> equal(length(names), 3, "all 3 cars match a brand HQ")
    t |> equal(names[0], "cheap")
    t |> equal(names[2], "lux")
}

[test]
def test_join_select_terminal_tuple(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let rows <- %linq! from c in cars join b in brands on c.brand equals b.brand select (Name = c.name, Country = b.country) %%
    t |> equal(length(rows), 3)
    t |> equal(rows[0].Name, "cheap")
    t |> equal(rows[0].Country, "USA")
    t |> equal(rows[2].Country, "Italy")
}

[test]
def test_join_select_terminal_whole_row(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let rows <- %linq! from c in cars join b in brands on c.brand equals b.brand select c %%
    t |> equal(length(rows), 3, "whole-row select yields the left rows")
    t |> equal(rows[1].name, "mid")
}

[test]
def test_join_pre_join_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // `where` before `join` filters cars (price>=150 → mid, lux), then joins — pushes down to SQL too.
    let rows <- %linq! from c in cars where c.price >= 150 join b in brands on c.brand equals b.brand select (Name = c.name, Country = b.country) %%
    t |> equal(length(rows), 2, "pre-join where filters the left source")
    t |> equal(rows[0].Name, "mid")
    t |> equal(rows[0].Country, "USA")
    t |> equal(rows[1].Country, "Italy")
}

[test]
def test_join_post_join_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // `where` after the join references `b` (transparent identifier) — keep USA brands.
    let names <- %linq! from c in cars join b in brands on c.brand equals b.brand where b.country == "USA" select c.name %%
    t |> equal(length(names), 2, "post-join where sees the joined row")
    t |> equal(names[0], "cheap")
    t |> equal(names[1], "mid")
}

[test]
def test_join_multiple_pre_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // two pre-join `where`s both filter cars (push to SQL, AND-folded): price>100 (mid,lux) AND eco (mid).
    let rows <- %linq! from c in cars where c.price > 100 where c.brand == "eco"
                       join b in brands on c.brand equals b.brand
                       select (Name = c.name, Country = b.country) %%
    t |> equal(length(rows), 1, "both pre-join predicates filter the left source")
    t |> equal(rows[0].Name, "mid")
    t |> equal(rows[0].Country, "USA")
}

[test]
def test_join_multiple_post_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // two post-join `where`s filter the carried (c,b) pair: price>100 (mid,lux) AND USA (cheap,mid) → mid.
    let names <- %linq! from c in cars join b in brands on c.brand equals b.brand
                        where c.price > 100 where b.country == "USA"
                        select c.name %%
    t |> equal(length(names), 1, "both post-join predicates filter the joined pair")
    t |> equal(names[0], "mid")
}

[test]
def test_join_mixed_pre_post_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // pre + post `where`s on one join: pre price>=150 (mid,lux); post USA (mid) AND price<300 (mid).
    let names <- %linq! from c in cars where c.price >= 150
                        join b in brands on c.brand equals b.brand
                        where b.country == "USA" where c.price < 300
                        select c.name %%
    t |> equal(length(names), 1, "pre-join and post-join wheres combine")
    t |> equal(names[0], "mid")
}

[test]
def test_join_orderby(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let names <- %linq! from c in cars join b in brands on c.brand equals b.brand orderby c.price descending select c.name %%
    t |> equal(names[0], "lux")
    t |> equal(names[1], "mid")
    t |> equal(names[2], "cheap")
}

[test]
def test_join_group_after(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // group the joined pairs by the right-side country (IGrouping of (c, b) pairs).
    let groups <- %linq! from c in cars join b in brands on c.brand equals b.brand group c by b.country %%
    t |> equal(length(groups), 2, "USA + Italy buckets")
    for (g in groups) {
        if (g._0 == "USA") { t |> equal(length(g._1), 2, "USA: cheap+mid") }
        if (g._0 == "Italy") { t |> equal(length(g._1), 1, "Italy: lux") }
    }
}

// ===== join … into g (group-join, C# GroupJoin / outer) =====
// `into g` binds the array of matching right rows alongside the left row; the terminal `select` reads both.
// Outer: every left row surfaces — an unmatched left pairs with an empty group. Select-terminal + a pre-join
// `where` (filters the left source); post-`into` where/orderby/group is rejected (see failed_linq_das). SQL
// sources reject (group-join is in-memory). brands has a carless "sport" to exercise the empty group.

def private mk_brands3() : array<BrandHQ> {
    return <- [BrandHQ(brand = "eco", country = "USA"),
               BrandHQ(brand = "lux", country = "Italy"),
               BrandHQ(brand = "sport", country = "Germany")]   // no car → outer empty group
}

[test]
def test_join_into_group_count(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands3()
    let rows <- %linq! from b in brands
                       join c in cars on b.brand equals c.brand into g
                       select (brand = b.brand, n = g |> length) %%
    t |> equal(length(rows), 3, "every brand surfaces, including the carless one (outer GroupJoin)")
    for (r in rows) {
        if (r.brand == "eco") { t |> equal(r.n, 2, "cheap + mid") }
        if (r.brand == "lux") { t |> equal(r.n, 1) }
        if (r.brand == "sport") { t |> equal(r.n, 0, "unmatched left → empty group") }
    }
}

[test]
def test_join_into_group_sum(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let rows <- %linq! from b in brands
                       join c in cars on b.brand equals c.brand into g
                       select (brand = b.brand, total = g |> select($(c : Car) => c.price) |> sum) %%
    t |> equal(length(rows), 2)
    for (r in rows) {
        if (r.brand == "eco") { t |> equal(r.total, 200, "cheap 50 + mid 150") }
        if (r.brand == "lux") { t |> equal(r.total, 300) }
    }
}

[test]
def test_join_into_pre_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands3()
    // pre-join `where` filters the left source (brands) before the group-join fuses.
    let rows <- %linq! from b in brands
                       where b.country != "Germany"
                       join c in cars on b.brand equals c.brand into g
                       select (brand = b.brand, n = g |> length) %%
    t |> equal(length(rows), 2, "the carless German brand is filtered out before the join")
    for (r in rows) {
        if (r.brand == "eco") { t |> equal(r.n, 2) }
        if (r.brand == "lux") { t |> equal(r.n, 1) }
    }
}

[test]
def test_join_into_iterator(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands3()
    var n = 0
    for (_r in %linq! from b in brands join c in cars on b.brand equals c.brand into g select (brand = b.brand, k = g |> length) iterator %%) { n++ }
    t |> equal(n, 3, "iterator over every brand (outer)")
}

[test]
def test_join_string_interpolation_select(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // string interpolation in a transparent-id projection references both range vars inside `{…}`.
    let labels <- %linq! from c in cars join b in brands on c.brand equals b.brand orderby c.price select "{c.name}:{b.country}" %%
    t |> equal(labels[0], "cheap:USA")
    t |> equal(labels[2], "lux:Italy")
}

[test]
def test_join_iterator_output(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    var n = 0
    for (_nm in %linq! from c in cars join b in brands on c.brand equals b.brand select c.name iterator %%) { n++ }
    t |> equal(n, 3)
}

[decs_template(prefix = "lr_")]
struct DealerComp {
    brand  : string
    region : string
}

def private seed_decs_join() {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.eid := eid
            cmp.ld_name := "car{i}"
            cmp.ld_price := 100 * (i + 1)
            cmp.ld_brand := i < 2 ? "a" : "b"
        }
    }
    create_entity() @(eid, cmp) { cmp.eid := eid; cmp.lr_brand := "a"; cmp.lr_region := "north" }
    create_entity() @(eid, cmp) { cmp.eid := eid; cmp.lr_brand := "b"; cmp.lr_region := "south" }
    commit()
}

[test]
def test_decs_join_select_terminal(t : T?) {
    seed_decs_join()
    let rows <- %linq! from c : CarComp in decs join d : DealerComp in decs on c.brand equals d.brand select (Name = c.name, Region = d.region) %%
    t |> equal(length(rows), 3, "car0,car1 (a→north), car2 (b→south)")
    var north = 0
    var south = 0
    for (r in rows) {
        if (r.Region == "north") { north++ }
        if (r.Region == "south") { south++ }
    }
    t |> equal(north, 2)
    t |> equal(south, 1)
}

// ===== cross_join engine (plain `_fold`, no reader) — the ops the uncorrelated multiple-`from` rides =====

[test]
def test_to_array_array_noop(t : T?) {
    // to_array(array<T>) is a move-through no-op (moved sqlite_linq → builtin so source-agnostic emitters
    // can append `|> to_array()` over any source). Consumes a temporary — the cross_join result the reader
    // actually feeds it.
    let r <- to_array([10, 20, 30])
    t |> equal(length(r), 3)
    t |> equal(r[0], 10)
    t |> equal(r[2], 30)
}

[test]
def test_cross_join_untyped_lambda_fold(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // UNTYPED result lambda — proves LinqCrossJoin injects (c : Car, b : BrandHQ). 3 × 2 = 6 pairs.
    let rows <- _fold(cars |> _cross_join(brands, $(c, b) => (N = c.name, Co = b.country)) |> to_array())
    t |> equal(length(rows), 6, "cartesian product of 3 cars × 2 brands")
}

// ===== multiple `from` (uncorrelated SelectMany → cross product) =====
// `from c in A from b in B …` with B independent of `c`. Same clause grammar as `join` (pre/post `where`,
// `orderby`, `select`/`group`, transparent-id carry, `iterator`) minus the `on … equals`. (See linq_das.rst.)

[test]
def test_multifrom_scalar(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let names <- %linq! from c in cars from b in brands select c.name %%
    t |> equal(length(names), 6, "3 cars × 2 brands")
}

[test]
def test_multifrom_tuple(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let rows <- %linq! from c in cars from b in brands select (Name = c.name, Country = b.country) %%
    t |> equal(length(rows), 6)
    t |> equal(rows[0].Name, "cheap")
    t |> equal(rows[0].Country, "USA")     // cross is srcA-outer / srcB-inner: cheap×eco first
    t |> equal(rows[1].Country, "Italy")   // cheap×lux second
}

[test]
def test_multifrom_whole_row(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let rows <- %linq! from c in cars from b in brands select c %%
    t |> equal(length(rows), 6, "each car appears once per brand")
    t |> equal(rows[0].name, "cheap")
    t |> equal(rows[2].name, "mid")
}

[test]
def test_multifrom_pre_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // `where` before the 2nd `from` filters cars (price>=150 → mid, lux), then crosses — pushes to SQL too.
    let names <- %linq! from c in cars where c.price >= 150 from b in brands select c.name %%
    t |> equal(length(names), 4, "2 filtered cars × 2 brands")
    t |> equal(names[0], "mid")
    t |> equal(names[3], "lux")
}

[test]
def test_multifrom_post_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // `where` after the 2nd `from` sees both range vars (transparent id). Cross-then-filter on equal brand
    // == the equi-join subset.
    let names <- %linq! from c in cars from b in brands where c.brand == b.brand select c.name %%
    t |> equal(length(names), 3)
    t |> equal(names[0], "cheap")
    t |> equal(names[1], "mid")
    t |> equal(names[2], "lux")
}

[test]
def test_multifrom_multiple_where(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // two post-`from` `where`s over the crossed pairs: brand-match (the join subset) AND price>100.
    let names <- %linq! from c in cars from b in brands
                        where c.brand == b.brand where c.price > 100
                        select c.name %%
    t |> equal(length(names), 2, "brand-match (cheap,mid,lux) AND price>100 (mid,lux)")
    t |> equal(names[0], "mid")
    t |> equal(names[1], "lux")
}

[test]
def test_multifrom_orderby(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let names <- %linq! from c in cars from b in brands orderby c.price descending select c.name %%
    t |> equal(length(names), 6)
    t |> equal(names[0], "lux")    // price 300
    t |> equal(names[5], "cheap")  // price 50
}

[test]
def test_multifrom_group_after(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // group the crossed pairs by the right-side country (IGrouping of (c, b) pairs).
    let groups <- %linq! from c in cars from b in brands group c by b.country %%
    t |> equal(length(groups), 2, "USA + Italy buckets")
    for (g in groups) {
        t |> equal(length(g._1), 3, "each country pairs with all 3 cars")
    }
}

[test]
def test_multifrom_string_interpolation(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let labels <- %linq! from c in cars from b in brands orderby c.price select "{c.name}:{b.country}" %%
    t |> equal(length(labels), 6)
    t |> equal(labels[0], "cheap:USA")   // lowest price, cheap×eco first
}

[test]
def test_multifrom_iterator_output(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    var n = 0
    for (_nm in %linq! from c in cars from b in brands select c.name iterator %%) { n++ }
    t |> equal(n, 6)
}

[test]
def test_multifrom_decs(t : T?) {
    seed_decs_join()
    // decs × decs cross — from_decs_template field-tuples + the LinqCrossJoin injection. 3 cars × 2 dealers.
    let rows <- %linq! from c : CarComp in decs from d : DealerComp in decs select (Name = c.name, Region = d.region) %%
    t |> equal(length(rows), 6, "3 cars × 2 dealers")
}

// ===== correlated multiple `from` (SelectMany flatten) =====
// `from o in O from l in o.lines …` — the second source is a per-row collection of the first, so it
// flattens (≠ the uncorrelated cross above) and emits `_select_many`. In-memory array source only: the
// outer row holds the flattened collection (non-copyable), which SQL can't model and `from_xml_node`
// can't materialize. Supported: select-terminal (both/inner), pre-`from` `where`, an inner-only post-`from`
// `where` (pushed into the collection before flattening), and `iterator`. orderby/group/outer-`where` over
// the flatten are rejected (see failed_linq_das.das) — the non-copyable outer can't ride the carry.

struct Line {
    sku : string
    qty : int
}

struct Order {
    id       : int
    customer : string
    lines    : array<Line>
}

def private mk_orders() : array<Order> {
    var o1 <- Order(id = 1, customer = "Acme", lines <- [Line(sku = "a", qty = 2), Line(sku = "b", qty = 1)])
    var o2 <- Order(id = 2, customer = "Acme", lines <- [Line(sku = "c", qty = 5)])
    var o3 <- Order(id = 3, customer = "Globex", lines <- [Line(sku = "d", qty = 3)])
    return <- [o1, o2, o3]
}

[test]
def test_select_many_pair_untyped_fold(t : T?) {
    let orders <- mk_orders()
    // UNTYPED lambdas — proves the two-stage `_select_many` op-macro injects (o : Order) then (x : Line),
    // and `select_many_pair` runs at tier-3. 2+1+1 lines flattened across 3 orders.
    let rows <- _fold(orders
        |> _select_many($(o) => o.lines |> to_sequence(), $(o, x) => (id = o.id, sku = x.sku))
        |> to_array())
    t |> equal(length(rows), 4, "flattened line count")
    t |> equal(rows[0].id, 1)
    t |> equal(rows[0].sku, "a")
    t |> equal(rows[3].id, 3)
    t |> equal(rows[3].sku, "d")
}

[test]
def test_correlated_project_both(t : T?) {
    let orders <- mk_orders()
    // headline correlated case — the result selector references BOTH the outer and inner range vars.
    let rows <- %linq! from o in orders from l in o.lines select (Id = o.id, Sku = l.sku) %%
    t |> equal(length(rows), 4)
    t |> equal(rows[0].Id, 1)
    t |> equal(rows[0].Sku, "a")
    t |> equal(rows[2].Id, 2)
    t |> equal(rows[2].Sku, "c")
}

[test]
def test_correlated_project_inner(t : T?) {
    let orders <- mk_orders()
    let skus <- %linq! from o in orders from l in o.lines select l.sku %%
    t |> equal(length(skus), 4)
    t |> equal(skus[0], "a")
    t |> equal(skus[3], "d")
}

[test]
def test_correlated_push_where(t : T?) {
    let orders <- mk_orders()
    // inner-only post-`from` `where` — pushed into the collection (filter o.lines before flattening).
    // qty>1 keeps a(2), c(5), d(3); drops b(1).
    let rows <- %linq! from o in orders from l in o.lines where l.qty > 1 select (Id = o.id, Sku = l.sku) %%
    t |> equal(length(rows), 3, "qty>1 drops the single qty=1 line")
    t |> equal(rows[0].Sku, "a")
    t |> equal(rows[1].Sku, "c")
    t |> equal(rows[2].Sku, "d")
}

[test]
def test_correlated_multiple_inner_where(t : T?) {
    let orders <- mk_orders()
    // two inner-only post-`from` `where`s both push into the collection (filter before flattening).
    // qty>1 keeps a(2),c(5),d(3); sku!="c" then drops c → a,d.
    let rows <- %linq! from o in orders from l in o.lines where l.qty > 1 where l.sku != "c" select (Id = o.id, Sku = l.sku) %%
    t |> equal(length(rows), 2, "both inner predicates push into the collection")
    t |> equal(rows[0].Sku, "a")
    t |> equal(rows[1].Sku, "d")
}

[test]
def test_correlated_pre_where(t : T?) {
    let orders <- mk_orders()
    // pre-`from` `where` filters the OUTER (orders 2, 3) before flattening.
    let skus <- %linq! from o in orders where o.id >= 2 from l in o.lines select l.sku %%
    t |> equal(length(skus), 2)
    t |> equal(skus[0], "c")
    t |> equal(skus[1], "d")
}

[test]
def test_correlated_iterator_output(t : T?) {
    let orders <- mk_orders()
    var n = 0
    for (_s in %linq! from o in orders from l in o.lines select l.sku iterator %%) { n++ }
    t |> equal(n, 4)
}

// ===== let bindings (Approach A — textual inlining) =====
// `let n = <expr>` introduces a computed range variable; every downstream reference is inlined as
// `(<expr>)`, so the chain is identical to writing the expression out by hand. Works in every context and,
// for SQL, the inlined computed column/key pushes down (see tests/dasSQLITE/test_linq_das_sql.das).

[test]
def test_let_where_and_select(t : T?) {
    let cars <- mk_cars()
    // `net` is reused in both the where predicate and the projection — inlined at each site.
    let rows <- %linq! from c in cars let net = c.price - 10 where net < 150 select (Name = c.name, Net = net) %%
    t |> equal(length(rows), 2, "net<150 keeps cheap(40), mid(140)")
    t |> equal(rows[0].Name, "cheap")
    t |> equal(rows[0].Net, 40)
    t |> equal(rows[1].Name, "mid")
    t |> equal(rows[1].Net, 140)
}

[test]
def test_let_chained(t : T?) {
    let cars <- mk_cars()
    // `net` references the earlier `disc` — chained lets resolve in order.
    let rows <- %linq! from c in cars let disc = c.price / 10 let net = c.price - disc
        orderby net descending select (N = c.name, Net = net) %%
    t |> equal(length(rows), 3)
    t |> equal(rows[0].N, "lux")     // 300 - 30 = 270
    t |> equal(rows[0].Net, 270)
    t |> equal(rows[2].N, "cheap")   // 50 - 5 = 45
    t |> equal(rows[2].Net, 45)
}

[test]
def test_let_computed_select_only(t : T?) {
    let cars <- mk_cars()
    let rows <- %linq! from c in cars let tax = c.price / 5 select (N = c.name, Tax = tax) %%
    t |> equal(length(rows), 3)
    t |> equal(rows[0].Tax, 10)   // 50/5
    t |> equal(rows[2].Tax, 60)   // 300/5
}

[test]
def test_let_in_group_key(t : T?) {
    let cars <- mk_cars()
    let groups <- %linq! from c in cars let b = c.brand group c by b %%
    t |> equal(length(groups), 2, "let key feeds group..by")
    for (g in groups) {
        if (g._0 == "eco") { t |> equal(length(g._1), 2) }
        if (g._0 == "lux") { t |> equal(length(g._1), 1) }
    }
}

[test]
def test_let_decs(t : T?) {
    seed_decs()
    let names <- %linq! from c : CarComp in decs let hi = c.price * 2 orderby hi descending select c.name %%
    t |> equal(length(names), 3)
    t |> equal(names[0], "car2")   // 300*2 highest
    t |> equal(names[2], "car0")   // 100*2 lowest
}

[test]
def test_let_join_both_vars(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    // a `let` after the join references BOTH range variables; inlined into the select-terminal.
    let tags <- %linq! from c in cars join b in brands on c.brand equals b.brand
        let tag = "{c.name}:{b.country}" select tag %%
    t |> equal(length(tags), 3)
    t |> equal(tags[0], "cheap:USA")
    t |> equal(tags[2], "lux:Italy")
}

[test]
def test_let_cross(t : T?) {
    let cars <- mk_cars()
    let brands <- mk_brands()
    let pairs <- %linq! from c in cars from b in brands let pair = "{c.name}/{b.country}" select pair %%
    t |> equal(length(pairs), 6, "3 cars × 2 brands")
    t |> equal(pairs[0], "cheap/USA")
}

[test]
def test_let_correlated(t : T?) {
    let orders <- mk_orders()
    // a `let` over a correlated (flatten) query references the outer and inner range vars.
    let labels <- %linq! from o in orders from l in o.lines let label = "{o.id}-{l.sku}" select label %%
    t |> equal(length(labels), 4)
    t |> equal(labels[0], "1-a")
    t |> equal(labels[3], "3-d")
}

[test]
def test_let_forward_scope_no_clobber(t : T?) {
    let cars <- mk_cars()
    let lux = "eco"   // OUTER var; the `where` references it BEFORE the same-named `let` is introduced
    // C# forward-only `let` scoping: the `where`'s `lux` is the outer ("eco") — it must NOT be rewritten
    // to the later binding. So the filter keeps the two eco cars, then `select lux` uses the let (c.brand).
    let brands <- %linq! from c in cars where c.brand == lux let lux = c.brand select lux %%
    t |> equal(length(brands), 2, "where uses OUTER lux (eco), not the let")
    t |> equal(brands[0], "eco")
    t |> equal(brands[1], "eco")
}

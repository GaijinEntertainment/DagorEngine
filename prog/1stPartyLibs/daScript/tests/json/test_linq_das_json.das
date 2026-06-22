options gen2

require daslib/json_boost
require daslib/linq_das
require daslib/linq_boost public
require dastest/testing_boost public

// The C# query reader (`%linq! … %%`) over a JSON source — `from c : Row in <JsonValue>` dispatches through
// from_in to from_json. Each query is compared against the equivalent array-source `%linq!` so the JSON
// typed-source path is verified to match the (untyped) array path exactly.

struct Car {
    id    : int
    price : int
    qty   : int
    brand : string
}

def fixture_array() : array<Car> {
    return <- [
        Car(id = 1, price = 100,   qty = 2, brand = "Ford"),
        Car(id = 2, price = 9999,  qty = 4, brand = "Ferrari"),
        Car(id = 3, price = 200,   qty = 6, brand = "Honda"),
        Car(id = 4, price = 50,    qty = 8, brand = "Ford"),
        Car(id = 5, price = 88888, qty = 1, brand = "Ferrari"),
        Car(id = 6, price = 300,   qty = 3, brand = "Honda"),
        Car(id = 7, price = 150,   qty = 5, brand = "Ford")]
}

def fixture_json() : JsonValue? {
    return JV([for (c in fixture_array()); JV(c)])
}

[test]
def query_where_select(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    let j <- %linq! from c : Car in jv where c.price > 150 select c.id %%
    let a <- %linq! from c in arr where c.price > 150 select c.id %%
    t |> equal(j, a)
}

[test]
def query_orderby(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    let j <- %linq! from c : Car in jv orderby c.price descending select c.id %%
    let a <- %linq! from c in arr orderby c.price descending select c.id %%
    t |> equal(j, a)
}

[test]
def query_let_and_multi_where(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    let j <- %linq! from c : Car in jv let total = c.price * c.qty where total > 400 where c.brand != "Ferrari" orderby c.id select "{c.brand}:{total}" %%
    let a <- %linq! from c in arr let total = c.price * c.qty where total > 400 where c.brand != "Ferrari" orderby c.id select "{c.brand}:{total}" %%
    t |> equal(j, a)
}

[test]
def query_group_by(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    let j <- %linq! from c : Car in jv group c by c.brand into g orderby g.key select "{g.key}={g |> length}" %%
    let a <- %linq! from c in arr group c by c.brand into g orderby g.key select "{g.key}={g |> length}" %%
    t |> equal(j, a)
}

[test]
def query_iterator_keyword(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    let ji <- [for (x in %linq! from c : Car in jv where c.qty > 3 select c.id iterator %%); x]
    let ai <- [for (x in %linq! from c in arr where c.qty > 3 select c.id iterator %%); x]
    t |> equal(ji, ai)
}

// ===== join: JSON left source ⋈ in-memory array right source =====

struct BrandHQ {
    brand   : string
    country : string
}

def private mk_brands() : array<BrandHQ> {
    return <- [BrandHQ(brand = "Ford",  country = "USA"),
               BrandHQ(brand = "Honda", country = "Japan")]
}

[test]
def query_join_select_terminal(t : T?) {
    let jv = fixture_json()
    let brands <- mk_brands()
    // Ford ×3 → USA, Honda ×2 → Japan; the two Ferraris are unmatched (inner join) → 5 rows.
    let rows <- %linq! from c : Car in jv join b in brands on c.brand equals b.brand select (Id = c.id, Country = b.country) %%
    t |> equal(length(rows), 5, "5 cars match a brand HQ")
    var usa = 0
    for (r in rows) {
        if (r.Country == "USA") { usa++ }
    }
    t |> equal(usa, 3, "three Fords → USA")
}

[test]
def query_join_pre_join_where(t : T?) {
    let jv = fixture_json()
    let brands <- mk_brands()
    // pre-join where filters the JSON rows (price > 199 → id2,3,5,6) before the join; only Hondas id3,id6 match.
    let rows <- %linq! from c : Car in jv where c.price > 199 join b in brands on c.brand equals b.brand select (Id = c.id, Country = b.country) %%
    t |> equal(length(rows), 2, "pre-join where prunes the array walk")
}

[test]
def query_join_orderby(t : T?) {
    let jv = fixture_json()
    let brands <- mk_brands()
    // post-join orderby → transparent-id carry; sort the joined pairs by price asc.
    let ids <- %linq! from c : Car in jv join b in brands on c.brand equals b.brand orderby c.price select c.id %%
    t |> equal(length(ids), 5)
    t |> equal(ids[0], 4, "Ford id4 price 50 first")
    t |> equal(ids[4], 6, "Honda id6 price 300 last")
}

[test]
def query_join_group_after(t : T?) {
    let jv = fixture_json()
    let brands <- mk_brands()
    let groups <- %linq! from c : Car in jv join b in brands on c.brand equals b.brand group c by b.country %%
    t |> equal(length(groups), 2, "USA + Japan buckets")
    for (g in groups) {
        if (g._0 == "USA")   { t |> equal(length(g._1), 3, "three Fords") }
        if (g._0 == "Japan") { t |> equal(length(g._1), 2, "two Hondas") }
    }
}

// ===== multiple `from`: JSON left source × in-memory array right source (uncorrelated cross product).
// The JSON source is the unfused `_cross_join` chain head — `from_json` is passed as a raw argument (not a
// fused for-loop source), so it relies on the unsafe wrap surviving from_in's `$c` expansion. =====

[test]
def query_multifrom_tuple(t : T?) {
    let jv = fixture_json()
    let brands <- mk_brands()
    let rows <- %linq! from c : Car in jv from b in brands select (Id = c.id, Country = b.country) %%
    t |> equal(length(rows), 14, "7 JSON cars × 2 brands cartesian product")
}

[test]
def query_multifrom_pre_where(t : T?) {
    let jv = fixture_json()
    let brands <- mk_brands()
    // pre-from where prunes the array walk (price > 199 → id2,3,5,6) before the cross.
    let rows <- %linq! from c : Car in jv where c.price > 199 from b in brands select (Id = c.id, Country = b.country) %%
    t |> equal(length(rows), 8, "4 filtered JSON cars × 2 brands")
}

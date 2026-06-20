options gen2
options persistent_heap

require daslib/json_boost
require daslib/linq_fold
require daslib/linq_boost public
require dastest/testing_boost public

// JSON linq parity — the fused JsonAdapter lane must agree with the trusted array baseline across every
// single-source shape (where / select / orderby / group_by / distinct / aggregates / element + quantifier
// terminators / take / skip / whole-row escape / under-guard). `options persistent_heap` because the
// single-element terminators declare an uninitialized var of the (string-bearing) element type.

struct Car {
    id    : int
    price : float
    qty   : int
    brand : string
}

def fixture_array() : array<Car> {
    return <- [
        Car(id = 1, price = 100.0,   qty = 2, brand = "Ford"),
        Car(id = 2, price = 9999.0,  qty = 4, brand = "Ferrari"),
        Car(id = 3, price = 200.0,   qty = 6, brand = "Honda"),
        Car(id = 4, price = 50.0,    qty = 8, brand = "Ford"),
        Car(id = 5, price = 88888.0, qty = 1, brand = "Ferrari"),
        Car(id = 6, price = 300.0,   qty = 3, brand = "Honda"),
        Car(id = 7, price = 150.0,   qty = 5, brand = "Ford"),
        Car(id = 8, price = 2000.0,  qty = 7, brand = "Toyota")]
}

// Same 8 rows as a JSON array of objects (reflective JV(Car)), so the fused JSON lane is directly
// comparable to fixture_array.
def fixture_json() : JsonValue? {
    return JV([for (c in fixture_array()); JV(c)])
}

[test]
def parity_where_select(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    // scalar projection, field-pruned (reads only price + id)
    let j <- _fold(unsafe(from_json(jv, type<Car>))._where(_.price > 199.0)._select(_.id).to_array())
    let a <- _fold(each(arr)._where(_.price > 199.0)._select(_.id).to_array())
    t |> equal(j, a)
    // string projection
    let js <- _fold(unsafe(from_json(jv, type<Car>))._where(_.qty > 4)._select(_.brand).to_array())
    let as_ <- _fold(each(arr)._where(_.qty > 4)._select(_.brand).to_array())
    t |> equal(js, as_)
}

[test]
def parity_whole_row(t : T?) {
    // whole-row escape: `to_array()` with no `_select` yields `array<Car>`, so the entire row escapes ->
    // allUsed -> the adapter materializes every field by name (Option B, not an overridable `from_JV(it)`).
    let jv = fixture_json()
    let arr <- fixture_array()
    let j <- _fold(unsafe(from_json(jv, type<Car>))._where(_.brand == "Ford").to_array())
    let a <- _fold(each(arr)._where(_.brand == "Ford").to_array())
    t |> equal(length(j), length(a))
    for (cj, ca in j, a) {
        t |> equal(cj.id, ca.id)
        t |> equal(cj.price, ca.price)
        t |> equal(cj.qty, ca.qty)
        t |> equal(cj.brand, ca.brand)
    }
    // second whole-row escape under a numeric filter, materializing a different (non-Ford) survivor set
    let jp <- _fold(unsafe(from_json(jv, type<Car>))._where(_.price > 199.0).to_array())
    let ap <- _fold(each(arr)._where(_.price > 199.0).to_array())
    t |> equal(length(jp), length(ap))
    for (cj, ca in jp, ap) {
        t |> equal(cj.id, ca.id)
        t |> equal(cj.brand, ca.brand)
    }
}

[test]
def parity_orderby(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    // single key ascending
    let j <- _fold(unsafe(from_json(jv, type<Car>))._order_by(_.price)._select(_.id).to_array())
    let a <- _fold(each(arr)._order_by(_.price)._select(_.id).to_array())
    t |> equal(j, a)
    // descending
    let jd <- _fold(unsafe(from_json(jv, type<Car>))._order_by_descending(_.price)._select(_.id).to_array())
    let ad <- _fold(each(arr)._order_by_descending(_.price)._select(_.id).to_array())
    t |> equal(jd, ad)
    // multi-key (brand asc, price desc)
    let jm <- _fold(unsafe(from_json(jv, type<Car>))._order_by_keys($(c) => (c.brand, c.price), 2u)._select(_.id).to_array())
    let am <- _fold(each(arr)._order_by_keys($(c) => (c.brand, c.price), 2u)._select(_.id).to_array())
    t |> equal(jm, am)
}

[test]
def parity_group_by(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    // group brand -> count per bucket, then order by key for a stable compare
    let j <- _fold(unsafe(from_json(jv, type<Car>))._group_by_lazy(_.brand)._order_by(_._0)._select("{_._0}={length(_._1)}").to_array())
    let a <- _fold(each(arr)._group_by_lazy(_.brand)._order_by(_._0)._select("{_._0}={length(_._1)}").to_array())
    t |> equal(j, a)
}

[test]
def parity_group_by_first(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    // group brand -> the FIRST whole row of each bucket escapes into the result tuple (allUsed whole-row
    // materialize), which fires the deferred survivor materialize (jhdef) alongside the group-key array
    // walk (jdef) at the same source line — a qname-collision regression (groupby_first benchmark).
    let j <- _fold(unsafe(from_json(jv, type<Car>))._group_by(_.brand)._select((Brand = _._0, First = _._1 |> first())).to_array())
    let a <- _fold(each(arr)._group_by(_.brand)._select((Brand = _._0, First = _._1 |> first())).to_array())
    var js <- [for (g in j); "{g.Brand}:{g.First.id}:{g.First.price}:{g.First.brand}"]
    var as_ <- [for (g in a); "{g.Brand}:{g.First.id}:{g.First.price}:{g.First.brand}"]
    sort(js)
    sort(as_)
    t |> equal(js, as_)
}

[test]
def parity_aggregates(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._select(_.qty).sum()),
               _fold(each(arr)._select(_.qty).sum()))
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._where(_.price > 100.0)._count(_.id > 0)),
               _fold(each(arr)._where(_.price > 100.0)._count(_.id > 0)))
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._select(_.qty).min()),
               _fold(each(arr)._select(_.qty).min()))
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._select(_.qty).max()),
               _fold(each(arr)._select(_.qty).max()))
}

[test]
def parity_distinct(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    // distinct brand, ordered for a stable compare
    let j <- _fold(unsafe(from_json(jv, type<Car>))._select(_.brand)._order_by(_).distinct().to_array())
    let a <- _fold(each(arr)._select(_.brand)._order_by(_).distinct().to_array())
    t |> equal(j, a)
}

[test]
def parity_first_last_single(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._where(_.price > 199.0).first()).id,
               _fold(each(arr)._where(_.price > 199.0).first()).id)
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._where(_.price > 199.0).last()).id,
               _fold(each(arr)._where(_.price > 199.0).last()).id)
    // exactly one Toyota
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._where(_.brand == "Toyota").single()).id,
               _fold(each(arr)._where(_.brand == "Toyota").single()).id)
    // under-guard: read a string field off the deferred survivor
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._where(_.price > 199.0).last()).brand,
               _fold(each(arr)._where(_.price > 199.0).last()).brand)
}

[test]
def parity_element_at(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    t |> equal(_fold(unsafe(from_json(jv, type<Car>)).element_at(2)).id,
               _fold(each(arr).element_at(2)).id)
    t |> equal(_fold(unsafe(from_json(jv, type<Car>)).element_at_or_default(99)).id,
               _fold(each(arr).element_at_or_default(99)).id)
}

[test]
def parity_any_all_contains(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._where(_.price > 50000.0).any()),
               _fold(each(arr)._where(_.price > 50000.0).any()))
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._all(_.price > 0.0)),
               _fold(each(arr)._all(_.price > 0.0)))
    t |> equal(_fold(unsafe(from_json(jv, type<Car>))._select(_.id).contains(5)),
               _fold(each(arr)._select(_.id).contains(5)))
}

[test]
def parity_take_skip(t : T?) {
    let jv = fixture_json()
    let arr <- fixture_array()
    let j <- _fold(unsafe(from_json(jv, type<Car>))._order_by(_.id).skip(2).take(3)._select(_.id).to_array())
    let a <- _fold(each(arr)._order_by(_.id).skip(2).take(3)._select(_.id).to_array())
    t |> equal(j, a)
}

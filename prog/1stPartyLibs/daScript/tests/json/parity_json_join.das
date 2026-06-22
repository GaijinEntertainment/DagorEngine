options gen2

require daslib/json_boost
require daslib/linq_fold
require daslib/linq_boost public
require dastest/testing_boost public

// JSON linq-join parity — `join` (and `join |> group_by`) over a from_json source. The join fuses via
// JsonAdapter.emit_join_hook (srcB collected into a hash, srcA probed from the field-pruned array walk);
// `join |> group_by` fuses via JsonJoinAdapter (returned from build_group_by_adapter on the upstream-join arm).
// This file proves the fused JSON result equals the array-fold result — the correctness gate for the fusion.
//   jf = _fold over from_json(...)   af = _fold over each(arr)

struct Car {
    id    : int
    price : float
    qty   : int
    brand : string
}

struct BrandHQ {
    brand   : string
    country : string
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

// Same 8 rows as a JSON array of objects (reflective JV(Car)), so the fused JSON lane is directly comparable.
def fixture_json() : JsonValue? {
    return JV([for (c in fixture_array()); JV(c)])
}

def brand_array() : array<BrandHQ> {
    return <- [
        BrandHQ(brand = "Ford",    country = "USA"),
        BrandHQ(brand = "Ferrari", country = "Italy"),
        BrandHQ(brand = "Honda",   country = "Japan"),
        BrandHQ(brand = "Toyota",  country = "Japan")]
}

// Sum of (id, country)-by-country maps must agree between JSON fold and array fold.
def country_id_sums(rows : array<tuple<id : int; country : string>>) : table<string; int> {
    var m : table<string; int>   // nolint:STYLE027 accumulation (running sum per key), not a simple insert — no comprehension form
    for (r in rows) {
        m[r.country] = (m?[r.country] ?? 0) + r.id
    }
    return <- m
}

def tables_equal(a : table<string; int>; b : table<string; int>) : bool {
    if (length(a) != length(b)) return false
    for (k, v in keys(a), values(a)) {
        if ((b?[k] ?? -1) != v) return false
    }
    return true
}

[test]
def parity_join_count(t : T?) {
    // join + count (no where) — bucket-length fast path. Every car's brand matches exactly one HQ → 8.
    let jv = fixture_json()
    let arr <- fixture_array()
    let hqs <- brand_array()
    let jf = _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> count())
    let af = _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> count())
    t |> equal(jf, 8)
    t |> equal(jf, af)
}

[test]
def parity_join_to_array(t : T?) {
    // join + to_array (named-tuple projection) — compare (country → sum id) maps.
    let jv = fixture_json()
    let arr <- fixture_array()
    let hqs <- brand_array()
    var jf <- _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> to_array())
    var af <- _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> to_array())
    t |> equal(length(jf), length(af))
    var sj <- country_id_sums(jf)
    var sa <- country_id_sums(af)
    t |> success(tables_equal(sj, sa), "country→sum(id) jf == af")
    delete jf; delete af; delete sj; delete sa
}

[test]
def parity_join_where_count(t : T?) {
    // join + where + count — predicate fires per pair; only USA (Ford ×3) survives.
    let jv = fixture_json()
    let arr <- fixture_array()
    let hqs <- brand_array()
    let jf = _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> _where(_.country == "USA")
                  |> count())
    let af = _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> _where(_.country == "USA")
                  |> count())
    t |> equal(jf, 3)
    t |> equal(jf, af)
}

[test]
def parity_join_select(t : T?) {
    // join + bare _select — sum the projected ids.
    let jv = fixture_json()
    let arr <- fixture_array()
    let hqs <- brand_array()
    var jf <- _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> _select(_.id)
                  |> to_array())
    var af <- _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> _select(_.id)
                  |> to_array())
    t |> equal(length(jf), 8)
    var sumj = 0
    var suma = 0
    for (x in jf) { sumj += x }
    for (x in af) { suma += x }
    t |> equal(sumj, 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8)
    t |> equal(sumj, suma)
    delete jf; delete af
}

[test]
def parity_join_select_no_terminator(t : T?) {
    // join + bare _select with NO to_array → iterator-typed result. Fuses via the fused-materialize path
    // (build result array, wrap as sequence). Correctness gate: the fused sequence equals the eager array lane.
    let jv = fixture_json()
    let arr <- fixture_array()
    let hqs <- brand_array()
    var jf <- _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> _select(_.id))
    var af <- _fold(arr |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> _select(_.id))
    var sumj = 0
    var cntj = 0
    for (x in jf) {
        sumj += x
        cntj ++
    }
    var suma = 0
    for (x in af) {
        suma += x
    }
    t |> equal(cntj, 8)
    t |> equal(sumj, 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8)
    t |> equal(sumj, suma)
    delete jf
    delete af
}

[test]
def parity_join_group_by_sum(t : T?) {
    // join |> group_by(country) |> select(sum qty) — group-by-after-join (JsonJoinAdapter). Compare country→sum(qty).
    let jv = fixture_json()
    let arr <- fixture_array()
    let hqs <- brand_array()
    var jf <- _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (country = h.country, qty = c.qty))
                  |> _group_by(_.country)
                  |> _select((c = _._0, total = _._1 |> _select(_.qty) |> sum()))
                  |> to_array())
    var af <- _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (country = h.country, qty = c.qty))
                  |> _group_by(_.country)
                  |> _select((c = _._0, total = _._1 |> _select(_.qty) |> sum()))
                  |> to_array())
    t |> equal(length(jf), length(af))
    var sj : table<string; int>
    var sa : table<string; int>
    for (g in jf) { sj[g.c] = g.total }
    for (g in af) { sa[g.c] = g.total }
    t |> success(tables_equal(sj, sa), "country→sum(qty) jf == af")
    delete jf; delete af
}

[test]
def parity_join_edge_buckets(t : T?) {
    // Inner-join semantics the 1:1 fixture above never hits: an unmatched srcA row (Tesla has no HQ → 0 rows)
    // and a multi-row srcB bucket (Ford has two HQ rows → each Ford car fans out ×2). Both exercise the
    // JsonAdapter hash-probe branches directly. Hand-computed inner join: Ford 2cars ×2HQ = 4, Honda 1×1 = 1,
    // Tesla 0 → 5 rows; country→sum(id) = {USA:3, Canada:3, Japan:4}.
    let arr <- [
        Car(id = 1, price = 100.0, qty = 2, brand = "Ford"),
        Car(id = 2, price = 200.0, qty = 3, brand = "Ford"),
        Car(id = 3, price = 300.0, qty = 4, brand = "Tesla"),
        Car(id = 4, price = 400.0, qty = 5, brand = "Honda")]
    let jv = JV([for (c in arr); JV(c)])
    let hqs <- [
        BrandHQ(brand = "Ford",  country = "USA"),
        BrandHQ(brand = "Ford",  country = "Canada"),   // duplicate key → multi-row bucket → fan-out
        BrandHQ(brand = "Honda", country = "Japan")]    // no Tesla → unmatched srcA row
    // count fast-path over a length-2 bucket + an unmatched row.
    let cj = _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> count())
    let ca = _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> count())
    t |> equal(cj, 5)
    t |> equal(cj, ca)
    // materializing path — country→sum(id) must agree across the fan-out + unmatched row.
    var jf <- _fold(unsafe(from_json(jv, type<Car>)) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> to_array())
    var af <- _fold(each(arr) |> _join(hqs,
                       $(c : Car, h : BrandHQ) => c.brand == h.brand,
                       $(c : Car, h : BrandHQ) => (id = c.id, country = h.country))
                  |> to_array())
    t |> equal(length(jf), 5)
    var sj <- country_id_sums(jf)
    var sa <- country_id_sums(af)
    t |> success(tables_equal(sj, sa), "edge country→sum(id) jf == af")
    delete jf; delete af; delete sj; delete sa
}

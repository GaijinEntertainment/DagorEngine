options gen2

require daslib/json_boost
require dastest/testing_boost public

// The from_json generator (the for-loop / tier-2 source builder) and read_json_field (the field-pruned
// single-key read), exercised directly — edge cases the fused _fold lane never reaches: empty array,
// non-array value, null source.

struct Car {
    id    : int
    name  : string
    price : int = -1
}

def mkjson(n : int) : JsonValue? {
    return JV([for (i in range(n)); JV(Car(id = i + 1, name = "Car{i}", price = i * 10))])
}

[test]
def test_for_loop(t : T?) {
    let jv = mkjson(5)
    var ids : array<int>
    var sumPrice = 0
    for (c in from_json(jv, type<Car>)) {
        ids |> push(c.id)
        sumPrice += c.price
    }
    t |> equal(ids, [1, 2, 3, 4, 5])
    t |> equal(sumPrice, 0 + 10 + 20 + 30 + 40)
}

[test]
def test_comprehension(t : T?) {
    let jv = mkjson(4)
    let names <- [for (c in from_json(jv, type<Car>)); c.name]
    t |> equal(names, ["Car0", "Car1", "Car2", "Car3"])
}

[test]
def test_empty_array(t : T?) {
    var empty : array<JsonValue?>
    let jv = JV(empty)
    var n = 0
    for (_c in from_json(jv, type<Car>)) {
        n++
    }
    t |> equal(n, 0)
}

[test]
def test_non_array(t : T?) {
    // a JSON string (not an array) yields nothing
    let jv = JV("not an array")
    var n = 0
    for (_c in from_json(jv, type<Car>)) {
        n++
    }
    t |> equal(n, 0)
}

[test]
def test_null_source(t : T?) {
    var jv : JsonValue? = null
    var n = 0
    for (_c in from_json(jv, type<Car>)) {
        n++
    }
    t |> equal(n, 0)
}

[test]
def test_read_json_field(t : T?) {
    let jv = mkjson(3)
    var first : JsonValue const?
    for (e in jv.value as _array) {
        first = e
        break
    }
    // present keys
    t |> equal(read_json_field(first, "id", -99), 1)
    t |> equal(read_json_field(first, "name", "?"), "Car0")
    // missing key keeps the supplied default
    t |> equal(read_json_field(first, "nope", 42), 42)
    t |> equal(read_json_field(first, "missing", "fallback"), "fallback")
}

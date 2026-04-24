options gen2
require dastest/testing_boost public
require daslib/json_boost

[test]
def safe_operators(t : T?) {
    t |> run("safe object") @(t) {
        let data = "\{ \"a\": 1, \"b\": true, \"c\": null, \"d\": \"str\" \}"
        var err = ""
        var js = read_json(data, err)
        t |> equal(err, "")

        t |> equal((js?["a"] ?as _longint) ?? -1l, 1l)
        t |> equal(js?.a ?? -1, 1)
        t |> equal(js?.b ?? false, true)
        t |> equal(js?.c is _null, true)
        t |> equal(js?.d ?? "", "str")
        t |> equal(js?.xx is _null, true) // invalid value
        t |> equal(js?.xx?.xy is _null, true) // invalid value
    }

    t |> run("safe array") @(t) {
        let data = "[ 1, true, null, \"str\" ]"
        var err = ""
        var js = read_json(data, err)
        t |> equal(err, "")

        t |> equal(js?[0] ?? -1, 1)
        t |> equal(js?[1] ?? false, true)
        t |> equal(js?[2] is _null, true)
        t |> equal(js?[3] ?? "", "str")
        t |> equal(js?[42] is _null, true) // invalid value
        t |> equal(js?[42]?[0] is _null, true) // invalid value
    }

    t |> run("null object") @(t) {
        var js : JsonValue?
        t |> equal(js?.a ?? -1, -1)
        t |> equal(js?[0] ?? -1, -1)
        js = JVNull()
        t |> equal(js?.a ?? -1, -1)
        t |> equal(js?[0] ?? -1, -1)
    }
}

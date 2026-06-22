options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== make_request (int id) =====

[test]
def test_make_request_int_id(t : T?) {
    t |> run("with object params") @(t : T?) {
        let r = make_request("set_color", "\{\"r\":1.0}", 7)
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"set_color\",\"params\":\{\"r\":1.0}}")
    }
    t |> run("with array params") @(t : T?) {
        let r = make_request("add", "[1,2,3]", 1)
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"add\",\"params\":[1,2,3]}")
    }
    t |> run("with null params") @(t : T?) {
        let r = make_request("ping", "null", 42)
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"ping\",\"params\":null}")
    }
    t |> run("method name with special chars is JSON-escaped") @(t : T?) {
        let r = make_request("a\"b", "null", 1)
        t |> success(find(r, "\"a\\\"b\"") >= 0, "method quote escaped")
    }
}

// ===== make_request (string id) =====

[test]
def test_make_request_string_id(t : T?) {
    t |> run("simple string id") @(t : T?) {
        let r = make_request("status", "null", "abc")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":\"abc\",\"method\":\"status\",\"params\":null}")
    }
    t |> run("string id with special chars is JSON-escaped") @(t : T?) {
        let r = make_request("x", "null", "call \"1\"")
        t |> success(find(r, "\"call \\\"1\\\"\"") >= 0, "id quotes escaped")
    }
}

// ===== make_notification =====

[test]
def test_make_notification(t : T?) {
    t |> run("notification has no id field") @(t : T?) {
        let r = make_notification("shutdown", "null")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"method\":\"shutdown\",\"params\":null}")
        t |> success(find(r, "\"id\"") < 0, "no id field")
    }
    t |> run("notification with params") @(t : T?) {
        let r = make_notification("log", "\{\"msg\":\"hi\"}")
        t |> success(find(r, "\"method\":\"log\"") >= 0, "method present")
        t |> success(find(r, "\"msg\":\"hi\"") >= 0, "params present")
        t |> success(find(r, "\"id\"") < 0, "no id field")
    }
}

// ===== make_batch =====

[test]
def test_make_batch(t : T?) {
    t |> run("two requests") @(t : T?) {
        var msgs : array<string>
        msgs |> push(make_request("a", "null", 1))
        msgs |> push(make_request("b", "null", 2))
        let r = make_batch(msgs)
        t |> success(character_at(r, 0) == '[', "starts with [")
        t |> success(character_at(r, length(r) - 1) == ']', "ends with ]")
        t |> success(find(r, "\"method\":\"a\"") >= 0, "first method present")
        t |> success(find(r, "\"method\":\"b\"") >= 0, "second method present")
        // Order preserved
        let pa = find(r, "\"method\":\"a\"")
        let pb = find(r, "\"method\":\"b\"")
        t |> success(pa >= 0 && pb > pa, "order preserved")
    }
    t |> run("empty input returns []") @(t : T?) {
        let msgs : array<string>
        t |> equal(make_batch(msgs), "[]")
    }
    t |> run("mixed request + notification") @(t : T?) {
        var msgs : array<string>
        msgs |> push(make_request("rpc", "null", 1))
        msgs |> push(make_notification("notif", "null"))
        let r = make_batch(msgs)
        t |> success(find(r, "\"id\":1") >= 0, "request id present")
        t |> success(find(r, "\"method\":\"notif\"") >= 0, "notification present")
    }
}

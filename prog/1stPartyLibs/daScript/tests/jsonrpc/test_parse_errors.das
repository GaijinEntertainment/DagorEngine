options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== PARSE_ERROR (-32700) =====

[test]
def test_parse_error_malformed_json(t : T?) {
    t |> run("garbage text returns -32700 with id:null") @(t : T?) {
        let r = parse_request("not json at all")
        t |> success(!empty(r.error_envelope), "envelope built")
        t |> equal(r.method, "")
        t |> success(find(r.error_envelope, "-32700") >= 0, "parse-error code")
        t |> success(find(r.error_envelope, "\"id\":null") >= 0, "id defaults to null")
    }
    t |> run("empty input returns parse error") @(t : T?) {
        let r = parse_request("")
        t |> success(find(r.error_envelope, "-32700") >= 0, "parse-error code")
    }
    t |> run("truncated object returns parse error") @(t : T?) {
        let r = parse_request("\{\"method\":\"x\"")
        t |> success(find(r.error_envelope, "-32700") >= 0, "parse-error code")
    }
}

// ===== INVALID_REQUEST (-32600) — non-object body =====

[test]
def test_invalid_request_non_object_body(t : T?) {
    t |> run("number body returns -32600") @(t : T?) {
        let r = parse_request("42")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
        t |> success(find(r.error_envelope, "not a JSON object") >= 0, "message names the rule")
    }
    t |> run("string body returns -32600") @(t : T?) {
        let r = parse_request("\"hello\"")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    }
    t |> run("null body returns -32600") @(t : T?) {
        let r = parse_request("null")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    }
}

// ===== INVALID_REQUEST (-32600) — missing/invalid method =====

[test]
def test_invalid_request_method(t : T?) {
    t |> run("missing method, id preserved") @(t : T?) {
        let r = parse_request("\{\"id\":7}")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
        t |> success(find(r.error_envelope, "\"id\":7") >= 0, "id preserved")
        t |> success(find(r.error_envelope, "missing or non-string method") >= 0, "message names the rule")
    }
    t |> run("non-string method") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":42}")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
        t |> success(find(r.error_envelope, "missing or non-string method") >= 0, "message names the rule")
    }
    t |> run("empty object (no method, no id) is NOT a notification") @(t : T?) {
        // A notification is a *well-formed* request without id. `{}` is missing
        // the required `method` member, so it must reach the wire with -32600.
        let r = parse_request("\{}")
        t |> success(!r.is_notification, "no method + no id is still NOT a notification")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    }
}

// ===== INVALID_REQUEST (-32600) — invalid id types =====

[test]
def test_invalid_id_types(t : T?) {
    t |> run("object id rejected with id:null") @(t : T?) {
        let r = parse_request("\{\"id\":\{\"weird\":1},\"method\":\"x\"}")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
        t |> success(find(r.error_envelope, "\"id\":null") >= 0, "response id reset to null")
        t |> success(find(r.error_envelope, "id must be string, number, or null") >= 0, "message names the rule")
    }
    t |> run("array id rejected") @(t : T?) {
        let r = parse_request("\{\"id\":[1,2],\"method\":\"x\"}")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    }
    t |> run("boolean id rejected") @(t : T?) {
        let r = parse_request("\{\"id\":true,\"method\":\"x\"}")
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    }
}

// ===== Notification semantics on error =====

[test]
def test_notification_error_suppression(t : T?) {
    t |> run("malformed JSON is NOT a notification (response emitted)") @(t : T?) {
        // Parse failures must produce -32700 — never silent — even though no id.
        let r = parse_request("garbage")
        t |> success(!empty(r.error_envelope), "envelope built")
        t |> success(!r.is_notification, "parse failure ≠ notification")
    }
    t |> run("non-object body is NOT a notification") @(t : T?) {
        let r = parse_request("42")
        t |> success(!r.is_notification, "non-object ≠ notification")
    }
}

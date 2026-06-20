options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require daslib/json
require strings

// ===== Valid request parsing =====

[test]
def test_method_only_no_params_no_id(t : T?) {
    let r = parse_request("\{\"method\":\"status\"}")
    t |> equal(r.error_envelope, "")
    t |> equal(r.method, "status")
    t |> equal(r.id_str, "null")
    t |> equal(r.params_json, "null")
    t |> success(r.params == null, "params is null when absent")
    t |> success(r.is_notification, "no id → notification")
}

[test]
def test_id_types_accepted(t : T?) {
    t |> run("integer id passes through") @(t : T?) {
        let r = parse_request("\{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"status\"}")
        t |> equal(r.error_envelope, "")
        t |> equal(r.id_str, "42")
        t |> equal(r.method, "status")
        t |> success(!r.is_notification, "present id → not a notification")
    }
    t |> run("string id passes through quoted") @(t : T?) {
        let r = parse_request("\{\"id\":\"call-1\",\"method\":\"status\"}")
        t |> equal(r.error_envelope, "")
        t |> equal(r.id_str, "\"call-1\"")
    }
    t |> run("explicit null id is NOT a notification (spec §4.1)") @(t : T?) {
        // JSON-RPC 2.0 distinguishes "absent id" (notification) from
        // "explicit null id" (request expecting a response with id:null).
        let r = parse_request("\{\"id\":null,\"method\":\"status\"}")
        t |> equal(r.error_envelope, "")
        t |> success(!r.is_notification, "explicit null id ≠ notification")
        t |> equal(r.id_str, "null")
    }
    t |> run("number id accepted (float)") @(t : T?) {
        let r = parse_request("\{\"id\":3.14,\"method\":\"status\"}")
        t |> equal(r.error_envelope, "")
    }
}

[test]
def test_params_round_trip(t : T?) {
    t |> run("object params populate params_json") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"set_color\",\"params\":\{\"r\":0.5}}")
        t |> equal(r.error_envelope, "")
        t |> equal(r.method, "set_color")
        t |> success(find(r.params_json, "\"r\"") >= 0, "params_json carries the r key")
        t |> success(r.params != null, "params is not null when present")
    }
    t |> run("array params") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"add\",\"params\":[1,2,3]}")
        t |> equal(r.error_envelope, "")
        // params_json comes from write_json which pretty-prints; compact for comparison.
        t |> equal(compact_json_whitespace(r.params_json), "[1,2,3]")
    }
    t |> run("missing params → 'null'") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"ping\"}")
        t |> equal(r.error_envelope, "")
        t |> equal(r.params_json, "null")
        t |> success(r.params == null, "params field stays null")
    }
}

// ===== gc_node reachability test =====
//
// The parser parses the body into a local JsonValue tree (`var parsed = read_json(...)`),
// then stores a view into `ParsedRequest.params` pointing at a child node. The local
// `parsed` goes out of scope when parse_request returns. JsonValue? is a plain raw
// pointer tracked by gc_node (not a smart_ptr — those are mostly retired); the GC
// keeps the tree reachable while `result.params` still points into it. This test
// confirms the borrowed child view is live after the parser returns — if the GC
// freed the tree, accessing params would crash or return garbage.

[test]
def test_params_lifetime_survives_parser_return(t : T?) {
    // Parse, return, then read params well after the parser's stack is gone.
    let req = parse_request("\{\"id\":1,\"method\":\"set\",\"params\":\{\"a\":1,\"b\":\"hello\",\"c\":[1,2,3]}}")
    t |> equal(req.error_envelope, "")
    t |> success(req.params != null, "params survives parser return")
    // Re-serialize the live params node — if the underlying memory was freed,
    // write_json would crash or produce garbage.
    let re = compact_json_whitespace(write_json(req.params))
    t |> success(find(re, "\"a\"") >= 0, "field 'a' present in re-serialized params")
    t |> success(find(re, "\"hello\"") >= 0, "string value 'hello' present")
    t |> success(find(re, "[1,2,3]") >= 0, "nested array present")
    // Direct field access through the gc-tracked pointer — confirms the variant data is live.
    t |> success(req.params.value is _object, "params is still an _object after parser return")
}

[test]
def test_params_lifetime_array_root(t : T?) {
    // Same test but params is an array (different storage path in JsonValue).
    let req = parse_request("\{\"id\":7,\"method\":\"sum\",\"params\":[10,20,30]}")
    t |> equal(req.error_envelope, "")
    t |> success(req.params != null, "array params survives")
    t |> success(req.params.value is _array, "still an array")
    let re = compact_json_whitespace(write_json(req.params))
    t |> equal(re, "[10,20,30]")
}

// ===== Permissive jsonrpc field (default mode) =====

[test]
def test_permissive_jsonrpc_field(t : T?) {
    t |> run("missing jsonrpc field accepted by default") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"status\"}")
        t |> equal(r.error_envelope, "")
        t |> equal(r.method, "status")
    }
    t |> run("non-2.0 jsonrpc value accepted by default") @(t : T?) {
        let r = parse_request("\{\"jsonrpc\":\"1.0\",\"id\":1,\"method\":\"status\"}")
        t |> equal(r.error_envelope, "")
        t |> equal(r.method, "status")
    }
    t |> run("strict=false flag is identical to default") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"status\"}", false)
        t |> equal(r.error_envelope, "")
    }
}

// ===== Params type validation (§4.2) =====

[test]
def test_params_must_be_array_or_object(t : T?) {
    t |> run("scalar params rejected") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"x\",\"params\":42}")
        t |> success(!empty(r.error_envelope), "envelope built")
        t |> success(find(r.error_envelope, "-32602") >= 0, "INVALID_PARAMS code")
        t |> success(find(r.error_envelope, "array or object") >= 0, "message names the rule")
    }
    t |> run("string params rejected") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"x\",\"params\":\"hello\"}")
        t |> success(find(r.error_envelope, "-32602") >= 0, "INVALID_PARAMS code")
    }
    t |> run("null params accepted (treated as absent)") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"x\",\"params\":null}")
        t |> equal(r.error_envelope, "")
    }
    t |> run("object params accepted") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"x\",\"params\":\{\"a\":1}}")
        t |> equal(r.error_envelope, "")
    }
    t |> run("array params accepted") @(t : T?) {
        let r = parse_request("\{\"id\":1,\"method\":\"x\",\"params\":[1,2]}")
        t |> equal(r.error_envelope, "")
    }
}

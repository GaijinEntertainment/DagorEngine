options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== parse_response — success cases =====

[test]
def test_parse_response_success(t : T?) {
    t |> run("scalar result, integer id") @(t : T?) {
        let r = parse_response("\{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":42}")
        t |> equal(r.parse_error, "")
        t |> success(r.is_success, "is_success=true")
        t |> equal(r.id_str, "1")
        t |> equal(r.result_json, "42")
        t |> equal(r.error_code, 0)
    }
    t |> run("object result") @(t : T?) {
        let r = parse_response("\{\"id\":2,\"result\":\{\"fps\":60}}")
        t |> success(r.is_success, "is_success=true")
        // result_json comes from write_json (pretty-printed); just confirm fields present
        t |> success(find(r.result_json, "\"fps\"") >= 0, "fps field in result")
        t |> success(find(r.result_json, "60") >= 0, "value in result")
    }
    t |> run("string id round-trips") @(t : T?) {
        let r = parse_response("\{\"id\":\"abc\",\"result\":null}")
        t |> equal(r.id_str, "\"abc\"")
        t |> success(r.is_success, "null result still counts as success")
    }
    t |> run("null id round-trips") @(t : T?) {
        let r = parse_response("\{\"id\":null,\"result\":1}")
        t |> equal(r.id_str, "null")
    }
}

// ===== parse_response — error cases =====

[test]
def test_parse_response_error(t : T?) {
    t |> run("method-not-found error") @(t : T?) {
        let r = parse_response("\{\"id\":1,\"error\":\{\"code\":-32601,\"message\":\"method not found\"}}")
        t |> equal(r.parse_error, "")
        t |> success(!r.is_success, "is_success=false on error")
        t |> equal(r.error_code, -32601)
        t |> equal(r.error_msg, "method not found")
        t |> equal(r.error_data, "")
    }
    t |> run("error with data field") @(t : T?) {
        let r = parse_response("\{\"id\":1,\"error\":\{\"code\":-32603,\"message\":\"oops\",\"data\":\{\"trace\":\"foo\"}}}")
        t |> success(!r.is_success, "is_success=false")
        t |> equal(r.error_code, -32603)
        t |> success(find(r.error_data, "\"trace\"") >= 0, "data field captured")
    }
    t |> run("error with scalar data") @(t : T?) {
        let r = parse_response("\{\"id\":1,\"error\":\{\"code\":-1,\"message\":\"x\",\"data\":42}}")
        t |> equal(r.error_data, "42")
    }
}

// ===== parse_response — malformed wires =====

[test]
def test_parse_response_malformed(t : T?) {
    t |> run("non-JSON returns parse_error") @(t : T?) {
        let r = parse_response("garbage")
        t |> success(!empty(r.parse_error), "parse_error populated")
        t |> success(find(r.parse_error, "parse error") >= 0, "names the failure")
    }
    t |> run("non-object body returns parse_error") @(t : T?) {
        let r = parse_response("42")
        t |> success(!empty(r.parse_error), "parse_error populated")
    }
    t |> run("missing both result and error") @(t : T?) {
        let r = parse_response("\{\"id\":1}")
        t |> success(!empty(r.parse_error), "parse_error populated")
    }
}

// ===== parse_response_batch =====

[test]
def test_parse_response_batch_single(t : T?) {
    let pb = parse_response_batch("\{\"id\":1,\"result\":true}")
    t |> success(!pb.is_batch, "single object → is_batch=false")
    t |> equal(length(pb.responses), 1)
    t |> success(pb.responses[0].is_success, "")
}

[test]
def test_parse_response_batch_array(t : T?) {
    let pb = parse_response_batch("[\{\"id\":1,\"result\":1},\{\"id\":2,\"error\":\{\"code\":-1,\"message\":\"e\"}}]")
    t |> success(pb.is_batch, "array → is_batch=true")
    t |> equal(length(pb.responses), 2)
    t |> equal(pb.parse_error, "")
    t |> success(pb.responses[0].is_success, "first success")
    t |> success(!pb.responses[1].is_success, "second error")
    t |> equal(pb.responses[1].error_code, -1)
}

[test]
def test_parse_response_batch_parse_error(t : T?) {
    let pb = parse_response_batch("not json")
    t |> success(!pb.is_batch, "")
    t |> success(!empty(pb.parse_error), "parse_error populated")
    t |> success(empty(pb.responses), "no responses on top-level parse failure")
}

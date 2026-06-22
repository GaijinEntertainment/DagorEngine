options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== strict=true enforces jsonrpc:"2.0" (spec §4) =====

[test]
def test_strict_rejects_missing_jsonrpc(t : T?) {
    let r = parse_request("\{\"id\":1,\"method\":\"x\"}", true)
    t |> success(!empty(r.error_envelope), "envelope built")
    t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    t |> success(find(r.error_envelope, "jsonrpc field must be \\\"2.0\\\"") >= 0, "message names the rule")
}

[test]
def test_strict_rejects_non_2_0(t : T?) {
    t |> run("jsonrpc:1.0 rejected in strict") @(t : T?) {
        let r = parse_request("\{\"jsonrpc\":\"1.0\",\"id\":1,\"method\":\"x\"}", true)
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code")
    }
    t |> run("jsonrpc as number rejected in strict") @(t : T?) {
        let r = parse_request("\{\"jsonrpc\":2.0,\"id\":1,\"method\":\"x\"}", true)
        t |> success(find(r.error_envelope, "-32600") >= 0, "invalid-request code (must be string \"2.0\")")
    }
    t |> run("jsonrpc:\"2.0\" accepted in strict") @(t : T?) {
        let r = parse_request("\{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"x\"}", true)
        t |> equal(r.error_envelope, "")
    }
}

[test]
def test_strict_preserves_id_in_response(t : T?) {
    // strict-mode rejection still echoes the request's id (not null) because
    // id was already validated by the time the jsonrpc field is checked.
    let r = parse_request("\{\"id\":\"abc\",\"method\":\"x\"}", true)
    t |> success(find(r.error_envelope, "\"id\":\"abc\"") >= 0, "id preserved on strict rejection")
}

[test]
def test_strict_batch(t : T?) {
    // parse_batch with strict=true should reject elements with missing jsonrpc.
    let pb = parse_batch("[\{\"id\":1,\"method\":\"x\"},\{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"y\"}]", true)
    t |> success(pb.is_batch, "is_batch true")
    t |> equal(length(pb.requests), 2)
    t |> success(!empty(pb.requests[0].error_envelope), "first rejected (missing jsonrpc)")
    t |> equal(pb.requests[1].error_envelope, "")
}

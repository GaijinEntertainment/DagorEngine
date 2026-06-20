options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== JSON-RPC 2.0 §6 batch parsing =====

[test]
def test_single_request_not_batch(t : T?) {
    let pb = parse_batch("\{\"id\":1,\"method\":\"x\"}")
    t |> success(!pb.is_batch, "single object → is_batch=false")
    t |> equal(length(pb.requests), 1)
    t |> equal(pb.framing_error, "")
    t |> equal(pb.requests[0].method, "x")
    t |> equal(pb.requests[0].id_str, "1")
}

[test]
def test_batch_two_valid(t : T?) {
    let pb = parse_batch("[\{\"id\":1,\"method\":\"a\"},\{\"id\":2,\"method\":\"b\"}]")
    t |> success(pb.is_batch, "array → is_batch=true")
    t |> equal(length(pb.requests), 2)
    t |> equal(pb.framing_error, "")
    t |> equal(pb.requests[0].method, "a")
    t |> equal(pb.requests[0].id_str, "1")
    t |> equal(pb.requests[1].method, "b")
    t |> equal(pb.requests[1].id_str, "2")
}

[test]
def test_batch_empty_array(t : T?) {
    // §6: empty array → server responds with a SINGLE INVALID_REQUEST envelope, not an array.
    let pb = parse_batch("[]")
    t |> success(pb.is_batch, "is_batch=true for empty array")
    t |> success(empty(pb.requests), "no requests")
    t |> success(!empty(pb.framing_error), "framing_error populated")
    t |> success(find(pb.framing_error, "-32600") >= 0, "INVALID_REQUEST")
    t |> success(find(pb.framing_error, "empty batch") >= 0, "message names the rule")
}

[test]
def test_batch_top_level_parse_error(t : T?) {
    let pb = parse_batch("not json")
    t |> success(!pb.is_batch, "is_batch=false on top-level parse failure")
    t |> success(empty(pb.requests), "no requests")
    t |> success(!empty(pb.framing_error), "framing_error populated")
    t |> success(find(pb.framing_error, "-32700") >= 0, "PARSE_ERROR")
}

[test]
def test_batch_mixed_valid_invalid(t : T?) {
    let pb = parse_batch("[\{\"id\":1,\"method\":\"a\"},\{\"bad\":1},\{\"id\":3,\"method\":\"c\"}]")
    t |> success(pb.is_batch, "is_batch=true")
    t |> equal(length(pb.requests), 3)
    t |> equal(pb.framing_error, "")
    t |> equal(pb.requests[0].error_envelope, "")
    t |> success(!empty(pb.requests[1].error_envelope), "second entry rejected (missing method)")
    t |> equal(pb.requests[2].error_envelope, "")
}

[test]
def test_batch_all_notifications(t : T?) {
    // §6: all-notifications batch → server returns nothing (empty wire). parse_batch
    // doesn't drop anything itself; the caller's dispatcher decides what to write back.
    let pb = parse_batch("[\{\"method\":\"a\"},\{\"method\":\"b\"}]")
    t |> success(pb.is_batch, "is_batch=true")
    t |> equal(length(pb.requests), 2)
    t |> success(pb.requests[0].is_notification, "first is notification")
    t |> success(pb.requests[1].is_notification, "second is notification")
}

[test]
def test_batch_non_object_elements(t : T?) {
    // §6: non-object elements get individual INVALID_REQUEST envelopes; batch shape preserved.
    let pb = parse_batch("[1, 2, 3]")
    t |> success(pb.is_batch, "is_batch=true")
    t |> equal(length(pb.requests), 3)
    t |> equal(pb.framing_error, "")
    for (req in pb.requests) {
        t |> success(!empty(req.error_envelope), "each scalar element rejected")
        t |> success(find(req.error_envelope, "-32600") >= 0, "INVALID_REQUEST code")
    }
}

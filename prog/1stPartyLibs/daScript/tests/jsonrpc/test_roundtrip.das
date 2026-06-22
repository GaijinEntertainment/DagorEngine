options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== End-to-end round-trip: client builds → server parses → server responds → client parses =====

[test]
def test_roundtrip_simple_request(t : T?) {
    // Client side
    let wire = make_request("echo", "[\"hello\"]", 7)
    // Server side — parse + build response
    let req = parse_request(wire)
    t |> equal(req.error_envelope, "")
    t |> equal(req.method, "echo")
    t |> equal(req.id_str, "7")
    let server_response = response(req.id_str, req.params_json |> compact_json_whitespace())
    // Client side — parse response
    let resp = parse_response(server_response)
    t |> equal(resp.parse_error, "")
    t |> success(resp.is_success, "")
    t |> equal(resp.id_str, "7")
    t |> equal(resp.result_json |> compact_json_whitespace(), "[\"hello\"]")
}

[test]
def test_roundtrip_notification_no_response(t : T?) {
    let wire = make_notification("log", "\{\"level\":\"info\"}")
    let req = parse_request(wire)
    t |> equal(req.error_envelope, "")
    t |> success(req.is_notification, "is_notification=true")
    // Server emits nothing for a notification — no parse_response needed.
}

[test]
def test_roundtrip_method_not_found_error(t : T?) {
    let wire = make_request("missing_method", "null", 1)
    let req = parse_request(wire)
    t |> equal(req.error_envelope, "")
    // Server decides method is not in its dispatch table → emit error envelope.
    let server_response = error(req.id_str, METHOD_NOT_FOUND, "method not found")
    // Client parses it as error
    let resp = parse_response(server_response)
    t |> success(!resp.is_success, "is_success=false")
    t |> equal(resp.error_code, METHOD_NOT_FOUND)
    t |> equal(resp.error_msg, "method not found")
    t |> equal(resp.id_str, "1")
}

[test]
def test_roundtrip_batch(t : T?) {
    // Client side — build batch
    let msgs <- [make_request("a", "null", 1), make_request("b", "null", 2)]
    let wire = make_batch(msgs)
    // Server side — parse_batch + emit responses
    let pb = parse_batch(wire)
    t |> success(pb.is_batch, "")
    t |> equal(length(pb.requests), 2)
    var responses : array<string>
    responses |> reserve(length(pb.requests))
    for (req in pb.requests) {
        responses |> push(response(req.id_str, "\"ok-{req.method}\""))
    }
    let server_wire = make_batch(responses)
    // Client side — parse_response_batch
    let rb = parse_response_batch(server_wire)
    t |> success(rb.is_batch, "")
    t |> equal(length(rb.responses), 2)
    t |> equal(rb.responses[0].id_str, "1")
    t |> equal(rb.responses[1].id_str, "2")
    t |> success(rb.responses[0].is_success, "")
    t |> success(rb.responses[1].is_success, "")
}

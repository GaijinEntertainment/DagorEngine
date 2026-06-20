options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require strings

// ===== High-level dispatch_line convenience =====

[test]
def test_dispatch_single_request(t : T?) {
    let r = dispatch_line("\{\"id\":1,\"method\":\"echo\",\"params\":[42]}", false) $(method, params_json) {
        return "\"got-{method}\""
    }
    t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"got-echo\"}")
}

[test]
def test_dispatch_single_notification(t : T?) {
    // Notification: dispatcher runs for side-effects but response is "".
    var dispatcher_called = false
    let r = dispatch_line("\{\"method\":\"fire\"}", false) $(method, params_json) {
        dispatcher_called = true
        return "\"unused\""
    }
    t |> equal(r, "")
    t |> success(dispatcher_called, "dispatcher invoked even for notification")
}

[test]
def test_dispatch_top_level_parse_error(t : T?) {
    var called = false
    let r = dispatch_line("garbage", false) $(method, params_json) {
        called = true
        return "x"
    }
    t |> success(!empty(r), "framing_error returned to wire")
    t |> success(find(r, "-32700") >= 0, "PARSE_ERROR")
    t |> success(!called, "dispatcher NOT called on parse failure")
}

[test]
def test_dispatch_batch_mixed(t : T?) {
    let r = dispatch_line("[\{\"id\":1,\"method\":\"a\"},\{\"method\":\"notif\"},\{\"id\":3,\"method\":\"b\"}]", false) $(method, params_json) {
        return "\"r-{method}\""
    }
    // Batch with one notification: response array has 2 entries (notification suppressed).
    t |> success(find(r, "\"id\":1") >= 0, "request 1 response present")
    t |> success(find(r, "\"id\":3") >= 0, "request 3 response present")
    t |> success(find(r, "\"id\":2") < 0, "no entry for notification (no id 2 existed anyway)")
    // Verify array wrapping (peek_data — O(1) per byte, one strlen total)
    peek_data(r) $(bytes) {
        t |> success(!empty(bytes) && bytes[0] == uint8('['), "wire begins with [")
        t |> success(!empty(bytes) && bytes[length(bytes) - 1] == uint8(']'), "wire ends with ]")
    }
    // Order preserved
    let pos1 = find(r, "r-a")
    let pos3 = find(r, "r-b")
    t |> success(pos1 >= 0 && pos3 > pos1, "input order preserved (a before b)")
}

[test]
def test_dispatch_batch_all_notifications(t : T?) {
    let r = dispatch_line("[\{\"method\":\"a\"},\{\"method\":\"b\"}]", false) $(method, params_json) {
        return "ignored"
    }
    t |> equal(r, "")
}

[test]
def test_dispatch_batch_per_entry_errors(t : T?) {
    // Mixed valid + malformed entries: batch returns array with per-entry error envelopes.
    let r = dispatch_line("[\{\"id\":1,\"method\":\"good\"},\{\"id\":2}]", false) $(method, params_json) {
        return "\"ok\""
    }
    t |> success(find(r, "\"id\":1") >= 0, "valid entry response present")
    t |> success(find(r, "\"id\":2") >= 0, "invalid entry error present (id preserved)")
    t |> success(find(r, "-32600") >= 0, "INVALID_REQUEST code in array")
}

[test]
def test_dispatch_strict_mode_propagates(t : T?) {
    let r = dispatch_line("\{\"id\":1,\"method\":\"x\"}", true) $(method, params_json) {
        return "\"ok\""
    }
    t |> success(find(r, "-32600") >= 0, "strict mode rejects missing jsonrpc field")
}

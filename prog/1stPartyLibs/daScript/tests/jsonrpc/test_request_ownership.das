options gen2
options persistent_heap
options force_inscope_pod
options no_unused_block_arguments = false
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/jsonrpc

let SINGLE = "\{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"echo\",\"params\":\{\"a\":[1,2,3],\"b\":\"text\"}}"
let BATCH = "[\{\"id\":1,\"method\":\"a\",\"params\":[1,2]},\{\"method\":\"n\"},\{\"id\":3,\"method\":\"b\"}]"
let MALFORMED = "\{\"id\":1,\"method\":42,\"params\":[1,2]}"

def echo_dispatch(line : string) : string {
    return dispatch_line(line, false) $(method, _params_json) {
        return "\{\"m\":\"{method}\"}"
    }
}

def dispatch_residue(t : T?; line : string) : uint64 {
    let warm = echo_dispatch(line)
    t |> success(!empty(warm), "the line produced a response")
    let before = heap_bytes_allocated()
    for (_ in range(8)) {
        let r = echo_dispatch(line)
        t |> success(!empty(r), "the line produced a response")
    }
    return heap_bytes_allocated() - before
}

[test]
def test_dispatch_line_owns_the_document(t : T?) {
    t |> run("a dispatched single request leaves nothing on the heap") @(t : T?) {
        t |> equal(dispatch_residue(t, SINGLE), 0ul, "no parse tree survives dispatch")
    }
    t |> run("a dispatched batch leaves nothing on the heap") @(t : T?) {
        t |> equal(dispatch_residue(t, BATCH), 0ul, "no parse tree survives dispatch")
    }
    t |> run("a malformed request leaves nothing on the heap") @(t : T?) {
        t |> equal(dispatch_residue(t, MALFORMED), 0ul, "no parse tree survives dispatch")
    }
    t |> run("unparseable input leaves nothing on the heap") @(t : T?) {
        t |> equal(dispatch_residue(t, "not json at all"), 0ul, "no parse tree survives dispatch")
    }
}

[test]
def test_response_parsers_own_their_document(t : T?) {
    t |> run("parse_response leaves nothing on the heap") @(t : T?) {
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            let r = parse_response("\{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\{\"x\":[1,2,3]}}")
            t |> equal(r.is_success, true)
        }
        t |> equal(heap_bytes_allocated(), before, "no parse tree survives parse_response")
    }
    t |> run("parse_response_batch leaves nothing on the heap") @(t : T?) {
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            var rb <- parse_response_batch("[\{\"id\":1,\"result\":1},\{\"id\":2,\"result\":[1,2]}]")
            t |> equal(length(rb.responses), 2)
            delete rb.responses
        }
        t |> equal(heap_bytes_allocated(), before, "no parse tree survives parse_response_batch")
    }
}

[test]
def test_free_request_ends_the_scope(t : T?) {
    t |> run("parse_request hands back an owned document") @(t : T?) {
        let before = heap_bytes_allocated()
        var req = parse_request(SINGLE)
        t |> success(req.document != null, "the request owns its parse tree")
        t |> success(req.params != null, "params is a view into it")
        t |> success(heap_bytes_allocated() > before, "the tree is on the heap")
        free_request(req)
        t |> success(req.document == null, "free_request clears the document")
        t |> success(req.params == null, "free_request clears the params view")
        t |> equal(heap_bytes_allocated(), before, "free_request releases the tree")
    }
    t |> run("the request strings outlive the document") @(t : T?) {
        var req = parse_request(SINGLE)
        free_request(req)
        t |> equal(req.method, "echo")
        t |> equal(req.id_str, "1")
        t |> equal(!empty(req.params_json), true)
    }
    t |> run("a failed parse owns nothing") @(t : T?) {
        let before = heap_bytes_allocated()
        var req = parse_request("not json at all")
        t |> success(req.document == null, "there is no document to own")
        t |> success(!empty(req.error_envelope), "the error envelope came back")
        free_request(req)
        t |> equal(heap_bytes_allocated(), before, "free_request on a failed parse is a no-op")
    }
    t |> run("repeated parse and free does not accumulate") @(t : T?) {
        var warm = parse_request(SINGLE)
        free_request(warm)
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            var req = parse_request(SINGLE)
            free_request(req)
        }
        t |> equal(heap_bytes_allocated(), before, "the scope closes every time")
    }
}

[test]
def test_free_batch_ends_the_scope(t : T?) {
    t |> run("parse_batch hands back one owned document for the whole batch") @(t : T?) {
        let before = heap_bytes_allocated()
        var pb <- parse_batch(BATCH)
        t |> success(pb.document != null, "the batch owns the parse tree")
        t |> equal(length(pb.requests), 3)
        for (req in pb.requests) {
            t |> success(req.document == null, "an entry never owns the document")
        }
        t |> success(pb.requests[0].params != null, "the first entry has a params view")
        free_batch(pb)
        t |> success(pb.document == null, "free_batch clears the document")
        t |> equal(heap_bytes_allocated(), before, "free_batch releases the tree and the entry array")
    }
    t |> run("repeated parse and free does not accumulate") @(t : T?) {
        var warm <- parse_batch(BATCH)
        free_batch(warm)
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            var pb <- parse_batch(BATCH)
            free_batch(pb)
        }
        t |> equal(heap_bytes_allocated(), before, "the scope closes every time")
    }
    t |> run("a framing error owns its document too") @(t : T?) {
        let before = heap_bytes_allocated()
        var pb <- parse_batch("[]")
        t |> success(!empty(pb.framing_error), "the empty batch was rejected")
        free_batch(pb)
        t |> equal(heap_bytes_allocated(), before, "free_batch releases the tree")
    }
}

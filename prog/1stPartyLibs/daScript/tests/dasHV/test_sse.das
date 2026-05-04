// Tests for SSE (Server-Sent Events) support
//
// Covers: server-side SSE handler, client-side streaming via request_cb

options gen2
options persistent_heap
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require _dashv_test_common
require strings

// Helper: format an SSE event as "event: {event}\ndata: {data}\n\n"
def sse_format(data, event : string) : string {
    return "event: {event}\ndata: {data}\n\n"
}

// ──────────────────────────────────────────────────────────────────────────
// SSE test server
// ──────────────────────────────────────────────────────────────────────────

class SseTestServer : HvWebServer {
    def override onInit {
        // SSE endpoint: sends 3 events with SSE-formatted body
        SSE("/events") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Content-Type", "text/event-stream")
            set_header(resp, "Cache-Control", "no-cache")
            var body = sse_format("hello", "message")
            body += sse_format("world", "message")
            body += sse_format("done", "complete")
            return resp |> TEXT_PLAIN(body)
        }
        // Echo endpoint for testing request_cb with POST
        SSE("/stream-echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Content-Type", "text/event-stream")
            return resp |> TEXT_PLAIN(sse_format(string(req.body), "echo"))
        }
        // Plain text endpoint
        GET("/plain") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("hello plain")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Tests
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_sse_basic(t : T?) {
    with_test_server(type<SseTestServer>, PORT_SSE) <| $(base_url) {
        t |> run("SSE endpoint returns event-stream content type") @(t : T?) {
            GET("{base_url}/events") <| $(var resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                let ct = get_header(resp, "Content-Type")
                t |> success(find(string(ct), "text/event-stream") >= 0, "Content-Type is event-stream")
            }
        }
        t |> run("SSE response body contains events") @(t : T?) {
            GET("{base_url}/events") <| $(var resp) {
                let body = string(resp.body)
                t |> success(find(body, "data: hello") >= 0, "body contains 'data: hello'")
                t |> success(find(body, "data: world") >= 0, "body contains 'data: world'")
                t |> success(find(body, "data: done") >= 0, "body contains 'data: done'")
                t |> success(find(body, "event: message") >= 0, "body contains 'event: message'")
                t |> success(find(body, "event: complete") >= 0, "body contains 'event: complete'")
            }
        }
    }
}

[test]
def test_request_cb_streaming(t : T?) {
    with_test_server(type<SseTestServer>, PORT_SSE) <| $(base_url) {
        t |> run("request_cb receives body chunks") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/events"
                var chunks : array<string>
                request_cb(req, $(data : string) {
                    chunks |> push(clone_string(data))
                }) <| $(var resp) {
                    t |> equal(int(resp.status_code), int(http_status.OK))
                }
                // We should have received at least one chunk
                t |> success(length(chunks) > 0, "received {length(chunks)} chunk(s)")
                // All chunks concatenated should contain our events
                var full = ""
                for (chunk in chunks) {
                    full = "{full}{chunk}"
                }
                t |> success(find(full, "data: hello") >= 0, "chunks contain 'data: hello'")
                t |> success(find(full, "data: done") >= 0, "chunks contain 'data: done'")
            }
        }
        t |> run("request_cb raw bytes overload") @(t2 : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/events"
                var total_bytes = 0
                request_cb(req, $(data; var size : int) {
                    total_bytes += size
                }) <| $(var resp : HttpResponse?) {
                    t2 |> equal(int(resp.status_code), int(http_status.OK))
                }
                t2 |> success(total_bytes > 0, "received {total_bytes} bytes via raw callback")
            }
        }
    }
}

[test]
def test_sse_echo(t : T?) {
    with_test_server(type<SseTestServer>, PORT_SSE) <| $(base_url) {
        t |> run("SSE echo endpoint returns posted body as event") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/stream-echo"
                req.body := "test-payload"
                set_content_type(req, "text/plain")
                var chunks : array<string>
                request_cb(req, $(data : string) {
                    chunks |> push(clone_string(data))
                }) <| $(var resp) {
                    t |> equal(int(resp.status_code), int(http_status.OK))
                }
                var full = ""
                for (chunk in chunks) {
                    full = "{full}{chunk}"
                }
                t |> success(find(full, "data: test-payload") >= 0, "echo contains posted body")
                t |> success(find(full, "event: echo") >= 0, "echo has correct event type")
            }
        }
    }
}

[test]
def test_request_cb_plain(t : T?) {
    with_test_server(type<SseTestServer>, PORT_SSE) <| $(base_url) {
        t |> run("request_cb works with plain GET endpoint") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/plain"
                var chunks : array<string>
                request_cb(req, $(data : string) {
                    chunks |> push(clone_string(data))
                }) <| $(var resp) {
                    t |> equal(int(resp.status_code), int(http_status.OK))
                }
                var full = ""
                for (chunk in chunks) {
                    full = "{full}{chunk}"
                }
                t |> success(find(full, "hello plain") >= 0, "received plain body via request_cb")
            }
        }
    }
}

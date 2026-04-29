// Tutorial HV-07: SSE and Streaming
//
// This tutorial covers:
//   - Server-Sent Events (SSE) — what they are and when to use them
//   - Server-side: registering an SSE endpoint with HvWebServer.SSE()
//   - Client-side: streaming body chunks with request_cb
//   - request_cb with raw bytes (uint8? + int) vs string callback
//   - Building an SSE line parser on top of request_cb
//
// Prerequisites: Tutorials HV-01 through HV-03 (HTTP client and server)
//
// SSE is a standard protocol for server-to-client streaming over HTTP.
// The server sends "event: name\ndata: payload\n\n" lines with
// Content-Type: text/event-stream.  Unlike WebSockets, SSE is one-way
// (server→client) and uses plain HTTP — no upgrade handshake needed.
//
// Run: daslang.exe tutorials/dasHV/07_sse_and_streaming.das

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require daslib/jobque_boost
require strings
require daslib/fio

let SERVER_PORT = 18087

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Server-side SSE endpoint
// ──────────────────────────────────────────────────────────────────────────
//
// SSE() registers a handler that matches any HTTP method, just like ANY().
// The handler has the same signature as GET/POST — it receives (req, resp)
// and returns an http_status.  You set the SSE headers and write the
// event-formatted body into the response.
//
// The SSE wire format is:
//   event: <event-type>\n
//   data: <payload>\n
//   \n
//
// Helper: format one SSE event as a string.

def sse_event_string(data, event : string) : string {
    return "event: {event}\ndata: {data}\n\n"
}

class SseTutorialServer : HvWebServer {
    def override onInit {

        // --- Section 1: basic SSE endpoint ---
        // Sends three events and closes the response.
        SSE("/events") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Content-Type", "text/event-stream")
            set_header(resp, "Cache-Control", "no-cache")
            var body = sse_event_string("first", "message")
            body += sse_event_string("second", "message")
            body += sse_event_string("all done", "complete")
            return resp |> TEXT_PLAIN(body)
        }

        // --- Section 3: echo endpoint for POST streaming ---
        SSE("/stream-echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Content-Type", "text/event-stream")
            return resp |> TEXT_PLAIN(sse_event_string(string(req.body), "echo"))
        }

        // --- Section 4: plain text endpoint for request_cb comparison ---
        GET("/plain") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("hello from plain endpoint")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Server lifecycle helper (same pattern as other tutorials)
// ──────────────────────────────────────────────────────────────────────────

def with_server(port : int; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new SseTutorialServer()
                    server->init(port)
                    server->start()
                    started |> notify_and_release
                    while (stop_flag |> get == 0) {
                        server->tick()
                        sleep(10u)
                    }
                    server->stop()
                    unsafe {
                        delete server
                    }
                    finished |> notify_and_release
                }
                started |> join
                let base_url = "http://127.0.0.1:{port}"
                invoke(blk, base_url)
                stop_flag |> set(1)
                finished |> join
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Client-side streaming with request_cb
// ──────────────────────────────────────────────────────────────────────────
//
// request_cb works like request() but calls a per-chunk callback as body
// data arrives.  This is the building block for consuming SSE streams.
//
// Two overloads:
//   request_cb(req, $(data : string#) { ... })       — string chunks
//   request_cb(req, $(data; var size : int) { ... })  — raw uint8? + size
//
// The string overload is convenient for text protocols like SSE.
// The raw overload is useful for binary data or when you need exact sizes.

def example_string_streaming(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/events"
        var chunks : array<string>
        request_cb(req, $(data : string) {
            chunks |> push(clone_string(data))
        }) <| $(var resp) {
            print("  status: {int(resp.status_code)}\n")
        }
        print("  received {length(chunks)} chunk(s)\n")
        for (chunk in chunks) {
            print("  chunk: \"{chunk}\"\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Raw bytes streaming
// ──────────────────────────────────────────────────────────────────────────
//
// The raw callback receives (uint8? data, int size).  This is useful for
// binary protocols or when you need to track exact byte counts.

def example_raw_streaming(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/events"
        var total_bytes = 0
        request_cb(req, $(data; var size : int) {
            total_bytes += size
        }) <| $(var resp : HttpResponse?) {
            print("  status: {int(resp.status_code)}\n")
        }
        print("  received {total_bytes} total bytes\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Streaming a POST request
// ──────────────────────────────────────────────────────────────────────────
//
// request_cb works with any HTTP method.  Here we POST a body and stream
// the SSE response.

def example_post_streaming(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.POST
        req.url := "{base_url}/stream-echo"
        req.body := "hello from POST"
        set_content_type(req, "text/plain")
        var chunks : array<string>
        request_cb(req, $(data : string) {
            chunks |> push(clone_string(data))
        }) <| $(var resp) {
            print("  status: {int(resp.status_code)}\n")
        }
        var full = ""
        for (chunk in chunks) {
            full = "{full}{chunk}"
        }
        print("  response: {full}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — request_cb with a non-SSE endpoint
// ──────────────────────────────────────────────────────────────────────────
//
// request_cb works with any HTTP endpoint — it's not SSE-specific.
// The callback fires for every body chunk regardless of Content-Type.

def example_plain_streaming(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/plain"
        var chunks : array<string>
        request_cb(req, $(data : string) {
            chunks |> push(clone_string(data))
        }) <| $(var resp) {
            print("  status: {int(resp.status_code)}\n")
        }
        var full = ""
        for (chunk in chunks) {
            full = "{full}{chunk}"
        }
        print("  body: {full}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Simple SSE event parser
// ──────────────────────────────────────────────────────────────────────────
//
// SSE events are line-based.  A minimal parser accumulates chunks into a
// line buffer, splits on "\n", and extracts "event:" and "data:" fields.
// A blank line signals the end of one event.

struct SseEvent {
    event : string
    data : string
}

def parse_sse_events(body : string) : array<SseEvent> {
    var events : array<SseEvent>
    var current_event = ""
    var current_data = ""
    peek_data(body) $(bytes) {
        var pos = 0
        let len = length(bytes)
        while (pos < len) {
            // Find end of line
            var eol = pos
            while (eol < len && int(bytes[eol]) != '\n') {
                eol++
            }
            let line_len = eol - pos
            if (line_len == 0) {
                // Blank line = end of event
                if (!empty(current_data) || !empty(current_event)) {
                    events |> emplace(SseEvent(event = current_event, data = current_data))
                    current_event = ""
                    current_data = ""
                }
            } else {
                let line = slice(body, pos, eol)
                if (starts_with(line, "event: ")) {
                    current_event = slice(line, 7)
                } elif (starts_with(line, "data: ")) {
                    current_data = slice(line, 6)
                }
            }
            pos = eol + 1
        }
    }
    return <- events
}

def example_parse_sse(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/events"
        var chunks : array<string>
        request_cb(req, $(data : string) {
            chunks |> push(clone_string(data))
        }) <| $(var resp) {
            assert(resp.status_code == http_status.OK)
        }
        // Concatenate all chunks
        var full = ""
        for (chunk in chunks) {
            full = "{full}{chunk}"
        }
        // Parse SSE events
        var events <- parse_sse_events(full)
        print("  parsed {length(events)} event(s):\n")
        for (evt in events) {
            print("    [{evt.event}] {evt.data}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Quick reference
// ──────────────────────────────────────────────────────────────────────────
//
//  Server-side:
//    SSE(uri) <| @(req, resp) { ... }   Register SSE endpoint (any method)
//    set_header(resp, k, v)             Set response header
//    resp |> TEXT_PLAIN(body)            Set body + return 200
//
//  Client-side:
//    request_cb(req, on_chunk) <| $(resp) { ... }
//      on_chunk overloads:
//        $(data : string)               String chunk callback
//        $(data; var size : int)         Raw bytes callback (uint8? + size)
//
//  SSE wire format:
//    event: <type>\n
//    data: <payload>\n
//    \n                                 (blank line = event boundary)

[export]
def main() {
    with_server(SERVER_PORT) <| $(base_url) {
        print("=== Section 1 & 2: SSE endpoint + string streaming ===\n")
        example_string_streaming(base_url)

        print("\n=== Section 3: Raw bytes streaming ===\n")
        example_raw_streaming(base_url)

        print("\n=== Section 4: POST streaming ===\n")
        example_post_streaming(base_url)

        print("\n=== Section 5: request_cb with plain endpoint ===\n")
        example_plain_streaming(base_url)

        print("\n=== Section 6: SSE event parsing ===\n")
        example_parse_sse(base_url)
    }
}

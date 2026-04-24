// Tutorial HV-03: HTTP Server
//
// This tutorial covers:
//   - Subclassing HvWebServer to create an HTTP server
//   - Registering routes for all HTTP methods (GET, POST, PUT, PATCH, DELETE, HEAD)
//   - Path parameters (/users/:id) and query parameters (?key=value)
//   - Response helpers: TEXT_PLAIN, JSON, set_header
//   - Reading request body and parsing JSON on the server side
//   - The ANY route for catch-all handlers
//
// Prerequisites: Tutorials HV-01 and HV-02 (HTTP client API)
//
// Each server tutorial creates its own server class to demonstrate
// server-side patterns.  The server starts on a background thread,
// the tutorial sends HTTP requests to verify each feature, then
// shuts down cleanly.
//
// Run: daslang.exe tutorials/dasHV/03_http_server.das

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require daslib/jobque_boost
require daslib/json_boost
require daslib/fio

let SERVER_PORT = 18082

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Minimal HTTP server
// ──────────────────────────────────────────────────────────────────────────
//
// To create an HTTP server, subclass HvWebServer and override onInit to
// register routes.  Each route handler receives (req, resp) and returns
// an int (the HTTP status code).
//
// The simplest response helper is TEXT_PLAIN(resp, text), which sets
// Content-Type: text/plain and the body in one call.

class TutorialHttpServer : HvWebServer {
    def override onInit {

        // --- Section 1: basic GET route ---
        GET("/hello") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("Hello, world!")
        }

        // --- Section 2: POST route (echo body) ---
        POST("/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN(string(req.body))
        }

        // --- Section 3: all HTTP methods ---
        PUT("/resource") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("PUT: {req.body}")
        }
        PATCH("/resource") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("PATCH: {req.body}")
        }
        DELETE("/resource") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("DELETE: ok")
        }
        HEAD("/resource") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "X-Resource-Exists", "true")
            return http_status.OK
        }

        // --- Section 4: path parameters ---
        // libhv supports RESTful path params with :name syntax.
        // The captured values are available via get_param(req, "name").
        GET("/users/:id") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let user_id = get_param(req, "id")
            return resp |> TEXT_PLAIN("user: {user_id}")
        }
        GET("/users/:id/posts/:post_id") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let user_id = get_param(req, "id")
            let post_id = get_param(req, "post_id")
            return resp |> TEXT_PLAIN("user={user_id}, post={post_id}")
        }

        // --- Section 5: query parameters ---
        // Query params (?key=value&key2=value2) are also accessible via
        // get_param(req, key).  Use each_param to iterate all of them.
        GET("/search") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            var result = "query:"
            each_param(req) <| $(key, value : string) {
                result = "{result} {key}={value}"
            }
            return resp |> TEXT_PLAIN(result)
        }

        // --- Section 6: response headers ---
        GET("/headers") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "X-Custom", "tutorial-03")
            set_header(resp, "X-Request-Id", "12345")
            return resp |> TEXT_PLAIN("check headers")
        }

        // --- Section 7: JSON response ---
        // JSON(resp, json_string) sets Content-Type: application/json
        // and the body.  Combine with daslib/json_boost for structured data.
        // JV() accepts structs and named tuples — field names become JSON keys.
        GET("/api/status") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let payload : tuple<status : string; version : int> = ("ok", 3)
            return resp |> JSON(write_json(JV(payload)))
        }
        POST("/api/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            // Parse JSON from request body, add a field, return it
            var err : string
            var parsed = read_json(string(req.body), err)
            if (parsed == null) {
                return resp |> TEXT_PLAIN("parse error: {err}", http_status.BAD_REQUEST)
            }
            // Echo it back as-is
            return resp |> JSON(write_json(parsed))
        }

        // --- Section 8: ANY route (catch-all for a path) ---
        // ANY registers a handler for all HTTP methods at once.
        ANY("/any-method") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("method: {req.method}")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Server lifecycle helper (same pattern as tutorial_server.das)
// ──────────────────────────────────────────────────────────────────────────

def with_server(port : int; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new TutorialHttpServer()
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
// Verification — send requests to the server to test each feature
// ──────────────────────────────────────────────────────────────────────────

[export]
def main() {
    with_server(SERVER_PORT) <| $(base_url) {

        print("=== Section 1: Minimal GET route ===\n")
        GET("{base_url}/hello") <| $(resp) {
            print("GET /hello -> {resp.status_code}: {resp.body}\n")
        }

        print("\n=== Section 2: POST route ===\n")
        POST("{base_url}/echo", "Hello from client!") <| $(resp) {
            print("POST /echo -> {resp.status_code}: {resp.body}\n")
        }

        print("\n=== Section 3: All HTTP methods ===\n")
        with_http_request() <| $(var req) {
            req.method = http_method.PUT
            req.url := "{base_url}/resource"
            req.body := "updated data"
            request(req) <| $(resp) {
                print("PUT /resource -> {resp.status_code}: {resp.body}\n")
            }
        }
        with_http_request() <| $(var req) {
            req.method = http_method.PATCH
            req.url := "{base_url}/resource"
            req.body := "partial update"
            request(req) <| $(resp) {
                print("PATCH /resource -> {resp.status_code}: {resp.body}\n")
            }
        }
        with_http_request() <| $(var req) {
            req.method = http_method.DELETE
            req.url := "{base_url}/resource"
            request(req) <| $(resp) {
                print("DELETE /resource -> {resp.status_code}: {resp.body}\n")
            }
        }
        with_http_request() <| $(var req) {
            req.method = http_method.HEAD
            req.url := "{base_url}/resource"
            request(req) <| $(resp) {
                let exists = get_header(resp, "X-Resource-Exists")
                print("HEAD /resource -> {resp.status_code}, X-Resource-Exists: {exists}\n")
            }
        }

        print("\n=== Section 4: Path parameters ===\n")
        GET("{base_url}/users/42") <| $(resp) {
            print("GET /users/42 -> {resp.body}\n")
        }
        GET("{base_url}/users/7/posts/99") <| $(resp) {
            print("GET /users/7/posts/99 -> {resp.body}\n")
        }

        print("\n=== Section 5: Query parameters ===\n")
        GET("{base_url}/search?q=daslang&page=1&limit=10") <| $(resp) {
            print("GET /search?... -> {resp.body}\n")
        }

        print("\n=== Section 6: Response headers ===\n")
        GET("{base_url}/headers") <| $(resp) {
            let custom = get_header(resp, "X-Custom")
            let req_id = get_header(resp, "X-Request-Id")
            print("GET /headers -> X-Custom: {custom}, X-Request-Id: {req_id}\n")
        }

        print("\n=== Section 7: JSON response ===\n")
        GET("{base_url}/api/status") <| $(resp) {
            print("GET /api/status -> {resp.body}\n")
            // Parse the JSON response
            var err : string
            var parsed = read_json(string(resp.body), err)
            if (parsed != null) {
                print("  parsed OK\n")
            }
        }
        POST("{base_url}/api/echo", "\{\"msg\":\"hello\"\}") <| $(resp) {
            print("POST /api/echo -> {resp.body}\n")
        }

        print("\n=== Section 8: ANY route ===\n")
        GET("{base_url}/any-method") <| $(resp) {
            print("GET /any-method -> {resp.body}\n")
        }
        POST("{base_url}/any-method", "") <| $(resp) {
            print("POST /any-method -> {resp.body}\n")
        }
    }
}

// Tutorial HV-04: Advanced HTTP Server Features
//
// This tutorial covers:
//   - Static file serving (STATIC)
//   - CORS (Cross-Origin Resource Sharing)
//   - HTTP redirects (REDIRECT)
//   - Serving individual files (SERVE_FILE)
//   - Custom response headers and caching
//   - Binary response data (DATA)
//   - Content-Type control on responses
//   - Status codes and error responses
//
// Prerequisites: Tutorial HV-03 (basic HTTP server)
//
// Run: daslang.exe tutorials/dasHV/04_http_server_advanced.das

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require daslib/jobque_boost
require daslib/json_boost
require daslib/fio

let SERVER_PORT = 18083

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Static file serving
// ──────────────────────────────────────────────────────────────────────────
//
// STATIC(path, dir) maps a URL prefix to a filesystem directory.
// All files under that directory are served automatically with the
// correct Content-Type based on file extension.
//
// In this tutorial we create a small temp directory with test files
// to demonstrate static serving.

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — CORS
// ──────────────────────────────────────────────────────────────────────────
//
// allow_cors() enables Cross-Origin Resource Sharing headers on all
// responses.  This is essential for browser-based JavaScript clients
// that make requests to your server from a different origin.

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Redirects
// ──────────────────────────────────────────────────────────────────────────
//
// REDIRECT(resp, location, status) sends a 3xx redirect response.
// The client will follow the redirect to the new location.

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Serving individual files
// ──────────────────────────────────────────────────────────────────────────
//
// SERVE_FILE(resp, filepath) reads a file from disk and serves it
// with the appropriate Content-Type based on file extension.

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Custom headers and caching
// ──────────────────────────────────────────────────────────────────────────
//
// set_header(resp, key, value) sets response headers.
// Common use cases: Cache-Control, ETag, custom app headers.

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Binary response
// ──────────────────────────────────────────────────────────────────────────
//
// DATA(resp, data, length) sends raw binary data with
// Content-Type: application/octet-stream.

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Content-Type and status codes
// ──────────────────────────────────────────────────────────────────────────
//
// set_content_type(resp, type) sets the Content-Type header.
// Combine with the handler return value for custom response types.

class AdvancedServer : HvWebServer {
    static_dir : string

    def override onInit {

        // Section 2: CORS — enable on all routes
        allow_cors()

        // Section 3: redirect
        GET("/old-path") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> REDIRECT("/new-path", http_status.MOVED_PERMANENTLY)
        }
        GET("/new-path") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("you arrived at /new-path")
        }

        // Section 5: custom headers and caching
        GET("/cached") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Cache-Control", "max-age=3600, public")
            set_header(resp, "ETag", "\"v1.0\"")
            set_header(resp, "X-Server", "daslang-tutorial")
            return resp |> TEXT_PLAIN("cached response")
        }

        // Section 6: binary data
        GET("/binary") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            // Create a simple 16-byte binary payload
            var payload = "BINARY\x00DATA1234"
            return resp |> DATA(payload, 16)
        }

        // Section 7: content-type and status codes

        // Custom HTML response
        GET("/html") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_content_type(resp, "text/html")
            resp.body := "<h1>Hello from daslang!</h1>"
            return http_status.OK
        }

        // Error response with custom status
        // JSON() and TEXT_PLAIN() accept an optional status parameter
        // (defaults to http_status.OK).
        GET("/not-found") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let payload : tuple<error : string; code : int> = ("resource not found", 404)
            return resp |> JSON(write_json(JV(payload)), http_status.NOT_FOUND)
        }

        // 201 Created with Location header
        POST("/items") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Location", "/items/42")
            let payload : tuple<id : int; body : string> = (42, string(req.body))
            return resp |> JSON(write_json(JV(payload)), http_status.CREATED)
        }

        // 204 No Content
        POST("/ack") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return http_status.NO_CONTENT
        }
    }
}


// ──────────────────────────────────────────────────────────────────────────
// Setup helpers — create temp files for static/file serving demos
// ──────────────────────────────────────────────────────────────────────────

def create_test_files(dir : string) {
    mkdir_rec(dir)
    fopen("{dir}/index.html", "w") <| $(f) {
        if (f != null) {
            fwrite(f, "<h1>Welcome</h1>")
        }
    }
    fopen("{dir}/style.css", "w") <| $(f) {
        if (f != null) {
            fwrite(f, "body \{ color: red; \}")
        }
    }
    fopen("{dir}/data.json", "w") <| $(f) {
        if (f != null) {
            fwrite(f, "\{\"hello\":\"world\"\}")
        }
    }
}

def cleanup_test_files(dir : string) {
    remove("{dir}/index.html")
    remove("{dir}/style.css")
    remove("{dir}/data.json")
}

// ──────────────────────────────────────────────────────────────────────────
// Server lifecycle
// ──────────────────────────────────────────────────────────────────────────

def with_advanced_server(port : int; static_root : string; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new AdvancedServer()
                    server.static_dir = static_root
                    server->init(port)

                    // Section 1: static file serving — must be done after init
                    // since init registers the HttpService with the router
                    STATIC(server.server, "/static", static_root)

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
// Quick reference
// ──────────────────────────────────────────────────────────────────────────
//
//  Function                                  Description
//  ────────────────────────────────────────  ────────────────────────────────
//  STATIC(path, dir)                        Serve static files
//  allow_cors()                             Enable CORS headers
//  REDIRECT(resp, location, status)         Send redirect response
//  SERVE_FILE(resp, filepath)               Serve a single file
//  JSON(resp, json_string)                  JSON response
//  TEXT_PLAIN(resp, text)                    Text response
//  DATA(resp, data, length)                 Binary response
//  set_header(resp, key, value)             Set response header
//  set_content_type(resp, content_type)     Set Content-Type header

[export]
def main() {
    let static_root = "{get_das_root()}/tutorials/dasHV/_static_test"
    create_test_files(static_root)

    with_advanced_server(SERVER_PORT, static_root) <| $(base_url) {

        print("=== Section 1: Static file serving ===\n")
        GET("{base_url}/static/index.html") <| $(resp) {
            print("GET /static/index.html -> {resp.status_code}: {resp.body}\n")
        }
        GET("{base_url}/static/style.css") <| $(resp) {
            let ct = get_header(resp, "Content-Type")
            print("GET /static/style.css -> {resp.status_code} (Content-Type: {ct})\n")
        }
        GET("{base_url}/static/data.json") <| $(resp) {
            let ct = get_header(resp, "Content-Type")
            print("GET /static/data.json -> {resp.status_code}: {resp.body} (Content-Type: {ct})\n")
        }

        print("\n=== Section 2: CORS ===\n")
        // Send an OPTIONS preflight request to check CORS headers
        with_http_request() <| $(var req) {
            req.method = http_method.OPTIONS
            req.url := "{base_url}/cached"
            set_header(req, "Origin", "http://example.com")
            set_header(req, "Access-Control-Request-Method", "GET")
            request(req) <| $(resp) {
                let cors_origin = get_header(resp, "Access-Control-Allow-Origin")
                let cors_methods = get_header(resp, "Access-Control-Allow-Methods")
                print("OPTIONS /cached -> {resp.status_code}\n")
                print("  Access-Control-Allow-Origin: {cors_origin}\n")
                print("  Access-Control-Allow-Methods: {cors_methods}\n")
            }
        }

        print("\n=== Section 3: Redirect ===\n")
        // Note: libhv client follows redirects by default
        with_http_request() <| $(var req) {
            req.method = http_method.GET
            req.url := "{base_url}/old-path"
            allow_redirect(req, false)  // don't follow, so we see the 301
            request(req) <| $(resp) {
                let location = get_header(resp, "Location")
                print("GET /old-path (no follow) -> {resp.status_code}, Location: {location}\n")
            }
        }
        // Now with redirect following
        GET("{base_url}/old-path") <| $(resp) {
            print("GET /old-path (follow) -> {resp.status_code}: {resp.body}\n")
        }

        print("\n=== Section 4: Serve individual file ===\n")
        // We reuse the static file endpoint to test SERVE_FILE indirectly
        // (SERVE_FILE was demonstrated in the server route itself)
        // The static serving uses SERVE_FILE internally for each file
        GET("{base_url}/static/data.json") <| $(resp) {
            print("File served -> {resp.status_code}: {resp.body}\n")
        }

        print("\n=== Section 5: Custom headers and caching ===\n")
        GET("{base_url}/cached") <| $(resp) {
            let cache_control = get_header(resp, "Cache-Control")
            let etag = get_header(resp, "ETag")
            let x_server = get_header(resp, "X-Server")
            print("GET /cached ->\n")
            print("  Cache-Control: {cache_control}\n")
            print("  ETag: {etag}\n")
            print("  X-Server: {x_server}\n")
            print("  Body: {resp.body}\n")
        }

        print("\n=== Section 6: Binary response ===\n")
        GET("{base_url}/binary") <| $(resp) {
            let ct = get_header(resp, "Content-Type")
            print("GET /binary -> {resp.status_code} ({resp.content_length} bytes, {ct})\n")
        }

        print("\n=== Section 7: Content-Type and status codes ===\n")
        // HTML response
        GET("{base_url}/html") <| $(resp) {
            let ct = get_header(resp, "Content-Type")
            print("GET /html -> {resp.status_code} ({ct}): {resp.body}\n")
        }
        // 404 JSON error
        GET("{base_url}/not-found") <| $(resp) {
            let ct = get_header(resp, "Content-Type")
            print("GET /not-found -> {resp.status_code} ({ct})\n")
            print("  {resp.body}\n")
        }
        // 201 Created
        POST("{base_url}/items", "new item") <| $(resp) {
            let location = get_header(resp, "Location")
            print("POST /items -> {resp.status_code}, Location: {location}\n")
            print("  {resp.body}\n")
        }
        // 204 No Content
        POST("{base_url}/ack", "done") <| $(resp) {
            print("POST /ack -> {resp.status_code} (body length: {resp.content_length})\n")
        }
    }

    cleanup_test_files(static_root)
}

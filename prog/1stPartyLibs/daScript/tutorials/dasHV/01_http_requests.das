// Tutorial HV-01: Simple HTTP Requests
//
// This tutorial covers:
//   - Making GET and POST requests with fire-and-forget API
//   - PUT, PATCH, DELETE, and HEAD requests
//   - Passing custom headers
//   - Reading response status, body, and headers
//
// Prerequisites: A basic understanding of HTTP (methods, headers, status codes)
//
// The fire-and-forget API creates a new connection per request — ideal for
// scripts, tools, and any code where connection reuse is not critical.
// For persistent connections and advanced configuration, see Tutorial HV-02.
//
// Run: daslang.exe tutorials/dasHV/01_http_requests.das

options gen2
options persistent_heap
options gc

require tutorial_server

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — GET requests
// ──────────────────────────────────────────────────────────────────────────
//
// The simplest HTTP call. GET retrieves a resource — it has no body.
// The response is delivered through a block.

def example_get(base_url : string) {
    // Simple GET — just a URL
    GET("{base_url}/ping") <| $(resp) {
        print("GET /ping -> {resp.status_code}: {resp.body}\n")
    }
    // GET with custom headers
    GET("{base_url}/ping", {"Accept" => "text/plain", "X-Request-Id" => "42"}) <| $(resp) {
        print("GET /ping (with headers) -> {resp.status_code}: {resp.body}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — POST requests
// ──────────────────────────────────────────────────────────────────────────
//
// POST sends a body to the server. Commonly used for creating resources
// or submitting data.

def example_post(base_url : string) {
    // Simple POST with a text body
    POST("{base_url}/echo", "hello from daslang!") <| $(resp) {
        print("POST /echo -> {resp.status_code}: {resp.body}\n")
    }
    // POST with JSON body and Content-Type header
    let json_body = "\{\"message\":\"hello\"\}"
    POST("{base_url}/echo", json_body, {"Content-Type" => "application/json"}) <| $(resp) {
        print("POST /echo (JSON) -> {resp.status_code}: {resp.body}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — PUT and PATCH requests
// ──────────────────────────────────────────────────────────────────────────
//
// PUT replaces a resource entirely. PATCH applies a partial update.
// Both take a URL and body, like POST.
//
// Note: our test server only registers GET and POST routes, so PUT and
// PATCH will return 501 Not Implemented.  The client calls themselves
// work correctly — responses are just whatever the server returns.

def example_put_patch(base_url : string) {
    PUT("{base_url}/echo", "full replacement data") <| $(resp) {
        print("PUT /echo -> {resp.status_code}\n")
    }
    PUT("{base_url}/echo", "replacement with header", {"Content-Type" => "text/plain"}) <| $(resp) {
        print("PUT /echo (headers) -> {resp.status_code}\n")
    }
    PATCH("{base_url}/echo", "partial update") <| $(resp) {
        print("PATCH /echo -> {resp.status_code}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — DELETE and HEAD requests
// ──────────────────────────────────────────────────────────────────────────
//
// DELETE removes a resource. HEAD is like GET but returns only headers,
// no body — useful for checking if a resource exists or getting its size.
//
// Note: our test server does not route DELETE or HEAD, so these return
// 501 / 404.  The client API is the same pattern as GET and POST.

def example_delete_head(base_url : string) {
    DELETE("{base_url}/data") <| $(resp) {
        print("DELETE /data -> {resp.status_code}\n")
    }
    DELETE("{base_url}/data", {"Authorization" => "Bearer token123"}) <| $(resp) {
        print("DELETE /data (auth) -> {resp.status_code}\n")
    }
    HEAD("{base_url}/ping") <| $(resp) {
        print("HEAD /ping -> {resp.status_code}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Reading response headers
// ──────────────────────────────────────────────────────────────────────────
//
// Use get_header to read a specific header, or each_header to iterate
// over all response headers.

def example_response_headers(base_url : string) {
    GET("{base_url}/ping") <| $(resp) {
        // Get a specific header
        let ct = get_header(resp, "Content-Type")
        print("Content-Type: {ct}\n")
        let cl = get_header(resp, "Content-Length")
        print("Content-Length: {cl}\n")
        // Iterate all headers
        print("All response headers:\n")
        each_header(resp) <| $(key, value) {
            print("  {key}: {value}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Response status message
// ──────────────────────────────────────────────────────────────────────────
//
// status_message returns the human-readable status text
// (e.g., "OK", "Not Found").

def example_status_message(base_url : string) {
    GET("{base_url}/ping") <| $(resp) {
        let msg = status_message(resp)
        print("Status: {int(resp.status_code)} {msg}\n")
    }
    GET("{base_url}/nonexistent") <| $(resp) {
        let msg = status_message(resp)
        print("Status: {int(resp.status_code)} {msg}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Binary response data
// ──────────────────────────────────────────────────────────────────────────
//
// get_body_bytes extracts the response body as array<uint8> — useful for
// downloading images, files, or any binary content.

def example_binary_response(base_url : string) {
    POST("{base_url}/echo", "binary test data") <| $(resp) {
        var bytes <- get_body_bytes(resp)
        print("Received {length(bytes)} bytes\n")
        delete bytes
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 8 — Quick reference
// ──────────────────────────────────────────────────────────────────────────
//
//  Function                    Description
//  ─────────────────────────   ──────────────────────────────────────────
//  GET(url) <| $(resp) {}      Simple GET request
//  GET(url, headers) <| ...    GET with custom headers
//  POST(url, body) <| ...      POST with text body
//  POST(url, body, hdrs) <| .. POST with body and headers
//  PUT(url, body) <| ...       PUT request (full replace)
//  PUT(url, body, hdrs) <| ... PUT with headers
//  PATCH(url, body) <| ...     PATCH request (partial update)
//  PATCH(url, body, hdrs) <| . PATCH with headers
//  DELETE(url) <| ...          DELETE request
//  DELETE(url, hdrs) <| ...    DELETE with headers
//  HEAD(url) <| ...            HEAD request (headers only)
//  HEAD(url, hdrs) <| ...      HEAD with headers
//  get_header(resp, key)       Read a response header value
//  each_header(resp) <| ...    Iterate all response headers
//  status_message(resp)        Human-readable status text
//  get_body_bytes(resp)        Response body as array<uint8>

[export]
def main() {
    with_tutorial_server(18080) <| $(base_url) {
        print("=== GET requests ===\n")
        example_get(base_url)

        print("\n=== POST requests ===\n")
        example_post(base_url)

        print("\n=== PUT and PATCH requests ===\n")
        example_put_patch(base_url)

        print("\n=== DELETE and HEAD requests ===\n")
        example_delete_head(base_url)

        print("\n=== Response headers ===\n")
        example_response_headers(base_url)

        print("\n=== Status message ===\n")
        example_status_message(base_url)

        print("\n=== Binary data ===\n")
        example_binary_response(base_url)
    }
}

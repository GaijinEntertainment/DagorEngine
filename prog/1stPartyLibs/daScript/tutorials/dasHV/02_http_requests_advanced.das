// Tutorial HV-02: Advanced HTTP Requests (Request Builder)
//
// This tutorial covers:
//   - The with_http_request RAII pattern (safe new + delete)
//   - Setting method, URL, body, and headers on a request
//   - Configuring timeouts (request and connect)
//   - Authentication helpers (Basic Auth, Bearer Token)
//   - Query parameters (set_param, get_param)
//   - Content-Type configuration
//   - Redirect control
//   - Using the generic request() function
//
// Prerequisites: Tutorial HV-01 (simple fire-and-forget API)
//
// The with_http_request() pattern creates an HttpRequest, passes it to
// a block as a temporary pointer (?#), and deletes it when the block
// returns.  The temporary pointer prevents the user from deleting or
// storing the request outside the block — safe RAII lifetime.
//
// Run: daslang.exe tutorials/dasHV/02_http_requests_advanced.das

options gen2
options persistent_heap
options gc

require tutorial_server

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — The with_http_request pattern
// ──────────────────────────────────────────────────────────────────────────
//
// Instead of calling GET(url), you can use with_http_request() to create
// a request builder.  It allocates an HttpRequest, passes it as a
// temporary pointer (?#) to your block, and deletes it automatically.
// Inside the block you configure method, headers, body, etc., then send
// with request(req).

def example_basic_request(base_url : string) {
    // with_http_request allocates an HttpRequest, passes it as a temporary
    // pointer (?#) to the block, and deletes it when the block returns.
    // The temporary pointer prevents storing or deleting the request
    // inside the block — safe RAII lifetime.
    // String fields (url, body) use := (clone) since they are std::string.
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/ping"
        set_timeout(req, 10)
        request(req) <| $(resp) {
            print("request(GET /ping) -> {resp.status_code}: {resp.body}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Request with body and headers
// ──────────────────────────────────────────────────────────────────────────
//
// For POST/PUT/PATCH, set the body field and use set_header() to add
// custom headers.

def example_request_with_body(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.POST
        req.url := "{base_url}/echo"
        req.body := "Hello from request builder!"
        set_header(req, "X-Custom", "tutorial-02")
        set_content_type(req, "text/plain")
        request(req) <| $(resp) {
            print("request(POST /echo) -> {resp.status_code}: {resp.body}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Timeouts
// ──────────────────────────────────────────────────────────────────────────
//
// Two timeout settings:
//   - timeout         : total request timeout in seconds
//   - connect_timeout  : just the connection phase
//
// You can set them directly on the request fields, or use the helper
// functions set_timeout() and set_connect_timeout().

def example_timeouts(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/ping"

        // Helper functions take int — no uint16 cast needed
        set_timeout(req, 30)
        set_connect_timeout(req, 5)

        request(req) <| $(resp) {
            print("request with timeout -> {resp.status_code}: {resp.body}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Authentication
// ──────────────────────────────────────────────────────────────────────────
//
// Two authentication helpers:
//   - set_basic_auth(req, username, password)
//       Sets the Authorization header to "Basic <base64(user:pass)>"
//   - set_bearer_token_auth(req, token)
//       Sets the Authorization header to "Bearer <token>"
//
// These are convenience wrappers — you could also call
// set_header(req, "Authorization", "Bearer mytoken") manually.

def example_auth(base_url : string) {
    // Basic Auth
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/ping"
        set_basic_auth(req, "user", "secret")
        request(req) <| $(resp) {
            print("Basic auth request -> {resp.status_code}\n")
        }
    }

    // Bearer Token
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/ping"
        set_bearer_token_auth(req, "eyJhbGciOiJIUzI1NiJ9.example")
        request(req) <| $(resp) {
            print("Bearer token request -> {resp.status_code}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Query parameters
// ──────────────────────────────────────────────────────────────────────────
//
// set_param(req, key, value) appends ?key=value to the URL.
// get_param(req, key) reads a parameter back.
//
// Multiple parameters are joined with &.

def example_params(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/ping"
        set_param(req, "page", "1")
        set_param(req, "limit", "10")

        // Read back the parameters we set
        let page = get_param(req, "page")
        let limit = get_param(req, "limit")
        print("Parameters: page={page}, limit={limit}\n")

        request(req) <| $(resp) {
            print("request with params -> {resp.status_code}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Redirect control
// ──────────────────────────────────────────────────────────────────────────
//
// allow_redirect(req, true/false) controls whether the HTTP client
// automatically follows 3xx redirects.

def example_redirect(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.GET
        req.url := "{base_url}/ping"
        allow_redirect(req, true)  // follow redirects (default behavior)
        request(req) <| $(resp) {
            print("redirect enabled -> {resp.status_code}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Content-Type
// ──────────────────────────────────────────────────────────────────────────
//
// set_content_type(req, "application/json") sets the Content-Type header.
// This is a convenience wrapper for set_header(req, "Content-Type", ...).

def example_content_type(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.POST
        req.url := "{base_url}/echo"
        req.body := "\{\"key\":\"value\"\}"
        set_content_type(req, "application/json")
        request(req) <| $(resp) {
            print("JSON POST -> {resp.status_code}: {resp.body}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 8 — Inspecting request fields
// ──────────────────────────────────────────────────────────────────────────
//
// HttpRequest exposes several read-only fields that you can inspect
// after building the request (or when receiving one on the server side):
//   method, url, scheme, host, port, path, timeout, connect_timeout, body

def example_inspect_request(base_url : string) {
    with_http_request() <| $(var req) {
        req.method = http_method.POST
        req.url := "{base_url}/echo"
        req.body := "inspect me"
        set_timeout(req, 15)

        print("Request fields:\n")
        print("  method:  {req.method}\n")
        print("  url:     {req.url}\n")
        print("  body:    {req.body}\n")
        print("  timeout: {int(req.timeout)}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 9 — Quick reference
// ──────────────────────────────────────────────────────────────────────────
//
//  Function / field                          Description
//  ──────────────────────────────────────────  ──────────────────────────────────
//  with_http_request() <| $(var req) { ... }  RAII request — auto new + delete
//  req.method = http_method.GET               Set the HTTP method
//  req.url := "..."                           Set the target URL (clone!)
//  req.body := "..."                          Set the request body (clone!)
//  set_header(req, key, value)                Add a custom header
//  set_content_type(req, ct)                  Set Content-Type header
//  set_basic_auth(req, user, pass)            Set Basic authentication
//  set_bearer_token_auth(req, token)          Set Bearer token authentication
//  set_timeout(req, sec)                      Set total timeout (seconds)
//  set_connect_timeout(req, sec)              Set connect timeout (seconds)
//  set_param(req, key, value)                 Add a query parameter
//  get_param(req, key)                        Read a query parameter
//  allow_redirect(req, true/false)            Enable/disable auto-redirect
//  request(req) <| $(resp) { ... }            Send the request, get response

[export]
def main() {
    with_tutorial_server(18081) <| $(base_url) {
        print("=== Basic request builder ===\n")
        example_basic_request(base_url)

        print("\n=== Request with body and headers ===\n")
        example_request_with_body(base_url)

        print("\n=== Timeouts ===\n")
        example_timeouts(base_url)

        print("\n=== Authentication ===\n")
        example_auth(base_url)

        print("\n=== Query parameters ===\n")
        example_params(base_url)

        print("\n=== Redirect control ===\n")
        example_redirect(base_url)

        print("\n=== Content-Type ===\n")
        example_content_type(base_url)

        print("\n=== Inspect request fields ===\n")
        example_inspect_request(base_url)
    }
}

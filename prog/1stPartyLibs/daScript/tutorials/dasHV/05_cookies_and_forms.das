// Tutorial HV-05: Cookies and Form / File Upload
//
// This tutorial covers:
//   - Setting cookies on HTTP responses (simple and extended)
//   - Reading cookies from HTTP requests
//   - Iterating all cookies on a message
//   - Sending form data from the client (set_form_data, set_form_file)
//   - Reading form data on the server (get_form_data, each_form_field)
//   - Saving uploaded files on the server (save_form_file)
//   - URL-encoded form data (set_url_encoded, get_url_encoded)
//
// Prerequisites: Tutorials HV-03 and HV-04 (HTTP server basics)
//
// The tutorial starts a local server with cookie and form-handling routes,
// then sends requests from the client side to exercise each feature.
//
// Run: daslang.exe tutorials/dasHV/05_cookies_and_forms.das

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require daslib/jobque_boost
require daslib/json_boost
require strings
require daslib/fio

let SERVER_PORT = 18084

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Cookies
// ──────────────────────────────────────────────────────────────────────────
//
// add_cookie(resp, name, value) sets a simple name=value cookie.
//
// add_cookie(resp, name, value, domain, path, max_age, secure, httponly)
// sets a cookie with extended attributes (domain, path, max-age, etc.).
//
// On the receiving side, get_cookie(req, name) returns the cookie value,
// and each_cookie(req) iterates all cookies on the message.

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Multipart form data (server side)
// ──────────────────────────────────────────────────────────────────────────
//
// When the client sends multipart/form-data, the server can:
//   get_form_data(req, "field")    – read a text field value
//   save_form_file(req, "f", dir)  – save an uploaded file to disk
//   each_form_field(req) <| $(n, content, filename) – iterate all fields

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — URL-encoded form data
// ──────────────────────────────────────────────────────────────────────────
//
// For simple key=value form submissions (Content-Type:
// application/x-www-form-urlencoded), use:
//   set_url_encoded(req, key, value)   – client side
//   get_url_encoded(req, key)          – server side

class CookieFormServer : HvWebServer {
    upload_dir : string

    def override onInit {

        // --- Section 1a: Set cookies ---
        // This route sets two cookies: a simple one and one with extended attributes
        GET("/set-cookies") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            // Simple cookie (resp is HttpResponse& — direct call)
            add_cookie(resp, "session", "abc123")
            // Extended cookie with path, max-age, httponly
            add_cookie(resp, "prefs", "dark-mode", "", "/", 3600, false, true)
            return resp |> TEXT_PLAIN("cookies set")
        }

        // --- Section 1b: Read cookies ---
        // The client sends cookies back; the server reads them
        GET("/read-cookies") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let session = get_cookie(req, "session")
            let prefs = get_cookie(req, "prefs")
            var s = !empty(session) ? string(session) : "not found"
            var p = !empty(prefs) ? string(prefs) : "not found"
            let payload : tuple<session : string; prefs : string> = (s, p)
            return resp |> JSON(write_json(JV(payload)))
        }

        // --- Section 1c: Iterate all cookies ---
        GET("/list-cookies") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            var result = ""
            each_cookie(req) <| $(name, value) {
                if (!empty(result)) {
                    result = "{result}, "
                }
                result = "{result}{name}={value}"
            }
            return resp |> TEXT_PLAIN(result)
        }

        // --- Section 2: Multipart form data ---
        POST("/upload") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let title = get_form_data(req, "title")
            let description = get_form_data(req, "description")

            // List all form fields (including file fields)
            var fields : array<string>
            each_form_field(req) <| $(name, content, filename) {
                if (!empty(filename)) {
                    let sz = length(content)
                    fields |> push_clone("{name}: file={filename} ({sz} bytes)")
                } else {
                    fields |> push_clone("{name}: {content}")
                }
            }

            // Save the uploaded file if present
            var save_status = 0
            save_status = save_form_file(req, "attachment", upload_dir)

            var t = !empty(title) ? string(title) : ""
            var d = !empty(description) ? string(description) : ""
            let payload : tuple<title : string; description : string; save_status : int; field_count : int> = (t, d, save_status, length(fields))
            return resp |> JSON(write_json(JV(payload)))
        }

        // --- Section 3: URL-encoded form data ---
        POST("/login") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let username = get_url_encoded(req, "username")
            let password = get_url_encoded(req, "password")
            var u = !empty(username) ? string(username) : ""
            var pw = !empty(password) ? string(password) : ""
            // Build JSON manually to avoid named-tuple collision with Section 1b
            // (both would be tuple<string;string>, so JV would reuse field names)
            resp.body := "\{\"username\":\"{u}\",\"password\":\"{pw}\"\}"
            set_content_type(resp, "application/json")
            return http_status.OK
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Setup helpers
// ──────────────────────────────────────────────────────────────────────────

def create_upload_dir(dir : string) {
    mkdir_rec(dir)
}

def create_test_upload_file(path : string) {
    fopen(path, "w") <| $(f) {
        if (f != null) {
            fwrite(f, "this is test file content for upload")
        }
    }
}

def cleanup_upload_dir(dir : string) {
    // Remove any files that might have been saved
    remove("{dir}/testfile.txt")
    remove(dir)
}

// ──────────────────────────────────────────────────────────────────────────
// Server lifecycle
// ──────────────────────────────────────────────────────────────────────────

def with_cookie_form_server(port : int; upload_dir : string; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new CookieFormServer()
                    server.upload_dir = upload_dir
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
// Quick reference
// ──────────────────────────────────────────────────────────────────────────
//
//  Function                                              Description
//  ────────────────────────────────────────────────────  ────────────────────────────────
//  add_cookie(msg, name, value)                         Simple cookie
//  add_cookie(msg, name, value, domain, path,           Extended cookie
//             max_age, secure, httponly)
//  get_cookie(msg, name)                                Get cookie value (string or null)
//  each_cookie(msg) <| $(name, value) { ... }           Iterate all cookies
//  set_form_data(req, name, value)                      Set form field (client)
//  set_form_file(req, name, filepath)                   Set file upload field (client)
//  get_form_data(msg, name)                             Get form field value (server)
//  save_form_file(msg, name, dir)                       Save uploaded file (server)
//  each_form_field(msg) <| $(name, content, filename)   Iterate all form fields
//  set_url_encoded(req, key, value)                     Set URL-encoded field (client)
//  get_url_encoded(msg, key)                            Get URL-encoded field (server)

[export]
def main() {
    let upload_dir = "{get_das_root()}/tutorials/dasHV/_upload_test"
    let test_file = "{get_das_root()}/tutorials/dasHV/_testfile.txt"
    create_upload_dir(upload_dir)
    create_test_upload_file(test_file)

    with_cookie_form_server(SERVER_PORT, upload_dir) <| $(base_url) {

        // ── Section 1a: Set cookies ──
        print("=== Section 1a: Set cookies ===\n")
        GET("{base_url}/set-cookies") <| $(resp) {
            print("GET /set-cookies -> {resp.status_code}: {resp.body}\n")
            // The response should have Set-Cookie headers
            each_header(resp) <| $(key, value) {
                if (key == "Set-Cookie" || key == "set-cookie") {
                    print("  {key}: {value}\n")
                }
            }
        }

        // ── Section 1b: Send cookies back to the server ──
        print("\n=== Section 1b: Read cookies ===\n")
        with_http_request() <| $(var req) {
            req.method = http_method.GET
            req.url := "{base_url}/read-cookies"
            // Add cookies to the request
            add_cookie(req, "session", "abc123")
            add_cookie(req, "prefs", "dark-mode")
            request(req) <| $(resp) {
                print("GET /read-cookies -> {resp.body}\n")
            }
        }

        // ── Section 1c: List all cookies ──
        print("\n=== Section 1c: List all cookies ===\n")
        with_http_request() <| $(var req) {
            req.method = http_method.GET
            req.url := "{base_url}/list-cookies"
            add_cookie(req, "token", "xyz789")
            add_cookie(req, "lang", "en")
            add_cookie(req, "theme", "dark")
            request(req) <| $(resp) {
                print("GET /list-cookies -> {resp.body}\n")
            }
        }

        // ── Section 2: Multipart form upload ──
        print("\n=== Section 2: Multipart form upload ===\n")
        with_http_request() <| $(var req) {
            req.method = http_method.POST
            req.url := "{base_url}/upload"
            // Set text form fields
            set_form_data(req, "title", "My Document")
            set_form_data(req, "description", "A test upload")
            // Attach a file
            set_form_file(req, "attachment", test_file)
            request(req) <| $(resp) {
                print("POST /upload -> {resp.status_code}: {resp.body}\n")
            }
        }

        // Verify the file was saved
        let saved_path = "{upload_dir}/_testfile.txt"
        if (stat(saved_path).is_valid) {
            fopen(saved_path, "r") <| $(f) {
                if (f != null) {
                    let content = fread(f)
                    print("  Saved file content: {content}\n")
                }
            }
        } else {
            print("  (file was not saved to {saved_path})\n")
        }

        // ── Section 3: URL-encoded form ──
        print("\n=== Section 3: URL-encoded form ===\n")
        with_http_request() <| $(var req) {
            req.method = http_method.POST
            req.url := "{base_url}/login"
            set_url_encoded(req, "username", "admin")
            set_url_encoded(req, "password", "secret123")
            request(req) <| $(resp) {
                print("POST /login -> {resp.status_code}: {resp.body}\n")
            }
        }
    }

    // Clean up
    remove(test_file)
    cleanup_upload_dir(upload_dir)
}

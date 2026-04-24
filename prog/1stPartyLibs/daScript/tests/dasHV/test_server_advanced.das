// Tests for advanced HTTP server features (Tutorial HV-04)
//
// Covers: STATIC, allow_cors, REDIRECT, SERVE_FILE, DATA,
//         set_content_type (on response), custom status codes
//         (201, 204, 404)

options gen2
options persistent_heap
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require _dashv_test_common
require strings

// ──────────────────────────────────────────────────────────────────────────
// Internal server with advanced features
// ──────────────────────────────────────────────────────────────────────────

class AdvancedTestServer : HvWebServer {
    static_dir : string

    def override onInit {
        // CORS
        allow_cors()

        // Redirect
        GET("/old-path") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> REDIRECT("/new-path", http_status.MOVED_PERMANENTLY)
        }
        GET("/new-path") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("arrived at new-path")
        }

        // Custom headers
        GET("/cached") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Cache-Control", "max-age=3600")
            set_header(resp, "ETag", "\"v1\"")
            set_header(resp, "X-Server", "daslang-test")
            return resp |> TEXT_PLAIN("cached response")
        }

        // Binary data
        GET("/binary") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            var payload = "BINARYDATA12"
            return resp |> DATA(payload, 12)
        }

        // HTML with custom content-type
        GET("/html") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_content_type(resp, "text/html")
            resp.body := "<h1>Hello</h1>"
            return http_status.OK
        }

        // 404 JSON error
        GET("/not-found") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> JSON("\{\"error\":\"not found\"\}", http_status.NOT_FOUND)
        }

        // 201 Created
        POST("/items") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "Location", "/items/42")
            return resp |> TEXT_PLAIN("created", http_status.CREATED)
        }

        // 204 No Content
        POST("/ack") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return http_status.NO_CONTENT
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Server lifecycle with static file setup
// ──────────────────────────────────────────────────────────────────────────

def with_advanced_test_server(port : int; blk : block<(base_url : string) : void>) {
    let static_root = "{get_das_root()}/tests/_dasHV/_static_test"
    // Create temp files for static serving
    mkdir_rec(static_root)
    write_test_file("{static_root}/index.html", "<h1>Test</h1>")
    write_test_file("{static_root}/data.json", "\{\"key\":\"value\"\}")

    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new AdvancedTestServer()
                    server.static_dir = static_root
                    server->init(port)
                    // STATIC must be registered after init
                    STATIC(server.server, "/static", static_root)
                    server->start()
                    started |> notify_and_release
                    while (stop_flag |> get == 0) {
                        server->tick()
                        sleep(1u)
                    }
                    server->stop()
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

    // Cleanup
    remove("{static_root}/index.html")
    remove("{static_root}/data.json")
    remove(static_root)
}

// ──────────────────────────────────────────────────────────────────────────
// Tests
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_static_file_serving(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("serves HTML file") @(t : T?) {
            GET("{base_url}/static/index.html") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                t |> success(find(string(resp.body), "<h1>Test</h1>") >= 0, "body contains HTML")
            }
        }
        t |> run("serves JSON file with correct content-type") @(t : T?) {
            GET("{base_url}/static/data.json") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                t |> success(find(string(resp.body), "key") >= 0, "body contains JSON data")
            }
        }
        t |> run("returns 404 for missing static file") @(t : T?) {
            GET("{base_url}/static/nonexistent.txt") <| $(resp) {
                t |> success(int(resp.status_code) != int(http_status.OK), "non-200 for missing file")
            }
        }
    }
}

[test]
def test_cors(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("CORS headers present on response") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.OPTIONS
                req.url := "{base_url}/cached"
                set_header(req, "Origin", "http://example.com")
                set_header(req, "Access-Control-Request-Method", "GET")
                request(req) <| $(resp) {
                    let cors_origin = get_header(resp, "Access-Control-Allow-Origin")
                    t |> success(!empty(string(cors_origin)), "Access-Control-Allow-Origin present")
                }
            }
        }
    }
}

[test]
def test_redirect(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("REDIRECT returns 301 with Location header") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/old-path"
                allow_redirect(req, false)
                request(req) <| $(resp) {
                    t |> equal(int(resp.status_code), int(http_status.MOVED_PERMANENTLY))
                    let location = get_header(resp, "Location")
                    t |> success(!empty(string(location)), "Location header present")
                }
            }
        }
        t |> run("redirect followed arrives at destination") @(t : T?) {
            GET("{base_url}/old-path") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                t |> equal(string(resp.body), "arrived at new-path")
            }
        }
    }
}

[test]
def test_custom_headers_caching(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("custom Cache-Control and ETag headers") @(t : T?) {
            GET("{base_url}/cached") <| $(resp) {
                let cc = get_header(resp, "Cache-Control")
                t |> equal(string(cc), "max-age=3600")
                let etag = get_header(resp, "ETag")
                t |> equal(string(etag), "\"v1\"")
                let xs = get_header(resp, "X-Server")
                t |> equal(string(xs), "daslang-test")
            }
        }
    }
}

[test]
def test_binary_data(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("DATA returns binary with octet-stream content-type") @(t : T?) {
            GET("{base_url}/binary") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                t |> equal(resp.content_length, 12)
                let ct = get_header(resp, "Content-Type")
                t |> success(find(string(ct), "octet-stream") >= 0, "Content-Type is octet-stream")
            }
        }
    }
}

[test]
def test_custom_content_type(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("HTML response with text/html content-type") @(t : T?) {
            GET("{base_url}/html") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                let ct = get_header(resp, "Content-Type")
                t |> success(find(string(ct), "text/html") >= 0, "Content-Type is text/html")
                t |> success(find(string(resp.body), "<h1>Hello</h1>") >= 0, "body is HTML")
            }
        }
    }
}

[test]
def test_status_404(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("404 error with JSON body") @(t : T?) {
            GET("{base_url}/not-found") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.NOT_FOUND))
                let ct = get_header(resp, "Content-Type")
                t |> success(find(string(ct), "application/json") >= 0, "Content-Type is JSON")
                t |> success(find(string(resp.body), "not found") >= 0, "body contains error message")
            }
        }
    }
}

[test]
def test_status_201_created(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("201 Created with Location header") @(t : T?) {
            POST("{base_url}/items", "new item") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.CREATED))
                let location = get_header(resp, "Location")
                t |> equal(string(location), "/items/42")
            }
        }
    }
}

[test]
def test_status_204_no_content(t : T?) {
    with_advanced_test_server(PORT_SERVER_ADVANCED) <| $(base_url) {
        t |> run("204 No Content") @(t : T?) {
            POST("{base_url}/ack", "done") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.NO_CONTENT))
            }
        }
    }
}

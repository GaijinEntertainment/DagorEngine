// Tests for with_http_request / request builder API (Tutorial HV-02)
//
// Covers: with_http_request, request(), set_header (on request),
//         set_timeout, set_connect_timeout, set_content_type (on request),
//         set_param, get_param, allow_redirect, set_basic_auth,
//         set_bearer_token_auth

options gen2
options persistent_heap
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require _dashv_test_common
require strings

// ──────────────────────────────────────────────────────────────────────────
// Internal server for request builder tests
// ──────────────────────────────────────────────────────────────────────────

class RequestBuilderServer : HvWebServer {
    def override onInit {
        GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("pong")
        }
        POST("/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN(string(req.body))
        }
        // Echo back a specific request header value
        GET("/echo-header") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let val = get_header(req, "X-Custom")
            return resp |> TEXT_PLAIN(string(val))
        }
        // Echo back Content-Type from the request
        POST("/echo-content-type") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let ct = get_header(req, "Content-Type")
            return resp |> TEXT_PLAIN(string(ct))
        }
        // Echo back query param
        GET("/echo-param") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let page = get_param(req, "page")
            let limit = get_param(req, "limit")
            return resp |> TEXT_PLAIN("page={page},limit={limit}")
        }
        // Echo back Authorization header
        GET("/check-auth") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let auth = get_header(req, "Authorization")
            return resp |> TEXT_PLAIN(string(auth))
        }
        // Redirect chain
        GET("/redirect-src") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> REDIRECT("/redirect-dst", http_status.MOVED_PERMANENTLY)
        }
        GET("/redirect-dst") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("arrived")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Tests
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_with_http_request_basic(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("basic GET via request builder") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/ping"
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "pong")
                    t |> equal(int(resp.status_code), int(http_status.OK))
                }
            }
        }
    }
}

[test]
def test_request_with_body(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("POST with body via request builder") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/echo"
                req.body := "request builder body"
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "request builder body")
                }
            }
        }
    }
}

[test]
def test_set_header_on_request(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("set_header adds header to request") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/echo-header"
                set_header(req, "X-Custom", "test-value-123")
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "test-value-123")
                }
            }
        }
    }
}

[test]
def test_set_content_type_on_request(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("set_content_type sets Content-Type header") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/echo-content-type"
                req.body := "data"
                set_content_type(req, "text/plain")
                request(req) <| $(resp) {
                    t |> success(find(string(resp.body), "text/plain") >= 0, "Content-Type contains text/plain")
                }
            }
        }
    }
}

[test]
def test_timeout_settings(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("set_timeout and set_connect_timeout do not break request") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/ping"
                set_timeout(req, 30)
                set_connect_timeout(req, 5)
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "pong")
                }
            }
        }
    }
}

[test]
def test_query_params(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("set_param adds query parameters") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/echo-param"
                set_param(req, "page", "5")
                set_param(req, "limit", "20")
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "page=5,limit=20")
                }
            }
        }
        t |> run("get_param reads back parameter") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/ping"
                set_param(req, "key", "value")
                let val = get_param(req, "key")
                t |> equal(string(val), "value")
            }
        }
    }
}

[test]
def test_basic_auth(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("set_basic_auth sets Authorization header") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/check-auth"
                set_basic_auth(req, "user", "pass")
                request(req) <| $(resp) {
                    t |> success(find(string(resp.body), "Basic") >= 0, "Authorization starts with Basic")
                }
            }
        }
    }
}

[test]
def test_bearer_token_auth(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("set_bearer_token_auth sets Authorization header") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/check-auth"
                set_bearer_token_auth(req, "my-secret-token")
                request(req) <| $(resp) {
                    t |> success(find(string(resp.body), "Bearer my-secret-token") >= 0, "Authorization is Bearer token")
                }
            }
        }
    }
}

[test]
def test_redirect_control(t : T?) {
    with_test_server(type<RequestBuilderServer>, PORT_REQUEST_BUILDER) <| $(base_url) {
        t |> run("allow_redirect(false) does not follow redirect") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/redirect-src"
                allow_redirect(req, false)
                request(req) <| $(resp) {
                    t |> equal(int(resp.status_code), int(http_status.MOVED_PERMANENTLY))
                    let location = get_header(resp, "Location")
                    t |> success(!empty(string(location)), "Location header present")
                }
            }
        }
        t |> run("allow_redirect(true) follows redirect") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.GET
                req.url := "{base_url}/redirect-src"
                allow_redirect(req, true)
                request(req) <| $(resp) {
                    t |> equal(int(resp.status_code), int(http_status.OK))
                    t |> equal(string(resp.body), "arrived")
                }
            }
        }
    }
}

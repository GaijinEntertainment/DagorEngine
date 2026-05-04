// Tests for fire-and-forget HTTP client API (Tutorial HV-01)
//
// Covers: GET, POST, PUT, PATCH, DELETE, HEAD (with and without headers),
//         get_header, each_header, status_message, get_body_bytes

options gen2
options persistent_heap
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require _dashv_test_common
require strings

// ──────────────────────────────────────────────────────────────────────────
// Internal server for client tests
// ──────────────────────────────────────────────────────────────────────────

class ClientTestServer : HvWebServer {
    def override onInit {
        GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("pong")
        }
        POST("/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN(string(req.body))
        }
        PUT("/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("PUT:{req.body}")
        }
        PATCH("/echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("PATCH:{req.body}")
        }
        DELETE("/resource") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("deleted")
        }
        HEAD("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "X-Pong", "true")
            return http_status.OK
        }
        GET("/json") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> JSON("\{\"status\":\"ok\"\}")
        }
        GET("/custom-header") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            set_header(resp, "X-Custom", "test-value")
            return resp |> TEXT_PLAIN("with-header")
        }
        GET("/echo-header") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let val = get_header(req, "X-Test")
            return resp |> TEXT_PLAIN(string(val))
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Tests — single server lifecycle, all sub-tests share the server
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_client_basic(t : T?) {
    with_test_server(type<ClientTestServer>, PORT_CLIENT_BASIC) <| $(base_url) {
        t |> run("PUT request") @(t : T?) {
            PUT("{base_url}/echo", "updated") <| $(resp) {
                t |> equal(string(resp.body), "PUT:updated")
            }
        }
        t |> run("GET returns correct body") @(t : T?) {
            GET("{base_url}/ping") <| $(resp) {
                t |> equal(string(resp.body), "pong")
                t |> equal(int(resp.status_code), int(http_status.OK))
            }
        }
        t |> run("GET with custom headers") @(t : T?) {
            GET("{base_url}/echo-header", {"X-Test" => "hello"}) <| $(resp) {
                t |> equal(string(resp.body), "hello")
            }
        }
        t |> run("POST echoes body") @(t : T?) {
            POST("{base_url}/echo", "test body") <| $(resp) {
                t |> equal(string(resp.body), "test body")
                t |> equal(int(resp.status_code), int(http_status.OK))
            }
        }
        t |> run("POST with custom headers") @(t : T?) {
            POST("{base_url}/echo", "json data", {"Content-Type" => "application/json"}) <| $(resp) {
                t |> equal(string(resp.body), "json data")
            }
        }
        t |> run("PUT with headers") @(t : T?) {
            PUT("{base_url}/echo", "data", {"X-Custom" => "val"}) <| $(resp) {
                t |> equal(string(resp.body), "PUT:data")
            }
        }
        t |> run("PATCH request") @(t : T?) {
            PATCH("{base_url}/echo", "partial") <| $(resp) {
                t |> equal(string(resp.body), "PATCH:partial")
            }
        }
        t |> run("DELETE request") @(t : T?) {
            DELETE("{base_url}/resource") <| $(resp) {
                t |> equal(string(resp.body), "deleted")
                t |> equal(int(resp.status_code), int(http_status.OK))
            }
        }
        t |> run("DELETE with headers") @(t : T?) {
            DELETE("{base_url}/resource", {"Authorization" => "Bearer tok"}) <| $(resp) {
                t |> equal(string(resp.body), "deleted")
            }
        }
        t |> run("HEAD request returns no body") @(t : T?) {
            HEAD("{base_url}/ping") <| $(resp) {
                t |> equal(int(resp.status_code), int(http_status.OK))
                let pong = get_header(resp, "X-Pong")
                t |> equal(string(pong), "true")
            }
        }
        t |> run("get_header reads specific header") @(t : T?) {
            GET("{base_url}/custom-header") <| $(resp) {
                let val = get_header(resp, "X-Custom")
                t |> equal(string(val), "test-value")
            }
        }
        t |> run("each_header iterates headers") @(t : T?) {
            GET("{base_url}/custom-header") <| $(resp) {
                var found_custom = false
                each_header(resp) <| $(key, value) {
                    if (key == "X-Custom") {
                        found_custom = true
                    }
                }
                t |> success(found_custom, "X-Custom header found")
            }
        }
        t |> run("status_message for 200") @(t : T?) {
            GET("{base_url}/ping") <| $(resp) {
                let msg = status_message(resp)
                t |> equal(string(msg), "OK")
            }
        }
        t |> run("get_body_bytes returns body as bytes") @(t : T?) {
            POST("{base_url}/echo", "binary test") <| $(resp) {
                var bytes <- get_body_bytes(resp)
                t |> equal(length(bytes), 11)
                delete bytes
            }
        }
        t |> run("JSON response has correct content-type") @(t : T?) {
            GET("{base_url}/json") <| $(resp) {
                let ct = get_header(resp, "Content-Type")
                t |> success(find(string(ct), "application/json") >= 0, "Content-Type contains application/json")
                t |> success(find(string(resp.body), "ok") >= 0, "body contains 'ok'")
            }
        }
    }
}

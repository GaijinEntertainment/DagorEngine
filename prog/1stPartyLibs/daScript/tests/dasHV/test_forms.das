// Tests for form data and file upload (Tutorial HV-05, Sections 2-3)
//
// Covers: set_form_data, set_form_file, get_form_data, save_form_file,
//         each_form_field, set_url_encoded, get_url_encoded

options gen2
options persistent_heap
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require _dashv_test_common
require strings

// ──────────────────────────────────────────────────────────────────────────
// Internal server for form tests
// ──────────────────────────────────────────────────────────────────────────

class FormTestServer : HvWebServer {
    upload_dir : string

    def override onInit {
        // Echo form data fields
        POST("/form-echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let name = get_form_data(req, "name")
            let value = get_form_data(req, "value")
            var n = !empty(name) ? string(name) : "missing"
            var v = !empty(value) ? string(value) : "missing"
            return resp |> TEXT_PLAIN("name={n},value={v}")
        }

        // Iterate all form fields
        POST("/form-fields") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            var count = 0
            var result = ""
            each_form_field(req) <| $(name, content, filename) {
                count++
                if (!empty(result)) {
                    result = "{result};"
                }
                if (!empty(filename)) {
                    result = "{result}{name}:file={filename}"
                } else {
                    result = "{result}{name}={content}"
                }
            }
            return resp |> TEXT_PLAIN("{count}:{result}")
        }

        // Save uploaded file
        POST("/upload") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            var save_status = save_form_file(req, "file", upload_dir)
            return resp |> TEXT_PLAIN("status={save_status}")
        }

        // Missing form field
        POST("/form-missing") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let missing = get_form_data(req, "nonexistent")
            let is_empty = empty(missing)
            return resp |> TEXT_PLAIN(is_empty ? "empty" : "found")
        }

        // URL-encoded form data
        POST("/url-encoded") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let username = get_url_encoded(req, "username")
            let password = get_url_encoded(req, "password")
            var u = !empty(username) ? string(username) : "missing"
            var p = !empty(password) ? string(password) : "missing"
            return resp |> TEXT_PLAIN("user={u},pass={p}")
        }

        // Missing URL-encoded field
        POST("/url-encoded-missing") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let missing = get_url_encoded(req, "nonexistent")
            let is_empty = empty(missing)
            return resp |> TEXT_PLAIN(is_empty ? "empty" : "found")
        }

        // PUT form echo (mirrors POST /form-echo)
        PUT("/form-echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let name = get_form_data(req, "name")
            let value = get_form_data(req, "value")
            var n = !empty(name) ? string(name) : "missing"
            var v = !empty(value) ? string(value) : "missing"
            return resp |> TEXT_PLAIN("PUT:name={n},value={v}")
        }

        // PATCH form echo (mirrors POST /form-echo)
        PATCH("/form-echo") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            let name = get_form_data(req, "name")
            let value = get_form_data(req, "value")
            var n = !empty(name) ? string(name) : "missing"
            var v = !empty(value) ? string(value) : "missing"
            return resp |> TEXT_PLAIN("PATCH:name={n},value={v}")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Helper: start form test server with upload dir setup
// ──────────────────────────────────────────────────────────────────────────

def with_form_test_server(port : int; blk : block<(base_url : string; upload_dir : string) : void>) {
    let up_dir = "{get_das_root()}/tests/_dasHV/_upload_test"
    mkdir_rec(up_dir)

    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new FormTestServer()
                    server.upload_dir = up_dir
                    server->init(port)
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
                invoke(blk, base_url, up_dir)
                stop_flag |> set(1)
                finished |> join
            }
        }
    }

    // Cleanup upload dir
    remove(up_dir)
}

// ──────────────────────────────────────────────────────────────────────────
// Tests — Multipart form data
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_set_form_data(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("set_form_data sends text fields") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/form-echo"
                set_form_data(req, "name", "Alice")
                set_form_data(req, "value", "42")
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "name=Alice,value=42")
                }
            }
        }
    }
}

[test]
def test_each_form_field(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("each_form_field iterates all fields") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/form-fields"
                set_form_data(req, "field1", "value1")
                set_form_data(req, "field2", "value2")
                request(req) <| $(resp) {
                    let body = string(resp.body)
                    // Format is "count:field1=value1;field2=value2"
                    t |> success(starts_with(body, "2:"), "2 fields iterated")
                    t |> success(find(body, "field1=value1") >= 0, "field1 found")
                    t |> success(find(body, "field2=value2") >= 0, "field2 found")
                }
            }
        }
    }
}

[test]
def test_form_file_upload(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        // Create a test file to upload
        let test_file = "{get_das_root()}/tests/_dasHV/_test_upload_src.txt"
        write_test_file(test_file, "upload content 12345")

        t |> run("set_form_file and each_form_field shows file") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/form-fields"
                set_form_data(req, "title", "My File")
                set_form_file(req, "attachment", test_file)
                request(req) <| $(resp) {
                    let body = string(resp.body)
                    t |> success(find(body, "title=My File") >= 0, "text field present")
                    t |> success(find(body, "attachment:file=") >= 0, "file field present")
                }
            }
        }

        t |> run("save_form_file saves uploaded file to disk") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/upload"
                set_form_file(req, "file", test_file)
                request(req) <| $(resp) {
                    t |> equal(int(resp.status_code), int(http_status.OK))
                    // save_form_file returns 200 on success
                    t |> success(find(string(resp.body), "status=200") >= 0, "save succeeded")
                }
            }
            // Verify the file was saved
            let saved_path = "{upload_dir}/_test_upload_src.txt"
            let saved_content = read_test_file(saved_path)
            t |> equal(saved_content, "upload content 12345")
            // Cleanup saved file
            remove(saved_path)
        }

        // Cleanup source file
        remove(test_file)
    }
}

[test]
def test_get_form_data_missing(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("get_form_data returns empty for missing field") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/form-missing"
                set_form_data(req, "existing", "value")
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "empty")
                }
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Tests — URL-encoded form data
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_url_encoded(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("set_url_encoded sends key-value pairs") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/url-encoded"
                set_url_encoded(req, "username", "admin")
                set_url_encoded(req, "password", "secret")
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "user=admin,pass=secret")
                }
            }
        }
    }
}

[test]
def test_url_encoded_missing(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("get_url_encoded returns empty for missing field") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/url-encoded-missing"
                set_url_encoded(req, "existing", "value")
                request(req) <| $(resp) {
                    t |> equal(string(resp.body), "empty")
                }
            }
        }
    }
}

[test]
def test_url_encoded_special_chars(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("URL-encoded handles special characters") @(t : T?) {
            with_http_request() <| $(var req) {
                req.method = http_method.POST
                req.url := "{base_url}/url-encoded"
                set_url_encoded(req, "username", "user@example.com")
                set_url_encoded(req, "password", "p@ss w0rd!")
                request(req) <| $(resp) {
                    let body = string(resp.body)
                    t |> success(find(body, "user=user@example.com") >= 0, "username with @ preserved")
                    t |> success(find(body, "pass=p@ss w0rd!") >= 0, "password with special chars preserved")
                }
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Tests — PUT/PATCH form-data overloads
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_put_form_data(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("PUT with form data via convenience overload") @(t : T?) {
            var headers <- { "X-Test" => "put-form" }
            var form_data <- { "name" => "Alice", "value" => "42" }
            PUT("{base_url}/form-echo", "", headers, form_data) <| $(resp) {
                t |> equal(string(resp.body), "PUT:name=Alice,value=42")
            }
            delete headers
            delete form_data
        }
    }
}

[test]
def test_patch_form_data(t : T?) {
    with_form_test_server(PORT_FORMS) <| $(base_url, upload_dir) {
        t |> run("PATCH with form data via convenience overload") @(t : T?) {
            var headers <- { "X-Test" => "patch-form" }
            var form_data <- { "name" => "Bob", "value" => "99" }
            PATCH("{base_url}/form-echo", "", headers, form_data) <| $(resp) {
                t |> equal(string(resp.body), "PATCH:name=Bob,value=99")
            }
            delete headers
            delete form_data
        }
    }
}

options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/jsonrpc
require daslib/json
require daslib/json_boost
require strings

// ===== Envelope builders =====

[test]
def test_response_envelope(t : T?) {
    t |> run("scalar result, integer id") @(t : T?) {
        let r = response("1", "true")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":true}")
    }
    t |> run("object result, integer id") @(t : T?) {
        let r = response("42", "\{\"fps\":60}")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":42,\"result\":\{\"fps\":60}}")
    }
    t |> run("string id is embedded verbatim (caller pre-quotes)") @(t : T?) {
        let r = response("\"abc\"", "null")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":\"abc\",\"result\":null}")
    }
    t |> run("null id") @(t : T?) {
        let r = response("null", "42")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":null,\"result\":42}")
    }
}

[test]
def test_error_envelope(t : T?) {
    t |> run("method-not-found shape") @(t : T?) {
        let r = error("\"abc\"", METHOD_NOT_FOUND, "method not found")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":\"abc\",\"error\":\{\"code\":-32601,\"message\":\"method not found\"}}")
    }
    t |> run("parse-error shape with null id") @(t : T?) {
        let r = error("null", PARSE_ERROR, "bad json")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":\{\"code\":-32700,\"message\":\"bad json\"}}")
    }
    t |> run("messages with quotes are JSON-escaped") @(t : T?) {
        let r = error("1", INVALID_REQUEST, "id must be \"string\"")
        // write_json escapes the quotes
        t |> success(find(r, "\\\"string\\\"") >= 0, "quotes escaped in message")
        t |> success(find(r, "-32600") >= 0, "code present")
    }
}

[test]
def test_error_with_data_envelope(t : T?) {
    t |> run("error with object data") @(t : T?) {
        let r = error_with_data("1", INTERNAL_ERROR, "internal", "\{\"trace\":\"foo\"}")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":1,\"error\":\{\"code\":-32603,\"message\":\"internal\",\"data\":\{\"trace\":\"foo\"}}}")
    }
    t |> run("error with scalar data") @(t : T?) {
        let r = error_with_data("null", INVALID_PARAMS, "bad", "42")
        t |> equal(r, "\{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":\{\"code\":-32602,\"message\":\"bad\",\"data\":42}}")
    }
}

[test]
def test_serialize_id(t : T?) {
    t |> run("null id serializes to 'null'") @(t : T?) {
        t |> equal(serialize_id(null), "null")
    }
    t |> run("integer id round-trips") @(t : T?) {
        t |> equal(serialize_id(JV(7)), "7")
    }
    t |> run("string id keeps quoting") @(t : T?) {
        t |> equal(serialize_id(JV("abc")), "\"abc\"")
    }
}

[test]
def test_error_constants(t : T?) {
    t |> equal(PARSE_ERROR,      -32700)
    t |> equal(INVALID_REQUEST,  -32600)
    t |> equal(METHOD_NOT_FOUND, -32601)
    t |> equal(INVALID_PARAMS,   -32602)
    t |> equal(INTERNAL_ERROR,   -32603)
}

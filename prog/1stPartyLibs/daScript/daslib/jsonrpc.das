options gen2
options indenting = 4
options strict_smart_pointers = true

module jsonrpc shared public

//! JSON-RPC 2.0 envelope + parser, transport-agnostic.
//!
//! Implements the JSON-RPC 2.0 spec (https://www.jsonrpc.org/specification):
//! single-request and §6 batch parsing on the server side, request/notification
//! builders on the client side, response envelope/error envelope helpers, and
//! standard error code constants. No I/O and no dispatch — pair with a
//! transport (stdio loop, HTTP server, websocket) and a method dispatch
//! callback at the call site.
//!
//! The ``compact_json_whitespace`` helper is a transport-framing utility
//! (newline-delimited wires need single-line responses; ``write_json``
//! pretty-prints). Not strictly JSON-RPC, but ships here because every
//! newline-framed transport hits the same problem.
//!
//! Permissive by default: the ``jsonrpc`` request member may be absent or
//! carry a value other than ``"2.0"`` — real-world clients are lazy about it.
//! Pass ``strict=true`` to ``parse_request`` / ``parse_batch`` /
//! ``dispatch_line`` for full §4 compliance (missing or non-"2.0" yields
//! INVALID_REQUEST).

require daslib/json
require daslib/json_boost
require strings


// ===== Error code constants (JSON-RPC 2.0 §5.1) =====

let public PARSE_ERROR      = -32700  //!< Invalid JSON received.
let public INVALID_REQUEST  = -32600  //!< Not a valid request object.
let public METHOD_NOT_FOUND = -32601  //!< Method does not exist or is not available.
let public INVALID_PARAMS   = -32602  //!< Invalid method params.
let public INTERNAL_ERROR   = -32603  //!< Internal JSON-RPC error.


// ===== Parsed request shape (server side) =====

struct public ParsedRequest {
    id_str          : string       //!< Serialized id ("null" / "7" / "\"abc\"") — embed verbatim into responses.
    method          : string       //!< Method name. Empty when the request was malformed.
    params_json     : string       //!< Raw serialized ``params`` value (e.g. ``"null"``, ``"[1,2]"``, ``"{\"x\":1}"``).
    params          : JsonValue?   //!< Pre-parsed ``params`` JsonValue; null when absent. Saves callers a second ``read_json``.
    is_notification : bool         //!< True when the request omits ``id`` (spec §4.1 — MUST NOT send a response).
    error_envelope  : string       //!< Pre-built error response for malformed input. Empty when the request was well-formed.
}

struct public ParsedBatch {
    is_batch        : bool                   //!< True when the wire body was a JSON array.
    requests        : array<ParsedRequest>   //!< One entry per array element (or one for a single request).
    framing_error   : string                 //!< Top-level error envelope to send as-is (parse failure, empty array). Mutually exclusive with ``requests``.
}


// ===== Parsed response shape (client side) =====

struct public ParsedResponse {
    id_str       : string         //!< Echoed id from the response.
    is_success   : bool           //!< True when the response carries ``result``; false when it carries ``error``.
    result_json  : string         //!< Raw ``result`` value; valid when ``is_success``.
    error_code   : int            //!< Error code; 0 when ``is_success``.
    error_msg    : string         //!< Error message; empty when ``is_success``.
    error_data   : string         //!< Optional ``error.data`` field; empty when absent.
    parse_error  : string         //!< Non-empty when the response wire itself was malformed.
}

struct public ParsedResponseBatch {
    is_batch    : bool                        //!< True when the wire body was a JSON array.
    responses   : array<ParsedResponse>       //!< One per array element (or one for a single response).
    parse_error : string                      //!< Top-level parse error; mutually exclusive with ``responses``.
}


// ===== Envelope builders (responses + errors) =====

def public response(id_str : string; result_json : string) : string {
    //! Wrap ``result_json`` in a JSON-RPC 2.0 success response envelope.
    //! ``id_str`` and ``result_json`` are embedded verbatim — both must be valid JSON.
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id_str},\"result\":{result_json}}"
}

def public error(id_str : string; code : int; message : string) : string {
    //! Wrap ``code`` + ``message`` in a JSON-RPC 2.0 error response envelope.
    let msg_json = write_json(JV(message))
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id_str},\"error\":\{\"code\":{code},\"message\":{msg_json}}}"
}

def public error_with_data(id_str : string; code : int; message, data_json : string) : string {
    //! Like ``error``, but also includes the optional ``data`` field (spec §5.1).
    //! ``data_json`` is embedded verbatim — must be valid JSON.
    let msg_json = write_json(JV(message))
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id_str},\"error\":\{\"code\":{code},\"message\":{msg_json},\"data\":{data_json}}}"
}

def public serialize_id(id_val : JsonValue?) : string {
    //! Serialize a JSON-RPC id (string/number/null) to its wire form.
    //! Returns ``"null"`` when ``id_val`` is null.
    return id_val == null ? "null" : write_json(id_val)
}


// ===== Outgoing request builders (client side) =====

def public make_request(method : string; params_json : string; id : int) : string {
    //! Build a JSON-RPC 2.0 request with an integer id.
    //! ``params_json`` is the pre-serialized params value (``"null"``, ``"[1,2]"``, ``"{\"x\":1}"``).
    let method_str = write_json(JV(method))
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id},\"method\":{method_str},\"params\":{params_json}}"
}

def public make_request(method : string; params_json : string; id : string) : string {
    //! Build a JSON-RPC 2.0 request with a string id.
    let method_str = write_json(JV(method))
    let id_str = write_json(JV(id))
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id_str},\"method\":{method_str},\"params\":{params_json}}"
}

def public make_notification(method : string; params_json : string) : string {
    //! Build a JSON-RPC 2.0 notification (no id — server MUST NOT respond).
    let method_str = write_json(JV(method))
    return "\{\"jsonrpc\":\"2.0\",\"method\":{method_str},\"params\":{params_json}}"
}

def public make_batch(messages : array<string>) : string {
    //! Wrap pre-built request/notification strings as a JSON-RPC 2.0 §6 batch array.
    return build_string() $(var w) {
        write_char(w, '[')
        var first = true
        for (m in messages) {
            if (!first) write_char(w, ',')
            first = false
            write(w, m)
        }
        write_char(w, ']')
    }
}


// ===== Server-side parsers =====

def private parse_one_request(body_value : JsonValue?; strict : bool) : ParsedRequest {
    var result = ParsedRequest(id_str = "null")
    if (body_value == null || !(body_value.value is _object)) {
        result.error_envelope = error("null", INVALID_REQUEST, "invalid request: not a JSON object")
        return result
    }
    let body & = unsafe(body_value.value as _object)
    let id_v = body?["id"] ?? null
    let method_v = body?["method"] ?? null
    // params_v needs the reinterpret: it's assigned to `result.params : JsonValue?`, and `?[]` on a const view returns `JsonValue? const` — assignment to a non-const field would fail. id_v / method_v above are read-only and don't need it.
    var params_v = unsafe(reinterpret<JsonValue?>(body?["params"] ?? null))
    // §4: id MUST be string, number, or null. Reject objects/arrays/booleans — they'd also break framing since write_json would pretty-print across lines.
    if (id_v != null && !(id_v.value is _string) && !(id_v.value is _number) && !(id_v.value is _longint) && !(id_v.value is _null)) {
        result.error_envelope = error("null", INVALID_REQUEST, "invalid request: id must be string, number, or null")
        return result
    }
    result.id_str = serialize_id(id_v)
    if (method_v == null || !(method_v.value is _string)) {
        // Don't flag as notification before method validation — `{}` or `{"method":42}` without id would otherwise swallow the envelope per spec §5.1.
        result.error_envelope = error(result.id_str, INVALID_REQUEST, "invalid request: missing or non-string method")
        return result
    }
    // Strict-mode jsonrpc field enforcement
    if (strict) {
        let jsonrpc_v = body?["jsonrpc"] ?? null
        if (jsonrpc_v == null || !(jsonrpc_v.value is _string) || (jsonrpc_v.value as _string) != "2.0") {
            result.error_envelope = error(result.id_str, INVALID_REQUEST, "invalid request: jsonrpc field must be \"2.0\"")
            return result
        }
    }
    // §4.2: params, if present, MUST be Array or Object. Reject scalars.
    if (params_v != null && !(params_v.value is _object) && !(params_v.value is _array) && !(params_v.value is _null)) {
        result.error_envelope = error(result.id_str, INVALID_PARAMS, "invalid params: must be array or object")
        return result
    }
    result.is_notification = (id_v == null)
    result.method = method_v.value as _string
    result.params = params_v
    result.params_json = params_v == null ? "null" : write_json(params_v)
    return result
}

def public parse_request(line : string; strict : bool = false) : ParsedRequest {
    //! Parse a single JSON-RPC 2.0 request line. Pure function — testable in isolation.
    var result = ParsedRequest(id_str = "null")
    var parse_error_msg = ""
    var parsed = read_json(line, parse_error_msg)
    if (parsed == null) {
        result.error_envelope = error("null", PARSE_ERROR, "parse error: {parse_error_msg}")
        return result
    }
    return parse_one_request(parsed, strict)
}

def public parse_batch(line : string; strict : bool = false) : ParsedBatch {
    //! Parse a JSON-RPC 2.0 wire body that may be a single request or a §6 batch array.
    var result : ParsedBatch
    var parse_error_msg = ""
    var parsed = read_json(line, parse_error_msg)
    if (parsed == null) {
        result.framing_error = error("null", PARSE_ERROR, "parse error: {parse_error_msg}")
        return <- result
    }
    if (parsed.value is _array) {
        result.is_batch = true
        let arr & = unsafe(parsed.value as _array)
        if (empty(arr)) {
            result.framing_error = error("null", INVALID_REQUEST, "invalid request: empty batch")
            return <- result
        }
        result.requests |> reserve(length(arr))
        for (entry in arr) {
            let ev = unsafe(reinterpret<JsonValue?>(entry))
            result.requests |> push(parse_one_request(ev, strict))
        }
        return <- result
    }
    // Single request — wrap in a one-element ParsedBatch
    result.is_batch = false
    result.requests |> push(parse_one_request(parsed, strict))
    return <- result
}


// ===== Client-side response parsers =====

def private parse_one_response(body_value : JsonValue?) : ParsedResponse {
    var result = ParsedResponse(id_str = "null", is_success = false)
    if (body_value == null || !(body_value.value is _object)) {
        result.parse_error = "response is not a JSON object"
        return result
    }
    let body & = unsafe(body_value.value as _object)
    let id_v = body?["id"] ?? null
    let result_v = body?["result"] ?? null
    let error_v = body?["error"] ?? null
    result.id_str = serialize_id(id_v)
    if (result_v != null) {
        result.is_success = true
        result.result_json = write_json(result_v)
        return result
    }
    if (error_v == null || !(error_v.value is _object)) {
        result.parse_error = "response missing both `result` and a valid `error`"
        return result
    }
    let err & = unsafe(error_v.value as _object)
    let code_v = err?["code"] ?? null
    let msg_v = err?["message"] ?? null
    let data_v = err?["data"] ?? null
    if (code_v != null && code_v.value is _longint) {
        result.error_code = int(code_v.value as _longint)
    } elif (code_v != null && code_v.value is _number) {
        result.error_code = int(code_v.value as _number)
    }
    if (msg_v != null && msg_v.value is _string) {
        result.error_msg = msg_v.value as _string
    }
    if (data_v != null) {
        result.error_data = write_json(data_v)
    }
    return result
}

def public parse_response(line : string) : ParsedResponse {
    //! Parse a single JSON-RPC 2.0 response line.
    var result = ParsedResponse(id_str = "null", is_success = false)
    var parse_error_msg = ""
    var parsed = read_json(line, parse_error_msg)
    if (parsed == null) {
        result.parse_error = "parse error: {parse_error_msg}"
        return result
    }
    return parse_one_response(parsed)
}

def public parse_response_batch(line : string) : ParsedResponseBatch {
    //! Parse a JSON-RPC 2.0 response wire body that may be a single response or a §6 batch array.
    var result : ParsedResponseBatch
    var parse_error_msg = ""
    var parsed = read_json(line, parse_error_msg)
    if (parsed == null) {
        result.parse_error = "parse error: {parse_error_msg}"
        return <- result
    }
    if (parsed.value is _array) {
        result.is_batch = true
        let arr & = unsafe(parsed.value as _array)
        result.responses |> reserve(length(arr))
        for (entry in arr) {
            let ev = unsafe(reinterpret<JsonValue?>(entry))
            result.responses |> push(parse_one_response(ev))
        }
        return <- result
    }
    result.is_batch = false
    result.responses |> push(parse_one_response(parsed))
    return <- result
}


// ===== Framing helper =====

def public compact_json_whitespace(s : string) : string {
    //! Strip JSON pretty-printer whitespace (spaces, tabs, newlines, carriage returns) that appears outside string literals. Whitespace inside ``"..."`` is preserved verbatim, including backslash-escaped quotes. Used to flatten ``write_json`` output for one-message-per-line wire framing.
    return build_string() $(var w) {
        peek_data(s) $(bytes) {
            var in_string = false
            var escape = false
            for (raw_b in bytes) {
                let b = int(raw_b)
                if (in_string) {
                    write_char(w, b)
                    if (escape) {
                        escape = false
                    } elif (b == '\\') {
                        escape = true
                    } elif (b == '"') {
                        in_string = false
                    }
                } else {
                    if (b == '"') {
                        in_string = true
                        write_char(w, b)
                    } elif (b != ' ' && b != '\n' && b != '\t' && b != '\r') {
                        write_char(w, b)
                    }
                }
            }
        }
    }
}


// ===== High-level dispatch convenience =====

def private dispatch_one(req : ParsedRequest; dispatcher : block<(method : string; params_json : string) : string>) : string {
    if (!empty(req.error_envelope)) return req.is_notification ? "" : req.error_envelope
    let result_json = invoke(dispatcher, req.method, req.params_json)
    return req.is_notification ? "" : response(req.id_str, result_json)
}

def public dispatch_line(line : string; strict : bool; dispatcher : block<(method : string; params_json : string) : string>) : string {
    //! Dispatch a JSON-RPC 2.0 wire line (single or §6 batch) through ``dispatcher``, returning the response string ready for the wire (or ``""`` for all-notifications).
    let pb = parse_batch(line, strict)
    if (!empty(pb.framing_error)) return pb.framing_error
    if (!pb.is_batch) return dispatch_one(pb.requests[0], dispatcher)
    var parts : array<string>
    for (req in pb.requests) {
        let r = dispatch_one(req, dispatcher)
        if (!empty(r)) parts |> push(r)
    }
    if (empty(parts)) return ""
    return build_string() $(var w) {
        write_char(w, '[')
        var first = true
        for (p in parts) {
            if (!first) write_char(w, ',')
            first = false
            write(w, p)
        }
        write_char(w, ']')
    }
}

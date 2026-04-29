options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/json
require daslib/json_boost
require live/live_commands
require strings
require live_host

// --- Test commands ---

var test_call_count : int = 0

[live_command]
def test_increment(input : JsonValue?) : JsonValue? {
    test_call_count++
    return JV(test_call_count)
}

[live_command]
def test_echo(input : JsonValue?) : JsonValue? {
    if (input != null) {
        return JV(write_json(input))
    }
    return JV("no input")
}

[live_command]
def test_add(input : JsonValue?) : JsonValue? {
    var a = 0.0lf
    var b = 0.0lf
    if (input != null && input.value is _object) {
        let tab & = unsafe(input.value as _object)
        var av = tab?["a"] ?? null
        if (av != null) {
            if (av.value is _number) {
                a = av.value as _number
            } elif (av.value is _longint) {
                a = double(av.value as _longint)
            }
        }
        var bv = tab?["b"] ?? null
        if (bv != null) {
            if (bv.value is _number) {
                b = bv.value as _number
            } elif (bv.value is _longint) {
                b = double(bv.value as _longint)
            }
        }
    }
    return JV(a + b)
}

[live_command]
def test_null_return(input : JsonValue?) : JsonValue? {
    return null
}

// --- Tests ---

[test]
def test_registration(t : T?) {
    t |> run("commands are registered") @(t : T?) {
        t |> success(key_exists(live_command_registry, "test_increment"), "test_increment registered")
        t |> success(key_exists(live_command_registry, "test_echo"), "test_echo registered")
        t |> success(key_exists(live_command_registry, "test_add"), "test_add registered")
        t |> success(key_exists(live_command_registry, "test_null_return"), "test_null_return registered")
    }
    t |> run("unknown command not registered") @(t : T?) {
        t |> success(!key_exists(live_command_registry, "nonexistent"), "nonexistent should not be in registry")
    }
}

[test]
def test_dispatch_basic(t : T?) {
    test_call_count = 0
    t |> run("dispatch increment") @(t : T?) {
        let r = live_dispatch_command("\{\"name\": \"test_increment\"}")
        t |> equal(r, "1")
    }
    t |> run("dispatch increment again") @(t : T?) {
        let r = live_dispatch_command("\{\"name\": \"test_increment\"}")
        t |> equal(r, "2")
    }
    t |> run("call count matches") @(t : T?) {
        t |> equal(test_call_count, 2)
    }
}

[test]
def test_dispatch_with_args(t : T?) {
    t |> run("dispatch with args") @(t : T?) {
        let r = live_dispatch_command("\{\"name\": \"test_add\", \"args\": \{\"a\": 3, \"b\": 4}}")
        var parse_error = ""
        var js = read_json(r, parse_error)
        t |> success(js != null, "result should be valid JSON")
        if (js != null) {
            if (js.value is _number) {
                let diff = js.value as _number - 7.0lf
                t |> success(diff > -0.001lf && diff < 0.001lf, "3 + 4 should be 7")
            } elif (js.value is _longint) {
                t |> equal(int(js.value as _longint), 7)
            } else {
                t |> failure("unexpected result type")
            }
        }
    }
}

[test]
def test_dispatch_null_return(t : T?) {
    t |> run("null return gives ok") @(t : T?) {
        let r = live_dispatch_command("\{\"name\": \"test_null_return\"}")
        t |> equal(r, "true")
    }
}

[test]
def test_dispatch_errors(t : T?) {
    t |> run("unknown command") @(t : T?) {
        let r = live_dispatch_command("\{\"name\": \"nonexistent\"}")
        t |> success(find(r, "unknown command") >= 0, "should report unknown command")
    }
    t |> run("missing name field") @(t : T?) {
        let r = live_dispatch_command("\{\"foo\": \"bar\"}")
        t |> success(find(r, "missing command name") >= 0, "should report missing name")
    }
    t |> run("invalid JSON") @(t : T?) {
        let r = live_dispatch_command("not json at all")
        t |> success(find(r, "error") >= 0, "should report parse error")
    }
    t |> run("empty string") @(t : T?) {
        let r = live_dispatch_command("")
        t |> success(find(r, "error") >= 0, "should report error for empty input")
    }
    t |> run("non-object JSON") @(t : T?) {
        let r = live_dispatch_command("42")
        t |> success(find(r, "missing command name") >= 0, "non-object has no name field")
    }
}

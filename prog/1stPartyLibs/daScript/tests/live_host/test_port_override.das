options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require live/live_api

[test]
def test_parse_port_string(t : T?) {
    t |> run("valid port") @(t : T?) {
        t |> equal(parse_port_string("19090"), 19090)
        t |> equal(parse_port_string("1"), 1)
        t |> equal(parse_port_string("65535"), 65535)
    }
    t |> run("out-of-range rejected as 0") @(t : T?) {
        // Bug shape: port_is_safe-style "digits only" check accepts 0, 65536,
        // 99999 and lets daslang-live exit with an error after 10s of MCP
        // polling. parse_port_string must reject them upfront.
        t |> equal(parse_port_string("0"), 0)
        t |> equal(parse_port_string("65536"), 0)
        t |> equal(parse_port_string("70000"), 0)
        t |> equal(parse_port_string("99999"), 0)
        t |> equal(parse_port_string("-1"), 0)
    }
    t |> run("non-numeric rejected as 0") @(t : T?) {
        t |> equal(parse_port_string(""), 0)
        t |> equal(parse_port_string("abc"), 0)
        t |> equal(parse_port_string("9090 "), 0)
        t |> equal(parse_port_string(" 9090"), 9090)
    }
}

[test]
def test_parse_argv_port(t : T?) {
    t |> run("--live-port N") @(t : T?) {
        t |> equal(parse_argv_port(["--live-port", "19090"]), 19090)
    }
    t |> run("--live-port=N") @(t : T?) {
        t |> equal(parse_argv_port(["--live-port=19091"]), 19091)
    }
    t |> run("missing flag") @(t : T?) {
        t |> equal(parse_argv_port(["foo", "bar"]), 0)
        t |> equal(parse_argv_port([]), 0)
    }
    t |> run("invalid value") @(t : T?) {
        t |> equal(parse_argv_port(["--live-port", "abc"]), 0)
        t |> equal(parse_argv_port(["--live-port", "0"]), 0)
        t |> equal(parse_argv_port(["--live-port", "65536"]), 0)
    }
    t |> run("last occurrence wins (clargs convention)") @(t : T?) {
        t |> equal(parse_argv_port(["--live-port", "9090", "--live-port", "19090"]), 19090)
    }
    t |> run("last-occurrence-wins applies even when the last value is invalid") @(t : T?) {
        // Bug shape (Copilot #2726 R3 / Thread 9): if C++ kept "last VALID"
        // while .das kept "last RAW (validated to 0 on bad input)", a tail
        // like `--live-port 19090 --live-port abc` would have C++ key its
        // lock on 19090 while .das defaults to 9090 -- lock-vs-bind drift.
        // Both sides must coerce invalid trailing occurrences to 0.
        t |> equal(parse_argv_port(["--live-port", "19090", "--live-port", "abc"]), 0)
        t |> equal(parse_argv_port(["--live-port", "9090", "--live-port", "65536"]), 0)
        t |> equal(parse_argv_port(["--live-port=19091", "--live-port", "0"]), 0)
    }
    t |> run("--live-port after `--` is still found") @(t : T?) {
        // Bug shape (Copilot #2726 R2 / Thread 5): daslang-live's C++ arg
        // loop stops at `--`, but find_flag_raw_value scans the full argv.
        // Both sides must agree on the same source: full argv including
        // post-`--`. If this regresses to a pre-`--` scan on the .das side,
        // lock-key vs bind-port would mismatch under
        //   daslang-live foo.das -- --live-port 19090
        t |> equal(parse_argv_port(["foo.das", "--", "--live-port", "19090"]), 19090)
        t |> equal(parse_argv_port(["--", "--live-port=19092"]), 19092)
    }
}

[test]
def test_resolve_precedence(t : T?) {
    t |> run("argv beats default") @(t : T?) {
        let no_argv : array<string>
        t |> equal(resolve_port_override_from(["--live-port", "11111"]), 11111)
        t |> equal(resolve_port_override_from(no_argv), 0)
    }
    t |> run("invalid argv → 0 (caller keeps default)") @(t : T?) {
        t |> equal(resolve_port_override_from(["--live-port", "99999"]), 0)
        t |> equal(resolve_port_override_from(["--live-port", "abc"]), 0)
    }
}

[test]
def test_set_port_still_wins_before_agent_install(t : T?) {
    t |> run("live_api_set_port mutates live_api_port") @(t : T?) {
        // CAVEAT: live_api_set_port is "before LiveApiAgent constructor"
        // wins -- the server reads live_api_port once at server->init(...)
        // in LiveApiAgent::LiveApiAgent() and never rebinds. After agent
        // install, setter calls only mutate the global (visible to
        // live_api_help_json's interpolation) but do NOT change the bound
        // socket. This test exercises the mutation only; the agent is not
        // installed in this test process.
        let original = live_api_port
        live_api_set_port(54321)
        t |> equal(live_api_port, 54321)
        live_api_set_port(original)
    }
}

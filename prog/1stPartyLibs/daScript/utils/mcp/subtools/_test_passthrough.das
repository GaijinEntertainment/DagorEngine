options gen2

require ../tools/common.das public

//! Test fixture for run_mcp_subtool. Echoes its `--`-separated args back as
//! a `make_tool_result` payload so the wrapper test can assert on the round-trip.

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    let payload = build_string() $(var w) {
        w |> write("PASSTHROUGH:")
        for (a in args) {
            w |> write(" {a}")
        }
    }
    print(make_tool_result(payload))
}

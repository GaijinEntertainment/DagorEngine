options gen2

require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings

// A colored log line must end "<text><reset>\n", never "<text>\n<reset>": a reset escape
// carried past the newline lands at the head of the NEXT stdout line and corrupts
// machine-read output (--bench-format json lines after a colored SUCCESS banner).

def private run_dastest(cmd : string; var output : string&; var exit_code : int&) {
    output = ""
    unsafe {
        exit_code = popen(cmd) $(f) {
            if (f != null) {
                while (!feof(f)) {
                    output += fgets(f)
                }
            }
        }
    }
}

[test]
def test_log_color_framing(tt : T?) {
    tt |> run("no captured line starts with a reset escape under --color") <| @(t : T?) {
        let exe = get_command_line_arguments()[0]
        let cmd = "{exe} {get_das_root()}/dastest/dastest.das -- --color --test {get_das_root()}/dastest/tests/_fixture_passing.das"
        var output : string
        var exit_code : int
        run_dastest(cmd, output, exit_code)
        t |> equal(exit_code, 0)
        let esc = "{to_char(27)}"
        t |> success(output |> contains("{esc}["), "--color must force color escapes on, or this test checks nothing")
        var bled = 0
        for (line in split(output, "\n")) {
            if (starts_with(line, "{esc}[0m")) {
                bled++
            }
        }
        t |> equal(bled, 0, "lines beginning with the reset escape — a colored line carried its newline inside the color wrap")
    }
}

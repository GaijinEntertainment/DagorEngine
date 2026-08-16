expect 30229
expect 30165

options gen2

// compiled by test_static_if_profiles.das: a genuinely non-constant condition must
// still refuse with "static_if must resolve to constant" in every compile profile

def f() {
    static_if (length(get_command_line_arguments()) > 0) {
        print("x\n")
    }
}

[export]
def main() {
    f()
}

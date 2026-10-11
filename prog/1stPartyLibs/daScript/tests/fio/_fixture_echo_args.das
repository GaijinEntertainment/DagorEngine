options gen2

require daslib/clargs

//! one argument per line - tests/fio/popen_argv.das matches on these lines
[export]
def main() {
    for (a in get_cli_arguments()) {
        print("{a}\n")
    }
}

options gen2
options no_aot

require daslib/fio

//! a child for test_process.das: prints one line, then ends through exit_now with code 7 - the
//! parent reads the code exit_now carries out of the process and the line its flush kept
[export]
def main() : int {
    print("before exit_now\n")
    unsafe {
        exit_now(7)
    }
    return 1
}

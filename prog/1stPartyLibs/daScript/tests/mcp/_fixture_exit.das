options gen2

//! Fixture: exit immediately with code 7. Used by
//! ``test_popen_argv_pipe`` to verify exit-code propagation.

[export]
def main() : int {
    return 7
}

options gen2

// A second context in the same binary as the dasHV example, reaching a different C++ module
// set (fio only): the two generated TUs add their module tables to one process-wide list, the
// first constructor registers the union, and one shutdown runs at exit.

require daslib/fio

[export]
def has_path_variable : bool {
    return has_env_variable("PATH")
}

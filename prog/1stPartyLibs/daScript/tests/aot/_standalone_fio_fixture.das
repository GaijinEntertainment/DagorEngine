options gen2

// reaches fio_core through an extern call and nothing else - the module must be linked and
// registered, and its AOT header included

require daslib/fio

[export]
def has_path : bool {
    return has_env_variable("PATH")
}

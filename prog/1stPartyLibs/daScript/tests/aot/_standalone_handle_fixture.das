options gen2

// reaches fio_core only through a handled type in a signature - no extern is called

require daslib/fio

[export]
def is_open(f : FILE?) : bool {
    return f != null
}

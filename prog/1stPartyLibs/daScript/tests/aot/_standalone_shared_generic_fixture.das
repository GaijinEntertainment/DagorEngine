options gen2

require _standalone_dep_gen

[export]
def shared_entry(x : int) : int {
    return twice(x) + dep_use(x)
}

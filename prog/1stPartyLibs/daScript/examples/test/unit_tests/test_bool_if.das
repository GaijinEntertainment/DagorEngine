// options log=true, log_infer_passes=true, optimize=false, log_nodes=true, log_stack=true

[export]
def test
    let a = false
    if a
        assert(false, "can't get here")
    return true


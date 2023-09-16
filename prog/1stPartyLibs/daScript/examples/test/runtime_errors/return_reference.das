// options log=true, log_infer_passes=false, optimize=false
// options log = true, log_optimization_passes=true

options no_aot

[sideeffects]
def ret_unsafe_ref : int &
    var t = 2
    unsafe
        return t

[export]
def test
    ret_unsafe_ref()
    return true

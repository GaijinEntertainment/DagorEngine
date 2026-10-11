options gen2

require _standalone_dep_b

var g_main = g_dep_b + 1

[export]
def get_main : int {
    return g_main
}

[export]
def span_width(s : _standalone_dep_a::DepSpan) : int {
    return s.hi - s.lo
}

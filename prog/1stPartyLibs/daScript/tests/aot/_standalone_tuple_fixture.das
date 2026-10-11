options gen2

[never_inline]
def sum_pair(pair : tuple<int; float>) : float {
    return float(pair._0) + pair._1
}

[export]
def entry() : float {
    return sum_pair((1, 2.0))
}

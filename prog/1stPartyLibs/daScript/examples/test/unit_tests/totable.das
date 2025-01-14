[export]
def test
    let a = [[ auto 1 => "one"; 2 => "two"; 3 => "three" ]]
    var b <- to_table([[ auto 1 => "one"; 2 => "two"; 3 => "three" ]])
    for t in a
        verify(b |> get_value(t._0)==t._1)
    return true


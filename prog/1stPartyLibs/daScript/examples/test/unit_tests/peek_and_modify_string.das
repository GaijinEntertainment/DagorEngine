require daslib/strings_boost

[export]
def test
    var sample = " a + b * ( c << d ) blah blah b"
    peek_data(sample) <| $ ( foo )
        for x in range(length(foo))
            if is_character_at(foo, x, 'b')
                assert(x==5 || x==20 || x==25 || x==30)
    var new_sample = modify_data(sample) <| $ ( var foo )
        for x in range(length(foo))
            if is_character_at(foo, x, 'b')
                foo[x] = uint8('B')
    assert(new_sample==" a + B * ( c << d ) Blah Blah B")
    return true





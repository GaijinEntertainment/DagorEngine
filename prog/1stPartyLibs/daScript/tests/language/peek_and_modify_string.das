options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/strings_boost

[test]
def test_peek_and_modify_string(t : T?) {
    t |> run("peek_data finds characters") @(t : T?) {
        var sample = " a + b * ( c << d ) blah blah b"
        peek_data(sample) <| $(foo) {
            for (x in range(length(foo))) {
                if (is_character_at(foo, x, 'b')) {
                    assert(x == 5 || x == 20 || x == 25 || x == 30)
                }
            }
        }
        t |> success(true)
    }
    t |> run("modify_data replaces characters") @(t : T?) {
        var sample = " a + b * ( c << d ) blah blah b"
        var new_sample = modify_data(sample) <| $(var foo) {
            for (x in range(length(foo))) {
                if (is_character_at(foo, x, 'b')) {
                    foo[x] = uint8('B')
                }
            }
        }
        t |> equal(new_sample, " a + B * ( c << d ) Blah Blah B")
    }
}

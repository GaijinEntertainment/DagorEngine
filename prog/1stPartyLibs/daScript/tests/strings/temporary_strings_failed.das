options gen2
expect 40102:3

require dastest/testing_boost
require daslib/temp_strings

var payload = "long str"

def get_str() : string {
    return payload
}

[test]
def test_temps_strings(t : T?) {

    // unable to use temp_string with a variable
    temp_string(payload) $(s) {// 40102: call annotated by TempStringMacro failed
        feint(s)
    }

    // unable to use temp_string with a function call
    temp_string(get_str()) $(s) {// 40102: call annotated by TempStringMacro failed
        feint(s)
    }

    // and with a string literal
    temp_string("long str\n") $(s) {// 40102: call annotated by TempStringMacro failed
        feint(s)
    }

    // but a string builder works
    temp_string("long str {payload}\n") $(s) {
        feint(s)
    }

    // and this works too
    temp_string("long str {get_str()}\n") $(s) {
        feint(s)
    }
}

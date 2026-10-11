options gen2

// Fixture for test_nolint_suppression.das (hardening half) - LINT003-eligible `var`
// declarations paired with directive spellings the hardened scanner must and must
// not honor; the variable names say which is which.

[export]
def fixture_function() : int {
    var in_string = "// nolint:LINT003 quoted"
    var in_url = 1                  // see http://x/nolint:LINT003
    var mid_comment = 2             // prose before the nolint:LINT003 marker
    var honored = 3                 // nolint:LINT003 reason text after the directive
    var honored_list = 4            // nolint:PERF999,LINT003
    var honored_doc = 5             //! nolint:LINT003
    var quote_char = '"'            // nolint:LINT003
    var quote_escaped = "a\"b"      // nolint:LINT003
    return (length(in_string) + in_url + mid_comment + honored + honored_list
        + honored_doc + quote_char + length(quote_escaped))
}

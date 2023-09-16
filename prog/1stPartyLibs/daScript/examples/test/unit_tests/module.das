options optimize=false

require math
require foo_bar
// require not_a_require
/* require not_a_require_too */

[export]
def test
    if false
        print("require not_a_require_either")
    assert(foo_bar::getValue()==13)
    return true



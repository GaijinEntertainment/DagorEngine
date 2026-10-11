// private property operators of a class are private to the declaring module
// verifies a private setter, a private getter, and an operator of a private class are rejected from another module
options gen2
expect 30915, 30928:2

require _private_property_helper

[export]
def main {
    var g = new Gate()
    g.flag = true                   // 30915: can only copy to a reference - the setter is private
    let h = g.hidden                // 30928: field 'hidden' not found - the getter is private
    let f = make_hidden().flag      // 30928: field 'flag' not found - the class is private, so its operator is too
}

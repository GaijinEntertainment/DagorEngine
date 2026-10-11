// a copy into a property whose only setter is private to another module
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it
expect 30915
options gen2
require _private_setter_helper

def use_it {
    var g = new Gate()
    g.flag = true
}

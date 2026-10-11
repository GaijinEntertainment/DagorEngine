options gen2
module optional_require_fixture_private shared public

require optional_require_fixture_hidden   // private on purpose: the root sees this module, not that one

def private_marker() : int {
    return hidden_marker() + 1
}

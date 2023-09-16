// options log_require = true
// options log = true

require event
require sub/event as sub_event
require sub/event as sub_event  // testing duplicate here

[export]
def test
    verify(event::fn_event())
    verify(!sub_event::fn_event())
    return true

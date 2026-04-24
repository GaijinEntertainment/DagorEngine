// sub/event.das — Sub-directory event module, returns false
//
// Loaded by `require sub/event as sub_event` in alias/main.das.
// The alias-aware resolver resolves "sub/event" to this file (alias/sub/event.das)
// and assigns module name "sub_event" with alias "event".
//
// fn_event() returns false, distinguishing this from the root alias/event.das
// which returns true. The test verifies both coexist correctly via aliasing.

options gen2
def fn_event {
    return false
}

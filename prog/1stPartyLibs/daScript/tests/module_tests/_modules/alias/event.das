// event.das — Root-level event module, returns true
//
// Loaded by `require event` in alias/main.das. The alias project resolver
// resolves "event" to this file (alias/event.das) relative to the requiring file.
//
// fn_event() returns true, distinguishing this from sub/event.das which returns false.

options gen2
def fn_event {
    return true
}

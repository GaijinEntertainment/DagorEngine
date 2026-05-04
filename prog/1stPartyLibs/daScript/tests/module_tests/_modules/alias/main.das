// main.das — Module aliasing test
//
// This test validates the `require ... as ...` module aliasing feature:
//
//   - `require event`              → loads alias/event.das, fn_event() returns true
//   - `require sub/event as sub_event` → loads alias/sub/event.das, fn_event() returns false
//
// The duplicate `require sub/event as sub_event` line tests that the compiler
// handles re-importing the same aliased module gracefully (no error, no duplication).
//
// Both modules define fn_event() but return different values, proving they are
// distinct modules resolved to different files via the alias-aware project resolver.
//
// Compiled with: alias/project.das_project (usePak=true)

options gen2
// options log_require = true
// options log = true

require event
require sub/event as sub_event
require sub/event as sub_event  // testing duplicate here

[export]
def test {
    verify(event::fn_event())
    verify(!sub_event::fn_event())
    return true
}

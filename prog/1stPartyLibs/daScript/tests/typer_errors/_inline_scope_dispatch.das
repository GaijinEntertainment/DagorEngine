// cross-module [inline] callee whose body dispatches through user-spelled `_::`:
// the prefix binds the program module before AND after the splice, so the spliced
// body would silently re-bind - fail-closed instead.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 30100
options gen2
require _inline_scope_dispatch_helper

[export]
def main() {
    print("{dispatched(3)}\n")
}

options gen2

// Same on-disk something.das, but expressed via `../`. After resolver
// normalization the absolute path matches from_root.das's `./something.das`
// → requireEx must dedup onto the SAME promoted Module, not fail with
// "already created".
require ../something.das

[export]
def from_sub_value() : int {
    return get_value()
}

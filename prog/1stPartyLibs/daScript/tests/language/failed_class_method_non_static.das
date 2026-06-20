// [class_method] applied to a non-static class method
// verifies the static-only guard in daslib/class_boost.das ClassMethodMacro.apply
options gen2
expect 20800    // invalid_annotation

require daslib/class_boost

class C {
    value : int = 0

    [class_method]
    def put(v : int) {
        value = v
    }
}

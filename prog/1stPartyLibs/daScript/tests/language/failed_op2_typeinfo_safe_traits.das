// A safe_* typeinfo trait that answers false replaces its node, so it tells infer to run again.
// Each arm takes its value from a nested typeinfo, or from an index that folds, so the trait
// resolves one pass late - in a pass where nothing else changes, which is the pass that catches a
// missing signal. Keep this file free of anything that keeps signalling (a print's string builder
// will do it): that grants the enclosing operator a repair pass and the arms stop proving anything.
// Without the signal every arm reports 50613 instead, the reported input included.
options gen2
expect 30341

[comment(i = 1)]
struct TsrAnnotated {
    x : int
}

var g_tsr_annotated : TsrAnnotated[8]

[export]
def main {
    var a = true
    a &&= typeinfo safe_has_field < no_such_field > (typeinfo is_ref(1))
    var b = true
    b &&= typeinfo struct_safe_has_annotation < no_such_annotation > (typeinfo is_ref(1))
    var c = true
    c &&= typeinfo struct_safe_has_annotation_argument < no_such_annotation; no_such_argument > (typeinfo is_ref(1))
    var d = true
    d &&= typeinfo struct_safe_has_annotation_argument < no_such_annotation; no_such_argument > (g_tsr_annotated[typeinfo sizeof(type<int>)])
    1 &&= typeinfo safe_has_field < no_such_type > (typeinfo is_ref(1))
}

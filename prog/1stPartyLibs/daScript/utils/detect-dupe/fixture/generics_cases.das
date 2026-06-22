options gen2


// Non-generic baseline — must be visible to detect-dupe.
def case_mono_baseline(a, b : int) : int {
    return a + b
}


// Generic template that IS called — the reified instance lands in
// `module->functions`, so the current pipeline sees it (one record
// after the per-source-line dedup in pipeline.das).
def case_generic_called(a, b : auto(T)) : T {
    return a + b
}


// Generic template that is NEVER called — stays in `module->generics`
// only. Was invisible to the tool before `collect_from_program` started
// walking `for_each_generic` alongside `for_each_function`; the
// `test_generics_uncalled_visible` regression test pins that fix.
def case_generic_uncalled(a, b : auto(T)) : T {
    return a + b
}


// Drive a reification of `case_generic_called` so the called-generic
// case is meaningfully exercised. Without this, even the called path
// would miss it.
def anchor_call_site() : int {
    return case_generic_called(1, 2)
}

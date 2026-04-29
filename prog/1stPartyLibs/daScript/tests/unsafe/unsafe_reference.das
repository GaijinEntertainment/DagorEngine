options gen2
expect 31300

options persistent_heap = true
options gc

require dastest/testing_boost public

[test]
def test_unsafe_reference(t : T?) {
    let rt & = t   // 31300: local reference to non-local expression is unsafe
}


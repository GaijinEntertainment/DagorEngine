options gen2
require dastest/testing_boost public

[comment(i=1, f=2.0, s="three", b=false)]
struct Event {
    i : int
}

[test]
def test_typeinfo(t : T?) {
    let arg : int
    let a = typeinfo sizeof(arg)
    t |> equal(4, a)
    let b = typeinfo has_field<i>(type<Event>)
    t |> equal(true, b)
    let c = typeinfo struct_get_annotation_argument<comment; s>(type<Event>)
    t |> equal("three", c)
}


options gen2
require dastest/testing_boost public

struct TaiFoo {
    i : int
}

[comment(i=1, f=2.0, s="three", b=false)]
struct TaiEvent {
    i : int
}

[test]
def test_typeinfo_annotations(t : T?) {
    t |> run("struct_has_annotation") @(t : T?) {
        t |> success(typeinfo struct_has_annotation < comment > (type<TaiEvent>))
        t |> success(!typeinfo struct_has_annotation < comment > (type<TaiFoo>))
    }
    t |> run("annotation argument queries") @(t : T?) {
        t |> success(typeinfo struct_has_annotation_argument<comment; i>(type<TaiEvent>))
        t |> success(!typeinfo struct_safe_has_annotation_argument < comment; i > (type<TaiFoo>))
        t |> success(typeinfo struct_has_annotation_argument<comment; f>(type<TaiEvent>))
        t |> success(typeinfo struct_has_annotation_argument<comment; s>(type<TaiEvent>))
        t |> success(typeinfo struct_has_annotation_argument<comment; b>(type<TaiEvent>))
        t |> success(!typeinfo struct_has_annotation_argument<comment; q>(type<TaiEvent>))
    }
    t |> run("get annotation argument values") @(t : T?) {
        let i = typeinfo struct_get_annotation_argument<comment; i>(type<TaiEvent>)
        let f = typeinfo struct_get_annotation_argument<comment; f>(type<TaiEvent>)
        let s = typeinfo struct_get_annotation_argument<comment; s>(type<TaiEvent>)
        let b = typeinfo struct_get_annotation_argument<comment; b>(type<TaiEvent>)
        t |> equal(i, 1)
        t |> equal(f, 2.0)
        t |> equal(s, "three")
        t |> equal(b, false)
    }
}

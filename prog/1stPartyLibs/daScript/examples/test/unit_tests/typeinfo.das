struct Foo
    i : int

[comment (i=1,f=2.0,s="three",b=false)]
struct Event
    i : int

[export]
def test
    assert(typeinfo(struct_has_annotation<comment> type<Event>))
    assert(!typeinfo(struct_has_annotation<comment> type<Foo>))
    assert(typeinfo(struct_has_annotation_argument<comment;i> type<Event>))
    assert(!typeinfo(struct_safe_has_annotation_argument<comment;i> type<Foo>))
    assert(typeinfo(struct_has_annotation_argument<comment;f> type<Event>))
    assert(typeinfo(struct_has_annotation_argument<comment;s> type<Event>))
    assert(typeinfo(struct_has_annotation_argument<comment;b> type<Event>))
    assert(!typeinfo(struct_has_annotation_argument<comment;q> type<Event>))

    let i = typeinfo(struct_get_annotation_argument<comment;i> type<Event>)
    let f = typeinfo(struct_get_annotation_argument<comment;f> type<Event>)
    let s = typeinfo(struct_get_annotation_argument<comment;s> type<Event>)
    let b = typeinfo(struct_get_annotation_argument<comment;b> type<Event>)

    assert(i==1)
    assert(f==2.0)
    assert(s=="three")
    assert(b==false)

    return true

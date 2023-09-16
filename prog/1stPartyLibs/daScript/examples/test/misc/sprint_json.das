options rtti

enum AnyThing
    any
    thing

variant Moo
    i : int
    f : float

struct Bar
    arr_int : array<int>

struct Foo
    v_int : int
    bar : Bar
    v_float : float
    en1 : AnyThing
    [[enum_as_int]] en2 : AnyThing
    [[rename="type"]] _type : uint64
    v_str : string
    [[unescape]] v_unescaped_str : string
    [[embed]] v_embed : string
    tab : table<string;int>
    tup : tuple<int;float>
    v_var : Moo
    ptr : void?
    [[optional]] empty_str : string
    [[optional]] empty_arr : array<int>
    [[optional]] empty_tab : table<string;int>

[export]
def main
    var a <- [[Foo
        v_int = 1,
        bar <- [[Bar
            arr_int <- [{auto 1;2;3}]
        ]],
        v_float = 2.34,
        en1 = AnyThing any,
        en2 = AnyThing thing,
        _type = 0x1234567890abcdeful,
        v_str = "hello\nworld \{ 2,3,4 \}",
        v_unescaped_str = "hello\nworld",
        v_embed = "[22,32,43]",
        tab <- {{ "hello" => 1; "world" => 2 }},
        tup = [[auto 3,.141592]],
        v_var = [[Moo f=3.141592]],
        ptr = null
    ]]
    let t = sprint_json(a,true)
    print("t = {t}\n")

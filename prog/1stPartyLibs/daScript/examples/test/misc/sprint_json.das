options gen2
options rtti

enum AnyThing {
    any
    thing
}

variant Moo {
    i : int
    f : float
}

struct Bar {
    arr_int : array<int>
    @optional v_bool_opt : bool
    @optional v_float_opt : float
}

class CFoo {
    @optional v_int : int
    @optional foo : CFoo?
}

struct Foo {
    v_int : int
    bars : array<Bar>
    v_float : float
    @optional v_float_opt : float
    v_bool : bool
    @optional v_bool_opt : bool
    @optional v_bool_opt2 : bool
    en1 : AnyThing
    @enum_as_int en2 : AnyThing
    @rename _type : uint64
    v_str : string
    @unescape v_unescaped_str : string
    @embed v_embed : string
    tab : table<string; int>
    tup : tuple<int; float>
    v_var : Moo
    ptr : void?
    @optional empty_str : string
    @optional empty_arr : array<int>
    @optional empty_tab : table<string; int>
    @optional cfoo : CFoo?
}


[export]
def main {
    var a <- Foo(uninitialized
        v_int = 1,
        bars = [Bar(
            arr_int = [ 1, 2, 3]
        )],
        v_bool = true,
        v_bool_opt = true,
        v_float = 2.34,
        en1 = AnyThing.any,
        en2 = AnyThing.thing,
        _type = 0x1234567890abcdeful,
        v_str = "hello\nworld \{ 2,3,4 \}",
        v_unescaped_str = "hello\nworld",
        v_embed = "[22,32,43]",
        tab <- { "hello" => 1, "world" => 2 },
        tup = (3, .141592),
        v_var = Moo(uninitialized f = 3.141592),
        ptr = null,
        cfoo = new CFoo(
            v_int = 42
        )
    )
    a.cfoo.foo = a.cfoo // self-reference
    let t = sprint_json(a, true)
    print("t = {t}\n")
}

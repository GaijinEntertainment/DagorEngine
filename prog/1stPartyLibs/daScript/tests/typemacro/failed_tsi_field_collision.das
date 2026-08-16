options gen2
expect 20503

require daslib/typemacro_boost

[ |> template_struct_instance]
struct template FT {
    x : KT
    n : int = 1
}

struct FInst : FT {
    typedef KT = int
    n : int = 2 // redeclared without `override` — the parser rejects this before the reifier runs
}

[export]
def main {
}

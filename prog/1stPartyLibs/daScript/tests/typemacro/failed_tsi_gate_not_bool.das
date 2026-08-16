options gen2
expect 20800

require daslib/typemacro_boost

[ |> template_struct_instance]
struct template GT {
    x : KT
    @template_gate = N y : KT // N is an int constant — a gate must name a BOOL axis
    @template_constant N : int = 1
}

struct GInst : GT {
    typedef KT = int
}

[export]
def main {
}

options gen2
expect 20800

require daslib/typemacro_boost

[ |> template_struct_instance]
struct template GT {
    x : KT
    @template_gate = NO_SUCH_AXIS y : KT
    @template_constant BATCHED : bool = false
}

struct GInst : GT {
    typedef KT = int
}

[export]
def main {
}

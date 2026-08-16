options gen2
expect 30826

require daslib/typemacro_boost

[ |> template_struct_instance]
struct template NT {
    x : KT
    y : VT
}

struct NInst : NT {
    typedef KT = int
    // VT deliberately unbound: falls through to infer, which reports the undefined type at the template field
}

[export]
def main {
}

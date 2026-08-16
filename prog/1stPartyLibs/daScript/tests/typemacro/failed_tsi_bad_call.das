options gen2
expect 20800

require daslib/typemacro_boost

def pick : int {
    return 1
}

[ |> template_struct_instance]
struct template BT {
    x : KT
    @template_call op = "valid_name"
}

struct BInst : BT {
    typedef KT = int
    override op = pick() // neither @@name nor a string constant
}

[export]
def main {
}

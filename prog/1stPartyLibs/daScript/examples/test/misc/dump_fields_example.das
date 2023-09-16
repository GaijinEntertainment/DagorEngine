require dump_fields

[dump_fields]
struct Foo
    position : float3
    velocity : float3
    color    : float4
    foo : float = 13.

[export]
def main
    print("// at this point compilation is done\n")

require tutorial_04

// uncomment the section to generate C++ bindings for the TutorialBaseClass
// this code will generate tutorial04_gen.inc which contains C++ bindings
/*
require fio
require ast
require daslib/cpp_bind
[init]
def generate_cpp_bindings
    let root = get_das_root() + "/examples/tutorial/"
    fopen(root + "tutorial04_gen.inc","wb") <| $ ( cpp_file )
        log_cpp_class_adapter(cpp_file, "TutorialBaseClass", typeinfo(ast_typedecl type<TutorialBaseClass>))
*/

require rtti

class ExampleObject : TutorialBaseClass
    position : float3
    velocity : float3
    gravity  : float = 9.8f
    name     : string
    def override update ( dt : float ) : void
        print("update {name} for {dt}\n")
        position += velocity * dt
        velocity.y -= gravity * dt
    def override get_position : float3
        print("get position for {name} => {position}\n")
        return position

def add_new_object ( classPtr )
    add_object(classPtr, class_info(*classPtr))

[export]
def test
    print("this tutorial shows possible implementation for deriving from C++ base class\n")
    print("tick = {tick(0.0)}\n")
    add_new_object ( new [[ExampleObject() name="A"]] )
    for t in range(5)
        print("tick = {tick(0.2)}\n")
    add_new_object ( new [[ExampleObject() name="B"]] )
    for t in range(5)
        print("tick = {tick(0.2)}\n")

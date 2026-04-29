options gen2
require tutorial_19

// We need RTTI to pass StructInfo to C++ via class_info().
require daslib/rtti

// Derive a daslang class from the C++ abstract base class.
// Override the virtual methods — these will be called from C++ through
// the class adapter's virtual dispatch bridge.

class ExampleObject : TutorialBaseClass {
    position : float3
    velocity : float3
    gravity  : float = 9.8f
    name     : string
    def override update(dt : float) : void {
        print("  [das] {name}.update({dt})\n")
        position += velocity * dt
        velocity.y -= gravity * dt
    }
    def override get_position : float3 {
        print("  [das] {name}.get_position => {position}\n")
        return position
    }
}

// Helper that constructs an object and registers it with C++.
def add_new_object(classPtr) {
    add_object(classPtr, class_info(*classPtr))
}

[export]
def test {
    print("Class adapter tutorial: daslang classes implementing C++ interfaces\n\n")
    // No objects yet — tick returns zero.
    print("tick(0.0) = {tick(0.0)}\n\n")
    // Register object A and simulate a few ticks.
    add_new_object(new ExampleObject(name = "A"))
    for (t in range(3)) {
        print("tick(0.1) = {tick(0.1)}\n\n")
    }
    // Register object B and simulate more ticks.
    add_new_object(new ExampleObject(name = "B", velocity = float3(1.0, 0.0, 0.0)))
    for (t in range(3)) {
        print("tick(0.1) = {tick(0.1)}\n\n")
    }
}

options gen2
options remove_unused_symbols = false

// Abstract base class — daslang side.
// C++ will include this via compileBuiltinModule and the XDD .das.inc file.
// daslang classes can derive from this and override the virtual methods.

class TutorialBaseClass {
    def abstract update(dt : float) : void
    def abstract get_position : float3
}

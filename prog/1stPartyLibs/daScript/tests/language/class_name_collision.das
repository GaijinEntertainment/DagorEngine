options gen2
require dastest/testing_boost public

require _finalize_collision_helper

// A class with the same name as a class in a required module. The generated
// __finalize field init must pin to this module's Doppel'__finalize - the old
// `_::` spelling resolved across all visible modules, found both, and failed
// with error[30810] on the class declaration itself.
class Doppel {
    y : int = 2
    def value : int {
        return y
    }
}

// deriving from the required module's twin while the local Doppel exists - the
// derived class regenerates __finalize in THIS module and must pin there too
class DoppelChild : _finalize_collision_helper::Doppel {
    z : int = 3
}

[test]
def test_class_name_collision(t : T?) {
    t |> run("same-named class across modules compiles") @(t : T?) {
        t |> success(true)
    }
    t |> run("derive from the required module's twin") @(t : T?) {
        var c = new DoppelChild()
        t |> equal(1, c.x)
        t |> equal(3, c.z)
        unsafe {
            delete c
        }
    }
}

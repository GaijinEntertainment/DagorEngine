options gen2
require dastest/testing_boost public

require _finalize_collision_helper public

// The harder half of the finalizer collision: the twin arrives through a PUBLIC
// require. Module-name-filtered lookup also walks the named module's public
// dependencies, so without the strict `__::` filter the generated finalizer was
// still ambiguous here - and `require dastest/testing_boost public` makes this
// the standard test-file shape.
class Doppel {
    y : int = 4
}

class DoppelChild : _finalize_collision_helper::Doppel {
    z : int = 5
}

[test]
def test_class_name_collision_public(t : T?) {
    t |> run("same-named class across a public require compiles") @(t : T?) {
        var c = new DoppelChild()
        t |> equal(1, c.x)
        t |> equal(5, c.z)
        unsafe {
            delete c
        }
    }
}

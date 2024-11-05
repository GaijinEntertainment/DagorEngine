options persistent_heap = true
options gc

module test_stages_extra public

require daslib/decs_boost
require dastest/testing_boost public

[decs(stage="make_invisible")]
def make_invisible ( var visible:bool )
    visible = false

[decs(stage="check_visible")]
def check_visible ( visible:bool )
    assert(visible)

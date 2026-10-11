options gen2
// Helper for style030_unused_require.das: an [init]-only module that nothing
// requires directly - it is reached only through _style030_hook_umbrella.
module _style030_init_only shared private

var private armed = 0

[init]
def arm_hook {
    armed ++
}

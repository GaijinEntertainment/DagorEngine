options gen2

module _style030_hook_umbrella

// Umbrella the client references nothing from: its one job is the hook-only
// module it requires as a side effect, so dropping it drops the hook.
require _style030_init_only

def style030_hook_umbrella_value() : int {
    return 3
}

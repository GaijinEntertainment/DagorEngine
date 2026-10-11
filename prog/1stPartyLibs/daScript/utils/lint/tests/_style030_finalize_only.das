options gen2
// Helper for style030_unused_require.das: [finalize] stands in for the whole
// hook set - the rule matches by annotation name, so one arm covers all.
module _style030_finalize_only shared private

var private touched = 0

[finalize]
def shutdown_hook {
    touched ++
}

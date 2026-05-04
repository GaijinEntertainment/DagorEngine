options gen2
options indenting = 4
options unsafe_table_lookup = false
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module profiler_boost shared private

//! Profiler cross-context helpers.
//!
//! Provides ``[apply_in_context(profiler)]`` wrappers that allow enabling
//! and disabling the profiler from a different context.
//! Requires the ``profiler`` module.

require daslib/profiler
require daslib/rtti public

def public enable_profiler(var ctx : Context) {
    //! Enables the profiler for the given context.
    profiler::set_enable_profiler(intptr(unsafe(addr(ctx))), true)
}

def public disable_profiler(var ctx : Context) {
    //! Disables the profiler for the given context.
    profiler::set_enable_profiler(intptr(unsafe(addr(ctx))), false)
}

def public enable_profiler_log(var ctx : Context) {
    //! Enables the profiler log for the given context.
    profiler::set_enable_profiler_log(intptr(unsafe(addr(ctx))), true)
}

def public disable_profiler_log(var ctx : Context) {
    //! Disables the profiler log for the given context.
    profiler::set_enable_profiler_log(intptr(unsafe(addr(ctx))), false)
}


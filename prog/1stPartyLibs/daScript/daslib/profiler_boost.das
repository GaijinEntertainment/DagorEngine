options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module profiler_boost shared private

require daslib/profiler
require daslib/apply_in_context

[apply_in_context(profiler)]
def internal_enable_profiler(var ctxId:uint64; enabled:bool)
    unsafe
        lock_debug_agent <|
            profiler::set_enable_profiler(ctxId,enabled)

def public enable_profiler(var ctx:Context)
    internal_enable_profiler(intptr(unsafe(addr(ctx))),true)

def public disable_profiler(var ctx:Context)
    internal_enable_profiler(intptr(unsafe(addr(ctx))),false)

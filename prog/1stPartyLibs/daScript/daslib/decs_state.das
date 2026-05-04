options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot
options strict_smart_pointers = true

module decs_state shared private

//! DECS debugger state reporting.
//!
//! Sends entity and archetype data to the debug-agent context
//! for live inspection in the DAP debugger's variable view.
//! Requires the ``decs`` module.

require daslib/decs
require daslib/strings_boost
require daslib/debugger

def report_to_debugger(var ctx : Context; category, name : string; value : auto(TT)) {
    let tinfo = typeinfo rtti_typeinfo(value)
    let pdata = unsafe(addr(value))
    report_context_state(ctx, category, name, unsafe(addr(tinfo)), pdata)
}

struct EcsRequestView {
    //! Read-only view of an ECS request for debug inspection.
    hash        : uint64
    REQUIRE     : string
    REQUIRE_NOT : string
    archetype   : array<uint64>
    file        : string
    line        : int
}

struct EcsComponentView {
    //! Read-only view of an ECS component for debug inspection.
    name        : string
    vtype       : string
    values      : string
    stride      : int
}

struct EcsArchetypeView {
    //! Read-only view of an ECS archetype for debug inspection.
    hash        : uint64
    size        : int
    components  : array<EcsComponentView>
}

class ContextStateAgent : DapiDebugAgent {
    //! Debug agent that exposes DECS state for inspection.
    def override onCollect(var ctx : Context; at : LineInfo) {
        let pState = unsafe(get_context_global_variable(ctx, "decsState"))
        if (pState == null) {
            return
        }
        let decsState = unsafe(reinterpret<DecsState? const>(pState))
        for (arch in decsState.allArchetypes) {
            var arq : EcsArchetypeView
            arq.hash = arch.hash
            arq.size = arch.size
            arq.components |> reserve(length(arch.components))
            for (c in arch.components) {
                var cv : EcsComponentView
                cv.name = c.name
                cv.vtype = c.info.fullName
                cv.stride = c.stride
                if (length(c.data) > 0) {
                    var tinfo : TypeInfo const?
                    let fnTypeInfo = c.info.mkTypeInfo
                    unsafe {
                        invoke_in_context(ctx) <| @ capture(& tinfo) {
                            tinfo = invoke(fnTypeInfo)
                        }
                    }
                    var arr : array<uint8>
                    unsafe(_builtin_make_temp_array(arr, unsafe(addr(c.data[0])), arch.size))
                    cv.values = sprint_data(unsafe(addr(arr)), tinfo, print_flags.singleLine)
                }
                arq.components |> push(cv)
            }
            report_to_debugger(ctx, "DECS archetype", "{arq.hash}", arq)
            delete arq
        }
        // report entity requests, pretty version
        for (eq in decsState.ecsQueries) {
            var prq : EcsRequestView
            prq.file = eq.at.file
            prq.line = int(eq.at.line)
            prq.hash = eq.hash
            if (!empty(eq.req)) {
                prq.REQUIRE = join(eq.req, ",")
            }
            if (!empty(eq.reqn)) {
                prq.REQUIRE_NOT = join(eq.reqn, ",")
            }
            prq.archetype <- [for (a in eq.archetypes); decsState.allArchetypes[a].hash]
            report_to_debugger(ctx, "DECS requests", "{prq.hash}", prq)
            delete prq
        }
    }
}


[export]
def decs_state_agent(ctx : Context) {
    install_new_debug_agent(new ContextStateAgent(), "decs")
}

[init, export]
def private auto_start_decs_state() {
    if (!is_in_debug_agent_creation()) {
        if (!has_debug_agent_context("decs")) {
            fork_debug_agent_context(@@decs_state_agent)
        }
    }
}

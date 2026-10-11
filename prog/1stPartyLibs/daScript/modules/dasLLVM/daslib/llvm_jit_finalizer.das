options gen2
options indenting = 4

module llvm_jit_finalizer shared public

require jit

[finalize(late), no_jit]
def public free_jit_dll {
    if (get_context_share_counter() == 1ul) {
        get_jit_state() $(dll, _ee, _ctx : void?) {
            if (dll != null) {
                close_dynamic_library(dll)
            }
        }
    }
}

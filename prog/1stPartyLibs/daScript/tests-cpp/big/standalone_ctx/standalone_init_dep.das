options gen2

module standalone_init_dep public

var g_dep = 30
var g_dep_init_stamp = 0

//! a literal, not next_stamp(), so the entry module's stamp ladder stays independent of this one
[init]
def dep_record_init {
    g_dep_init_stamp = 11
}

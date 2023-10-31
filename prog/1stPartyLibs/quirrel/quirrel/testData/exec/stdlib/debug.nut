let dbg = require("debug")

let info = dbg.getbuildinfo()

println($"Version: {info.version}")
println($"seterrorhandler: {type(dbg.seterrorhandler)}")
println($"setdebughook: {type(dbg.setdebughook)}")
println($"getstackinfos: {type(dbg.getstackinfos)}")
println($"error: {type(error)}")
println($"errorln: {type(errorln)}")

if (info.gc == "enabled") {
    if (type(dbg.collectgarbage) == "function") {
        println($"collectgarbage: OK")
    } else {
        println($"collectgarbage: FAIL")
    }
    if (type(dbg.resurrectunreachable) == "function") {
        println($"resurrectunreachable: OK")
    } else {
        println($"resurrectunreachable: FAIL")
    }
} else {
    if (dbg?.collectgarbage == null) {
        println($"collectgarbage: OK")
    } else {
        println($"collectgarbage: FAIL")
    }
    if (dbg?.resurrectunreachable == null) {
        println($"resurrectunreachable: OK")
    } else {
        println($"resurrectunreachable: FAIL")
    }
}

println($"getobjflags: {type(getobjflags)}")
println($"getbuildinfo: {type(dbg.getbuildinfo)}")

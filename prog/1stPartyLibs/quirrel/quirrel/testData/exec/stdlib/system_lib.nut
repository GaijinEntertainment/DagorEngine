let { getenv, setenv, system, remove, rename } = require("system")
let { file } = require("io")

setenv("QUIRREL_STDLIB_TEST_VAR", "hello42")
assert(getenv("QUIRREL_STDLIB_TEST_VAR") == "hello42")

setenv("QUIRREL_STDLIB_TEST_VAR", "updated")
assert(getenv("QUIRREL_STDLIB_TEST_VAR") == "updated")

let rc = system("cd .")
assert(typeof rc == "integer")

let f = file("qtest_sys_tmp.txt", "wb")
assert(typeof f == "file")
f.close()

rename("qtest_sys_tmp.txt", "qtest_sys_tmp2.txt")
remove("qtest_sys_tmp2.txt")

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

mustThrow("getenv",    @() getenv(null))
mustThrow("setenv.1",  @() setenv(null, "v"))
mustThrow("setenv.2",  @() setenv("k", null))
mustThrow("system",    @() system(null))
mustThrow("remove",    @() remove(null))
mustThrow("rename.1",  @() rename(null, "x"))
mustThrow("rename.2",  @() rename("x", null))

println("ok")

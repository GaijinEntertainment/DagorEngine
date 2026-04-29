let io = require("io")
let { file } = io
let { remove } = require("system")

let f = file("qtest_file_io.txt", "wb+")
assert(typeof f == "file")
f.close()

let f2 = file("qtest_file_io.txt", "rb")
assert(typeof f2 == "file")
f2.close()

remove("qtest_file_io.txt")

// userpointer branch: stdout/stdin/stderr are wrapped via sqstd_createfile
assert(typeof io.stdout == "file")
assert(typeof io.stdin == "file")
assert(typeof io.stderr == "file")

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label}") } catch (_) {}
}

mustThrow("file.ctor.path=null",  @() file(null, "rb"))
mustThrow("file.missing",         @() file("__no_such_file_XYZ_123__.txt", "rb"))
mustThrow("file.str+int",         @() file("qtest_any.txt", 5))
mustThrow("file.str+null",        @() file("qtest_any.txt", null))
mustThrow("file.bad_mode.letter", @() file("qtest_bad1.txt", "zzz"))
mustThrow("file.bad_mode.empty",  @() file("qtest_bad2.txt", ""))
mustThrow("file.bad_mode.suffix", @() file("qtest_bad3.txt", "r?"))

println("ok")

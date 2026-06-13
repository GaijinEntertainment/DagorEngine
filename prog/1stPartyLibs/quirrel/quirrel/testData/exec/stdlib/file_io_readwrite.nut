// Coverage: real file read/write/seek/tell/len/eos/flush drive the SQFile
// virtual methods and sqstd_fread/fwrite/fseek/ftell/fflush/feof in sqstdio.cpp
// (existing stream tests only use in-memory blobs, so SQFile::* stayed at 0%).

let io = require("io")
let { file } = io
let { blob } = require("iostream")
let { remove } = require("system")

let path = "qtest_io_rw_unique.bin"

// --- write a mix of numeric formats + a string + a blob to a real file ---
let f = file(path, "wb+")
f.writen(-5, 'c')
f.writen(250, 'b')
f.writen(1000, 'i')
f.writen(123456789, 'l')
f.writen(3.5, 'f')
f.writen(2.75, 'd')
f.writestring("HELLO")

let payload = blob()
payload.writen(0x11223344, 'i')
f.writeblob(payload)

let total = 1 + 1 + 4 + 8 + 4 + 8 + 5 + 4
assert(f.len() == total, $"len {f.len()} != {total}")
assert(f.tell() == total)

// --- flush, then seek + read everything back (SQFile::Seek/Tell/Read) ---
assert(f.flush() != null)

f.seek(0, 'b')
assert(f.tell() == 0)
assert(f.readn('c') == -5)
assert(f.readn('b') == 250)
assert(f.readn('i') == 1000)
assert(f.readn('l') == 123456789)
assert(f.readn('f') == 3.5)
assert(f.readn('d') == 2.75)
assert(f.readblob(5).as_string() == "HELLO")
assert(f.readn('i') == 0x11223344)

// --- end-of-stream / eof handling (SQFile::EOS, feof) ---
f.seek(0, 'e')
assert(f.tell() == total)
assert(f.eos() != null)

// seek relative to current and from end
f.seek(-4, 'c')
assert(f.tell() == total - 4)
f.seek(-total, 'e')
assert(f.tell() == 0)

// reading past the end must raise an "io error" (SQFile::Read short read)
f.seek(0, 'e')
local threw = false
try { f.readn('i') } catch (e) { threw = true }
assert(threw, "expected io error reading past end")

f.close()

// --- reopen read-only and verify persistence ---
let f2 = file(path, "rb")
f2.seek(0, 'b')
assert(f2.readn('c') == -5)
assert(f2.len() == total)
f2.close()

function writen_bug() {
  let { file } = require("io")
  local f = file(path, "r")
  local errN = null
  try {
    f.writen(123, 'i')
  } catch (e) {
    errN = e
  }
  local errS = null
  try {
    f.writestring("x")
  } catch (e) {
    errS = e
  }
  f.close()
  assert(errN != null)
  assert(errS != null)
}

writen_bug()
remove(path)
println("file io rw: OK")

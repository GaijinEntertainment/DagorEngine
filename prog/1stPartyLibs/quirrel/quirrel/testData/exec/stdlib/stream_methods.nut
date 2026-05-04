let { blob } = require("iostream")

let b = blob()

b.writen(-5, 'c')
b.writen(250, 'b')
b.writen(100, 's')
b.writen(200, 'w')
b.writen(42, 'i')
b.writen(256, 'l')
b.writen(3.5, 'f')
b.writen(2.75, 'd')

let expectedLen = 1 + 1 + 2 + 2 + 4 + 8 + 4 + 8
assert(b.len() == expectedLen)
assert(b.tell() == expectedLen)

b.seek(0, 'b')
assert(b.tell() == 0)

b.seek(0, 'e')
assert(b.tell() == b.len())

b.seek(-4, 'c')
assert(b.tell() == b.len() - 4)

b.seek(0)
assert(b.tell() == 0)

assert(b.eos() == null)
b.seek(0, 'e')
assert(b.eos() != null)

b.seek(0)
assert(b.readn('c') == -5)
assert(b.readn('b') == 250)
assert(b.readn('s') == 100)
assert(b.readn('w') == 200)
assert(b.readn('i') == 42)
assert(b.readn('l') == 256)
let ff = b.readn('f')
assert(ff == 3.5)
let dd = b.readn('d')
assert(dd == 2.75)

let src = blob()
src.writen(0xAA, 'b')
src.writen(0xBB, 'b')
src.writen(0xCC, 'b')
src.seek(0)

let dst = blob()
dst.writeblob(src)
assert(dst.len() == 3)
dst.seek(0)
let rd = dst.readblob(3)
assert(typeof rd == "blob")
assert(rd.len() == 3)
assert(rd[0] == 0xAA)
assert(rd[1] == 0xBB)
assert(rd[2] == 0xCC)

let sb = blob()
sb.writestring("abc")
assert(sb.len() == 3)
sb.seek(0)
assert(sb.readn('b') == 97)
assert(sb.readn('b') == 98)
assert(sb.readn('b') == 99)

b.flush()

let nb = blob()
nb.writeobject([1, 2, 3], null)
nb.seek(0)
let nres = nb.readobject(null)
assert(nres.len() == 3 && nres[0] == 1)

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

let z = blob()
z.writen(1, 'b')
z.seek(0)

mustThrow("readblob",    @() z.readblob(null))
mustThrow("readn",       @() z.readn(null))
mustThrow("writeblob",   @() z.writeblob(null))
mustThrow("writestring", @() z.writestring(null))
mustThrow("writen.1",    @() z.writen(null, 'b'))
mustThrow("writen.2",    @() z.writen(1, null))
mustThrow("seek.1",      @() z.seek(null))
mustThrow("seek.2",      @() z.seek(0, null))

println("ok")

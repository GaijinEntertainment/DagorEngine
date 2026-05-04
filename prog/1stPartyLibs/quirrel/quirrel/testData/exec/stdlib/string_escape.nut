let { escape } = require("string")

let bs = "\\"  // single backslash
let dq = "\""  // double-quote
let sq = "'"   // single-quote

// --- empty string: returned unchanged ---
assert(escape("") == "")
assert("".escape() == "")

// --- both call forms produce identical results ---
assert("hello".escape() == escape("hello"))

// --- all-printable, no special chars: pass through unchanged ---
assert("hello".escape() == "hello")
assert("Hello World!".escape() == "Hello World!")
assert("0123456789".escape() == "0123456789")
assert("AaBbCcZz".escape() == "AaBbCcZz")
assert(" !#$%&()*+,-./:;<=>?@[]^_`{|}~".escape() == " !#$%&()*+,-./:;<=>?@[]^_`{|}~")

// --- named escape chars: backslash, double-quote, single-quote ---
assert(bs.escape()  == bs + bs)
assert(dq.escape()  == bs + dq)
assert(sq.escape()  == bs + sq)
assert((bs + dq + sq).escape() == bs + bs + bs + dq + bs + sq)

// --- control chars: \a \b \t \n \v \f \r all go to \xNN path ---
// (the named-escape switch cases for these are dead code — they live inside
//  the sq_isprint() branch which is never entered for control chars)
assert("\x01".escape() == "\\x01")
assert("\x06".escape() == "\\x06")
assert("\x07".escape() == "\\x07")  // \a
assert("\x08".escape() == "\\x08")  // \b
assert("\x09".escape() == "\\x09")  // \t
assert("\x0a".escape() == "\\x0a")  // \n
assert("\x0b".escape() == "\\x0b")  // \v
assert("\x0c".escape() == "\\x0c")  // \f
assert("\x0d".escape() == "\\x0d")  // \r
assert("\x0e".escape() == "\\x0e")
assert("\x1f".escape() == "\\x1f")

// --- 0x7f (DEL) ---
assert("\x7f".escape() == "\\x7f")

// --- high bytes 0x80..0xff ---
assert("\x80".escape() == "\\x80")
assert("\xab".escape() == "\\xab")
assert("\xff".escape() == "\\xff")

// --- mixed: printable + control + special ---
assert("a\x01b".escape()       == "a\\x01b")
assert("a\nb".escape()         == "a\\x0ab")
assert(("a" + bs + "b").escape() == "a\\\\b")
assert(("a" + dq + "b").escape() == "a\\\"b")

// from existing string.nut (extended): ~ LF " ' \ ~
// expected: ~\x0a\"\'\\~ (12 chars)
let mixed_escaped = ("~\n" + dq + sq + bs + "~").escape()
assert(mixed_escaped.len() == 12)
assert(mixed_escaped[ 0] == '~')   // literal ~
assert(mixed_escaped[ 1] == '\\')  // start of \x0a
assert(mixed_escaped[ 2] == 'x')
assert(mixed_escaped[ 3] == '0')
assert(mixed_escaped[ 4] == 'a')
assert(mixed_escaped[ 5] == '\\')  // start of \"
assert(mixed_escaped[ 6] == '"')
assert(mixed_escaped[ 7] == '\\')  // start of \'
assert(mixed_escaped[ 8] == '\'')
assert(mixed_escaped[ 9] == '\\')  // start of \\
assert(mixed_escaped[10] == '\\')
assert(mixed_escaped[11] == '~')   // literal ~

// --- output length is always >= input length ---
assert("abc".escape().len() >= 3)
assert("\x01\x02\x03".escape().len() >= 3)

// --- regression: all-non-printable strings (every char -> 4-byte \xNN) ---
// Before the fix, scsprintf received the total buffer size (destcharsize) rather
// than the remaining bytes, causing a 1-byte null-terminator OOB write when
// every character in the input required hex escaping.
assert("\x01".escape()        == "\\x01")
assert("\x01".escape().len()  == 4)

assert("\x01\x02".escape()    == "\\x01\\x02")
assert("\x01\x02".escape().len() == 8)

assert("\x01\x02\x03\x04\x05".escape().len()         == 20)
assert("\x01\x02\x03\x04\x05\x06\x07\x08\x0e\x0f".escape().len() == 40)

// larger all-non-printable string to stress the fixed boundary
local s50 = ""
for (local i = 0; i < 5; i++)
    s50 += "\x01\x02\x03\x04\x05\x06\x07\x08\x0e\x0f"
assert(s50.len() == 50)
assert(s50.escape().len() == 200)

local s100 = s50 + s50
assert(s100.escape().len() == 400)

println("ok")

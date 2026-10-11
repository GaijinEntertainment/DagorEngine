#allow-switch-statement
#allow-delete-operator
#allow-compiler-internals
from "math" import sin, cos as cosine
from "json_render_mod.nut" import bump

// a line comment, and a block comment on the same line as code
const LIMIT = 10 /* ten */
let inexpr = $${ return LIMIT * 2 }
/* a block comment
   over two lines */
global const SHARED = 2
enum Color { RED, GREEN = 5 }

function [pure] scale(v: number, k = 2.5) {
  @@"Doc ""string"" with a backslash \"
  return v * k
}

function sum(first, ...) {
  return vargv.reduce(@(a, b) a + b, first)
}

let tbl = {
  [LIMIT] = -1,
  "json key": true
  text = "line\nbreak"
  escapes = "tab:\t bell:\x07 quote:\" slash:\\"
  utf8 = "2 bytes: \xc3\xa4, 3 bytes: \xe2\x82\xac"
  notUtf8 = "a byte that starts no sequence: \xc3\x28"
  pi = 3.14
  ...{extra = null}
  scale
}

function gen() {
  yield 1
  return 2
}

function [nodiscard] loops(n) {
  local i = n, total = 0
  while (i > 0) {
    total += i
    i--
  }
  do {
    total++
  } while (--i > -2)
  for (local j = 0; j < 2; j++, total++)
    total += j
  let [first, second] = [total, n]
  return first + second + ::type(n).len()
}

class Base {
  static kind = "base"
  value = 0
  constructor(v) { this.value = v }
}

class Child(Base) {
  function get() { return base.value ?? tbl?.text }
}

function spread({ text, pi = 0.0 }, [lo, hi]) {
  foreach (i, [a, b] in [[lo, hi]])
    println(i, a, b)
  return $"{text} {pi} {lo} {hi}"
}

function patterns([], {
  head,
  tail = 0
}) {
  foreach ({} in [tbl])
    println(head, tail)
  return head
}

function pick(x) {
  switch (x) {
    case 0:
    case 1: return [sin(x), cosine(x)]
    default: break
  }
  local i = 0
  for (; i < LIMIT; i++)
    if (i > 3 && !x)
      continue
  let seen = clone tbl
  delete seen["json key"]
  foreach (k, v in tbl)
    try {
      seen[k] = v
    } catch (e) {
      throw e
    }
  let { text, missing = const [1, 2] } = seen
  return x ? @() text : missing
}

function classify(v) {
  return (typeof v == "string") && ("text" in tbl) && !(v instanceof Base)
}

async function scaled(v) {
  return scale(v) * SHARED
}

async function later(v) {
  return await scaled(v)
}

let g = gen()
later(1)
println(pick(1).len(), Child(2).get(), Color.GREEN, scale(2), sum(1, 2, 3), spread(tbl, [1, 2]),
  loops(3), bump(1), resume g, classify("text"), patterns([], {head = "h"}))

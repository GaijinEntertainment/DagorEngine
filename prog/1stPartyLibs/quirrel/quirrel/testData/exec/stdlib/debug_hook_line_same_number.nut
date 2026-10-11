from "debug" import setdebughook

let events = []
local firstLine = null

function hook(kind, _src, line, funcname) {
  if (kind != 'l')
    return
  firstLine = firstLine ?? line
  events.append($"{funcname}+{line - firstLine}")
}

function sameLine() {
  function one() { return 1 }; let a = one(); let b = one()
  return a + b
}

function recursion() {
  function rec(n) { return n == 0 ? 0 : 1 + rec(n - 1) }; return rec(2)
}

function trace(fn) {
  events.clear()
  firstLine = null
  setdebughook(hook)
  newthread(fn).call()
  setdebughook(null)
  println(" ".join(events))
}

trace(sameLine)
trace(recursion)

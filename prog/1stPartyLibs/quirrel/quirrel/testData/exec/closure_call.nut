// BUG: sqbaselib.cpp closure_call() reads stack_get(v,-1) (the LAST ARGUMENT)
// instead of slot 1 (the callee) to decide whether to tail-call.
// So Class.call(...) with a closure as last arg wrongly takes the tail-call path.

function countArgs(...) {
  return vargv.len()
}

local sideEffects = []

function sideEffectClosure(name) {
  return function() {
    sideEffects.append(name)
    return name
  }
}

function sideEffectsSummary() {
  if (sideEffects.len() == 0)
    return "none"

  local res = ""
  foreach (i, name in sideEffects) {
    if (i > 0)
      res += ","
    res += name
  }
  return res
}

print("closure.call 0 args: " + countArgs.call(null) + "\n")
print("closure.call 1 closure arg: " + countArgs.call(null, sideEffectClosure("closure-1")) + "\n")
print("closure.call 2 args closure last: " + countArgs.call(null, 1, sideEffectClosure("closure-2-last")) + "\n")
print("closure.call 2 closure args: " + countArgs.call(null, sideEffectClosure("closure-2-a"), sideEffectClosure("closure-2-b")) + "\n")
print("closure.call 3 args closure last: " + countArgs.call(null, 1, "x", sideEffectClosure("closure-3-last")) + "\n")
print("closure.call 3 closure args: " + countArgs.call(null, sideEffectClosure("closure-3-a"), sideEffectClosure("closure-3-b"), sideEffectClosure("closure-3-c")) + "\n")

class C {
  argc = null
  lastType = null

  constructor(...) {
    this.argc = vargv.len()
    this.lastType = vargv.len() > 0 ? typeof(vargv[vargv.len() - 1]) : "none"
  }
}

local inst = C.call(null)
print("class.call 0 args: argc=" + inst.argc + " last=" + inst.lastType + "\n")

inst = C.call(null, sideEffectClosure("class-1"))
print("class.call 1 closure arg: argc=" + inst.argc + " last=" + inst.lastType + "\n")

inst = C.call(null, 1, sideEffectClosure("class-2-last"))
print("class.call 2 args closure last: argc=" + inst.argc + " last=" + inst.lastType + "\n")

inst = C.call(null, sideEffectClosure("class-2-a"), sideEffectClosure("class-2-b"))
print("class.call 2 closure args: argc=" + inst.argc + " last=" + inst.lastType + "\n")

inst = C.call(null, sideEffectClosure("class-2-first"), 1)
print("class.call 2 args int last: argc=" + inst.argc + " last=" + inst.lastType + "\n")

inst = C.call(null, 1, "x", sideEffectClosure("class-3-last"))
print("class.call 3 args closure last: argc=" + inst.argc + " last=" + inst.lastType + "\n")

inst = C.call(null, sideEffectClosure("class-3-a"), sideEffectClosure("class-3-b"), sideEffectClosure("class-3-c"))
print("class.call 3 closure args: argc=" + inst.argc + " last=" + inst.lastType + "\n")

function gen(...) {
  yield vargv.len()
}

local g = gen.call(null)
print("gen.call 0 args: " + typeof(g) + "\n")

g = gen.call(null, sideEffectClosure("gen-1"))
print("gen.call 1 closure arg: " + typeof(g) + "\n")

g = gen.call(null, 1, sideEffectClosure("gen-2-last"))
print("gen.call 2 args closure last: " + typeof(g) + "\n")

g = gen.call(null, sideEffectClosure("gen-2-a"), sideEffectClosure("gen-2-b"))
print("gen.call 2 closure args: " + typeof(g) + "\n")

g = gen.call(null, 1, "x", sideEffectClosure("gen-3-last"))
print("gen.call 3 args closure last: " + typeof(g) + "\n")

g = gen.call(null, sideEffectClosure("gen-3-a"), sideEffectClosure("gen-3-b"), sideEffectClosure("gen-3-c"))
print("gen.call 3 closure args: " + typeof(g) + "\n")

print("argument closure side effects: " + sideEffectsSummary() + "\n")

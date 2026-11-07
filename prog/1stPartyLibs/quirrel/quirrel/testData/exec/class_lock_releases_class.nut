// The _lock call must not keep the class in a stack slot above the top

local lockError = null
local baseDropped = false

function instanceLock() {
  local C = class {
    function _lock() {}
  }
  let wr = C.weakref()
  C()
  C = null
  return wr.ref() == null
}

function manualLock() {
  local C = class {
    function _lock() {}
  }
  let wr = C.weakref()
  assert(wr.ref() == C)
  C.lock()
  C = null
  return wr.ref() == null
}

function baseLock() {
  local B = class {
    function _lock() {}
  }
  let wr = B.weakref()
  local D = class(B) {}
  B = null
  D = null
  return wr.ref() == null
}

function failedLock() {
  local C = class {
    function _lock() {
      throw "lock failed"
    }
  }
  let wr = C.weakref()
  assert(wr.ref() == C)
  try {
    C.lock()
  } catch (e) {
    lockError = e
  }
  C = null
  return wr.ref() == null
}

// _lock drops the last reference to B
function droppedBase() {
  local B = class {
    x = 1
  }
  B._lock <- function() {
    B = null
    baseDropped = true
  }
  let D = class(B) {}
  return D().x
}

println($"Instance: {instanceLock()}")
println($"Manual lock: {manualLock()}")
println($"Base class: {baseLock()}")
println($"Failed lock: {failedLock()} ({lockError})")
println($"Dropped base: {droppedBase()} ({baseDropped})")

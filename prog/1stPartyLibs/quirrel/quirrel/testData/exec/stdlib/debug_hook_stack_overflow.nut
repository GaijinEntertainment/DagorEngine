from "debug" import setdebughook

local hookCalls = 0
setdebughook(function(_etype, _src, _line, _funcname) {
  hookCalls++
})

function deep(n) {
  return 1 + deep(n + 1)
}

function callOverflow() {
  try {
    deep(0)
    return "no error"
  }
  catch (e) {
    return e
  }
}

function wideGenerator() {
  local l0 = 0, l1 = 0, l2 = 0, l3 = 0, l4 = 0, l5 = 0, l6 = 0, l7 = 0, l8 = 0, l9 = 0
  local m0 = 0, m1 = 0, m2 = 0, m3 = 0, m4 = 0, m5 = 0, m6 = 0, m7 = 0, m8 = 0, m9 = 0
  local n0 = 0, n1 = 0, n2 = 0, n3 = 0, n4 = 0, n5 = 0, n6 = 0, n7 = 0, n8 = 0, n9 = 0
  while (true)
    yield l0 + l1 + l2 + l3 + l4 + l5 + l6 + l7 + l8 + l9 + m0 + m1 + m2 + m3 + m4 + m5 + m6 + m7 + m8 + m9
      + n0 + n1 + n2 + n3 + n4 + n5 + n6 + n7 + n8 + n9
}

function resumeOverflow() {
  let gen = wideGenerator()
  local failure = "no error"
  function dive() {
    try {
      resume gen
    }
    catch (e) {
      failure = e
      return 0
    }
    return 1 + dive()
  }
  dive()
  return failure
}

// the hook needs a new thread
let callRes = newthread(callOverflow).call()
let resumeRes = newthread(resumeOverflow).call()
setdebughook(null)

println($"call: {callRes}")
println($"resume: {resumeRes}")
println($"hook called: {hookCalls > 0}")

// Test: closure captures variable from nested block (indirect local)
// The inner lambda captures 'uniqueTimerKey' which is inside an 'if' block.
// This should prevent hoisting to fieldReadOnly's body level because the
// variable is not accessible at that block level - must stay inside the
// outer lambda where it can see the captured variable.

function clearTimer(_id) {
}

function fieldReadOnly(params = {}) {
  let {rawComponentName=null} = params

  if (true) {
    let uniqueTimerKey = rawComponentName

    return @() {
      onDetach = @() clearTimer(uniqueTimerKey)
    }
  }
}

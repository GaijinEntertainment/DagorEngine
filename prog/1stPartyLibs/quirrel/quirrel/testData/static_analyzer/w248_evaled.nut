if (__name__ == "__analysis__")
  return


let t = {}

function foo(_p) {}

let sr = t.value.findvalue(@(s) s.s == 10 && (s?.e ?? 0) > 0 )
let r = sr != null
let _timerObj = r ? foo({
    ts = sr.e  // No warning - r being true means sr != null
}) : null

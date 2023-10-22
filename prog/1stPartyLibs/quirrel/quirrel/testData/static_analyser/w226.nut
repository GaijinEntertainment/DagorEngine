//expect:w226
//-file:declared-never-used
//-file:plus-string

let o = {}

let function x(y) {
  if (y == 1)
    return "y == 1"
  else if (y == 2)
    return y == 2
  else
    return "unknown"
}

function foo(a, b) {
    return a.value ? b?.getEcsTemplates("ss") ?? [] : []
}

let B = {
    D = {}
    DC = {}
}

function bar(w) {
    if (w?.c != null)
      return {
        c = w?.c
      }
    if (w?.b == null)
      return B.D

    return B.findvalue(@(bt) bt.p != null && (w?.bt.indexof(bt.p) ?? -1) == 0)
    ?? B.DC
  }



function qux() {
    return o.isGG() ? "" : o.get1Txt() + " " + o.get2Txt()
}

function loc(a) { return a }

function getabc() {
    let p = o.gts()?.aaa
    return p != null ? p + loc("dskjfhjs") : ""
  }

const SVG_EXT = "xxx"

let sahj = @(id, p = "ddd") (id in o) ? p + id + SVG_EXT : ""

let _sumScore = o.reduce(@(res, v) res + (v?.score ?? 0))

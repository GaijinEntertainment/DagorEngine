// `type` and `classof` are leaf natives marked `fastcall`: results through the
// _OP_FASTCALL mini-frame must match the ordinary native-call path (`.call`).

from "types" import classof

class Point {}
function gen() {
  yield 1
}
let values = [null, true, 1, 1.5, "s", [], {}, @() null, print, Point, Point(), gen(), newthread(@() null)]

foreach (v in values) {
  let viaFastcall = $"{type(v)} {classof(v)}"
  let viaCall = $"{type.call(null, v)} {classof.call(null, v)}"
  println(viaFastcall == viaCall ? type(v) : $"MISMATCH {viaFastcall} {viaCall}")
}
println(classof(Point()) == Point)
println($"{type.getfuncinfos().fastcall} {classof.getfuncinfos().fastcall}")

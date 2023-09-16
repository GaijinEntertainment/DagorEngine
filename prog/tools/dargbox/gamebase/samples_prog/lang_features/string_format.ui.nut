from "%darg/ui_imports.nut" import *

let lineHeight = hdpx(30)
let function anyToString(a) {
  if (typeof a == "string")
    return $"\"{a}\""
  else
    return a.tostring()
}


let function tableToString(tbl) {
  let s = tbl.reduce(function(a,b, key) {
    a.append($"{key} = {anyToString(b)}")
    return a
  }, [])

  return ", ".join(s)
}


let function text(s){
  return {
    rendObj = ROBJ_TEXT
    halign = ALIGN_LEFT
    text = s
    size = [flex(), lineHeight]
  }
}

let function sampleText(a,b){
  return {
    rendObj = ROBJ_TEXT
    halign = ALIGN_LEFT
    text = " => ".concat(a,b)
    size = [flex(), lineHeight]
  }
}

let function sampleSubstTextTable(str, table) {
  return text($"\"{str}\".subst(\{ {tableToString(table)} \}) => \" {str.subst(table)} \"")
}

let function sampleSubstTextTableTable(str, table1, table2) {
  return text($"\"{str}\".subst(\{{tableToString(table1)}\}, \{{tableToString(table2)}\}) => \"{str.subst(table1, table2)}\"")
}


let function basicsRoot() {
  return {
    rendObj = ROBJ_SOLID
    size = [pw(100), ph(100)]
    color = Color(30,40,50)
    cursor = Cursor({rendObj = ROBJ_IMAGE size = [32, 32] image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32)))})
    padding = 50
    children = [
      {
        valign = ALIGN_TOP
        halign = ALIGN_LEFT
        flow = FLOW_VERTICAL
        size = [pw(100), ph(100)]
        gap = 0
        children = [
          sampleText("\"Score: {0}\".subst(4200)", "Score: {0}".subst(4200)),
          sampleText("\"x={0} y={1} z={2}\".subst(42, 45.5, -10.2)", "x={0} y={1} z={2}".subst(42, 45.5, -10.2)),//-forgot-subst
          sampleSubstTextTable("Score: {score}", {score=4200}),//-forgot-subst
          sampleSubstTextTable("x={x} y={y} z={z}", {x=42, y=45.53, z=-10.8}),//-forgot-subst
          sampleSubstTextTableTable("Type: {type}, Health: {hp}", {hp=100, damage=5}, {type="helicopter", isAir=true}),//-forgot-subst
          sampleText("\"Type: {type}, Pos: x={1} y={2}\".subst({type=\"helicopter\"}, 23, 2.0)", "Type: {type}, Pos: x={1} y={2}".subst({type="helicopter"}, 23, 2.0)),//-forgot-subst
          sampleSubstTextTable("Score: {score}", {}),//-forgot-subst
        ]
      }
    ]
  }
}

return basicsRoot

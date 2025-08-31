from "%darg/ui_imports.nut" import *

//this better be syntax error in squirrel!
function fatal_bad_return() {
  return
  { } //-unreachable-code
}

function fatal_bad_components() {
  function retintfromfunction(){return 1}
  function retfunctionfromfunction(){return @(){}}
  return{
    children = [
      retfunctionfromfunction
      retintfromfunction
    ]
  }
}
function badobservable(){
  return {
    watch = false
  }
}
function fatal_unknown_prop_type() {
  return {
    size = 1
    color = 2
    halign = "a"
    rendObj = "a"
    //etc
  }
}

function fatals() {
  let a = 10
  return {
    size = flex()
    valign = ALIGN_CENTER
    children = [
      { size = [flex,flex] } //fatal on size
      a
      { gap = [2,2] } //fatal on gap
      { gap = flex(1) } //fatal on gap
      { children = [
           {rendObj = ROBJ_BOX fillColor=Color(0,0,0) size = flex() borderColor=Color(255,0,0) borderWidth=0}
           {rendObj = ROBJ_TEXT text = "If you see this text - no known fatals on script or hotreload exist" color = Color(50,255,50) margin=10}
        ]
      }
      fatal_bad_return
      badobservable
      fatal_bad_components
      fatal_unknown_prop_type
    ]
  }
}

return fatals
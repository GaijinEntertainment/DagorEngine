// warning disable: -file:forbidden-function

let dagorDebug = require("dagor.debug")
let string = require("string.nut")
let math = require("math")
let tostring_r = string.tostring_r
let join = string.join //like join, but skip emptylines

let function print_(val, separator="\n"){
  print($"{val}{separator}")
}
const DEF_MAX_DEEPLEVEL = 4
let function Log(tostringfunc=null) {
  let function vlog(...){
    local out = ""
    if (vargv.len()==1)
      out = tostring_r(vargv[0],{splitlines=false, compact=true, maxdeeplevel=DEF_MAX_DEEPLEVEL, tostringfunc=tostringfunc})
    else
      out = join(vargv.map(@(val) tostring_r(val,{splitlines=false, compact=true, maxdeeplevel=DEF_MAX_DEEPLEVEL, tostringfunc=tostringfunc}))," ")
    dagorDebug.screenlog(out.slice(0,math.min(out.len(),200)))
  }

  let function log(...) {
    if (vargv.len()==1)
      print_(tostring_r(vargv[0],{compact=true, maxdeeplevel=DEF_MAX_DEEPLEVEL tostringfunc=tostringfunc}))
    else
      print_(" ".join(vargv.map(@(v) tostring_r(v,{compact=true, maxdeeplevel=DEF_MAX_DEEPLEVEL tostringfunc=tostringfunc}))))
  }

  let function dlog(...) {
    vlog.acall([this].extend(vargv))
    log.acall([this].extend(vargv))
  }

  let function dlogsplit(...) {
    log.acall([this].extend(vargv))
    if (vargv.len()==1)
      vargv=vargv[0]
    let out = tostring_r(vargv,{tostringfunc=tostringfunc})
    let s = string.split_by_chars(out,"\n")
    for (local i=0; i < math.min(80,s.len()); i++) {
      dagorDebug.screenlog(s[i])
    }
  }
  let function debugTableData(value, params={recursionLevel=3, addStr="", printFn=null, silentMode=true}){
    let addStr = params?.addStr ?? ""
    let silentMode = params?.silentMode ?? true
    let recursionLevel = params?.recursionLevel ?? 3
    let printFn = params?.printFn ?? print
    let prefix = silentMode ? "" : "DD: "

    let newline = $"\n{prefix}{addStr}"
    let maxdeeplevel = recursionLevel+1

    if (addStr=="" && !silentMode)
      printFn("DD: START")
    printFn(tostring_r(value,{compact=false, maxdeeplevel=maxdeeplevel, newline=newline, showArrIdx=false, tostringfunc=tostringfunc}))
  }

  let function console_print(...) {
    dagorDebug.console_print(" ".join(vargv.map(@(v) tostring_r(v, {maxdeeplevel=DEF_MAX_DEEPLEVEL, showArrIdx=false, tostringfunc=tostringfunc}))))
  }

  let function with_prefix(prefix) {
    return @(...) log("".concat(prefix, " ".join(vargv.map(@(val) tostring_r(val, {compact=true, maxdeeplevel=DEF_MAX_DEEPLEVEL tostringfunc=tostringfunc})))))
  }
  let function dlog_prefix(prefix) {
    return @(...) dlog.acall([null, prefix].extend(vargv))  //disable: -dlog-warn
  }

  local function wlog(watched, prefix = null, transform=null, logger = log) {
    local ftransform = transform ?? @(v) v
    local fprefix = prefix
    if (type(prefix) == "function") {
      ftransform = prefix
      fprefix = transform
    }
    if (type(transform) == "string")
      fprefix = transform
    if (type(watched) != "array")
      watched = [watched]
    fprefix = fprefix != "" ? fprefix : null
    if (fprefix != null)
      foreach (w in watched) {
        logger(fprefix, ftransform(w.value))
        w.subscribe(@(v) logger(fprefix, ftransform(v)))
      }
    else
      foreach (w in watched) {
        logger(ftransform(w.value))
        w.subscribe(@(v) logger(ftransform(v)))
      }
  }

  return {
    vlog
    v = vlog
    log
    dlog //disable: -dlog-warn
    d = dlog
    dlogsplit
    debugTableData
    console_print
    with_prefix
    dlog_prefix
    wlog
    //lowlevel dagor functions
    debug = dagorDebug.debug
    logerr = dagorDebug.logerr
    screenlog = dagorDebug.screenlog
  }
}

return Log

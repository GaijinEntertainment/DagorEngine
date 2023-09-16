from "daRg" import *

let {get_arg_value_by_name} = require("dagor.system") //for game_name stub below below
let {chunk} = require("std/underscore.nut")

const MOVE_ABS = "MOVE_ABS"
const MOVE_REL = "MOVE_REL"
const DRAWLINE_REL = "DRAWLINE_REL"
const DRAWLINE_ABS = "DRAWLINE_ABS"
const DRAWLINE_HOR_REL = "DRAWLINE_HOR_REL"
const DRAWLINE_HOR_ABS = "DRAWLINE_HOR_ABS"
const DRAWLINE_VER_REL = "DRAWLINE_VER_REL"
const DRAWLINE_VER_ABS = "DRAWLINE_VER_ABS"
const CLOSE = "CLOSE"
const BEZIER_CORNER_ABS = "BEZIER_CORNER_ABS"
const BEZIER_CORNER_REL = "BEZIER_CORNER_REL"
const BEZIER_SMOOTH_REL = "BEZIER_SMOOTH_REL"
const BEZIER_SMOOTH_ABS = "BEZIER_SMOOTH_ABS"
const BEZIER_QUADRATIC_REL = "BEZIER_QUADRATIC_REL"
const BEZIER_QUADRATIC_ABS = "BEZIER_QUADRATIC_ABS"
const BEZIER_T_REL = "BEZIER_T_REL"
const BEZIER_T_ABS = "BEZIER_T_ABS"
const ARC_ABS = "ARC_ABS"
const ARC_REL = "ARC_REL"

let COMMANDS_KEYS = {
  "M" : {command = MOVE_ABS, points_req=2, abs=true}
  "m" : {command = MOVE_REL, points_req=2, abs=false}
  "h" : {command = DRAWLINE_HOR_REL, points_req=1, abs=false}
  "H" : {command = DRAWLINE_HOR_ABS, points_req=1, abs=true}
  "v" : {command = DRAWLINE_VER_REL, points_req=1, abs=false}
  "V" : {command = DRAWLINE_VER_ABS, points_req=1, abs=true}
  "l" : {command = DRAWLINE_REL, points_req=2, abs=false}
  "L" : {command = DRAWLINE_ABS, points_req=2, abs=true}
  "C" : {command = BEZIER_CORNER_ABS, points_req=6, abs=true}
  "c" : {command = BEZIER_CORNER_REL, points_req=6, abs=false}
  "s" : {command = BEZIER_SMOOTH_REL, points_req=4, abs=false}
  "S" : {command = BEZIER_SMOOTH_ABS, points_req=4, abs=true}
//  "q" : {command = BEZIER_QUADRATIC_REL, points_req=4, abs=false}
//  "Q" : {command = BEZIER_QUADRATIC_ABS, points_req=4, abs=true}
//  "t" : {command = BEZIER_T_REL, points_req=2, abs=false}
//  "T" : {command = BEZIER_T_ABS, points_req=2, abs=true}
//  "A" : {command = ARC_ABS, points_req=2, draw=false}
//  "a" : {command = ARC_REL, points_req=6, draw=false}
  "z" : {command = CLOSE, points_req=0}
  "Z" : {command = CLOSE, points_req=0}
}
let COMMANDS = {}
foreach(i in COMMANDS_KEYS){
  COMMANDS[i.command] <- {command = i.command, points_req = i.points_req, abs=i?.abs}
}

let NUMBERS = [0,1,2,3,4,5,6,7,8,9, "."].map(@(v) v.tostring())
const MINUS = "-"

let function prP(str, start=0){
  for (local i=start; i<str.len(); i++){
    let chr = str[i].tochar()
    if ((i == start && chr == MINUS) || NUMBERS.contains(chr))
      continue
    return {substr = str.slice(start, i), end = i}
  }
  return {substr = str.slice(start), end=str.len()}
}
let function parsePoints(str){
  let res = []
  local start = 0
  while (start<str.len()){
    let {substr, end} = prP(str,start)
    if (end!=start) {
      res.append(substr)
      start = end
    }
    else
      start = start+1
  }
  return res.map(@(v) v.tofloat())
}

let function parsePath(pathstr){
  let res = []
  local last = -1
  local lastcommand = null
  let len =pathstr.len()
  for (local i=0;i<len; i++){
    let char = pathstr[i].tochar()
    if (i==(len-1)) {
      if (char in COMMANDS_KEYS) {
        if (COMMANDS_KEYS[char].command == CLOSE) {
          let substr = pathstr.slice(last+1, len)
          res.append({command = lastcommand, points=parsePoints(substr)})
          res.append({command = CLOSE})
          break
        }
        else
          assert(false, $"points required for command {char}")
      }
      let substr = pathstr.slice(last+1, len)
      res.append({command = lastcommand, points=parsePoints(substr)})
    }
    else if (char in COMMANDS_KEYS) {
      if (last>=0) {
        let substr = pathstr.slice(last+1, i)
        let points = parsePoints(substr)
        assert((points.len() % COMMANDS[lastcommand].points_req)==0,
          $"incorrect points for command {COMMANDS[lastcommand].command}: should be times of {COMMANDS[lastcommand].points_req}, got = {points.len()}, points = {", ".join(points)}, index={i}, str:{pathstr}")
        res.append({command = lastcommand, points})
        lastcommand = COMMANDS_KEYS[char].command
      }
      else if (last<0){
        lastcommand = COMMANDS_KEYS[char].command
      }
      last = i
    }
  }
  return res
}

let function transformP(point, offset, scale, curpos, command=null){
  if (command?.abs)
    curpos = [0,0]
  return [(point[0]+offset[0]+curpos[0])*scale[0], (point[1]+offset[1]+curpos[1])*scale[1]]
}

let DRAWLINE_CMDS = COMMANDS.filter(@(v) !v?.draw)

let function curPos(curCursorPos, point, command=null){
  let abs = command?.abs
  if (abs) {
    return [point[0], point[1]]
  }
  return [curCursorPos[0]+point[0], curCursorPos[1]+point[1]]
}

let function pathToCanvas(path, viewBox=null, fill=false){
  viewBox = viewBox ?? [0,0,100,100]
  let offset = [viewBox[0], viewBox[1]]
  let scale = [100.0/(viewBox[2]-offset[0]),100.0/(viewBox[3]-offset[1])]
  local curCursorPos = [0,0]
  local lastInitialPoint = null
  local lastCmd = null
  let res = []
  foreach(c in path){
    local {points=[]} = c
    let commandName = c.command
    let command = COMMANDS[commandName]
    points = chunk(points, command.points_req)
    if ([MOVE_ABS, MOVE_REL].contains(commandName)) {
      lastCmd = [fill ? VECTOR_POLY : VECTOR_LINE]
      res.append(lastCmd)
      lastInitialPoint = transformP(points[0], offset, scale, curCursorPos, command)
      foreach (p in points){
        curCursorPos = curPos(curCursorPos, p, command)
        lastCmd.extend(transformP(p, offset, scale, curCursorPos, command))
      }
    }
    else if (commandName == CLOSE) {
      lastCmd.extend(lastInitialPoint)
    }
    else if (commandName in DRAWLINE_CMDS){
      if (commandName.contains("_HOR"))
        points = points.map(@(v) [v[0], curCursorPos[1]])
      if (commandName.contains("_VER"))
        points = points.map(@(v) [curCursorPos[0], v[0]])
      if (commandName.contains("BEZIER_CORNER"))
        points = points.map(@(v) [v[4], v[5]])
      if (commandName.contains("BEZIER_SMOOTH"))
        points = points.map(@(v) [v[2], v[3]])
      foreach (p in points){
        let pt = transformP(p, offset, scale, curCursorPos, command)
        curCursorPos = curPos(curCursorPos, p, command)
        lastCmd.extend(pt)
      }
    }
    else
      throw({commandName, c})
  }
  return res
}

let log = @(...) " ".join(vargv)

if (__name__=="__main__") {
  let viewBox = get_arg_value_by_name("viewBox")
  let path = get_arg_value_by_name("path") ?? ""
  let fill = get_arg_value_by_name("fill") ?? "no"
  log("viewBox:", viewBox, "doFill:", fill, "path:", path)
  log(pathToCanvas(parsePath(path), viewBox, fill=="yes"))
}
else
  return pathToCanvas

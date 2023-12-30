//https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial/Gradients
//see example at https://briangrinstead.com/gradient/
from "%darg/ui_imports.nut" import *
from "base64" import encodeString

let math = require("math")

const BLEND_MODE_PREMULTIPLIED = "PREMULTIPLIED"
const BLEND_MODE_NONPREMULTIPLIED = "NONPREMULTIPLIED"
const BLEND_MODE_ADDITIVE = "ADDITIVE"

let blendModesPrefix = {
  [BLEND_MODE_PREMULTIPLIED] = "",
  [BLEND_MODE_NONPREMULTIPLIED] = "!",
  [BLEND_MODE_ADDITIVE] = "+"
}

function mkGradPointStyle(point, idx, points){
  let offset = point?.offset ?? (100 * idx/(points.len()-1))
//  assert(offset<=100 && offset >=0 && (["integer", "float"].contains(type(offset))))
  local color = point?.color
  if (color==null && type(point)=="array")
    color = point
  let opacity = color?.len()==4
    ? color[3]/255.0
    : point?.opacity
  local colorStr = ""
  if (color!=null){
    let [r,g,b] = color
    colorStr = $"stop-color:rgb({r}, {g}, {b});"
  }
  let opacityStr = (opacity!=null) ? $"stop-opacity:{opacity};" : ""
  assert(colorStr != "" || opacityStr != "", $"point in gradient should have color and/or opacity! got '{point}'")
  return $"<stop offset='{offset}%' style='{opacityStr}{colorStr}'/>"
}

enum GRADSPREAD {
  PAD = "pad"
  REFLECT = "reflect"
  REPEAT = "repeat"
}

function mkLinearGradSvgTxtImpl(points, width, height, x1=0, y1=0, x2=null, y2=0, spreadMethod=GRADSPREAD.PAD, transform=null){
  x2 = x2 ?? width
  assert(type(points)=="array", "points should be array of objects with color=[r,g,b,optional alpha] and optional offset. If offset is missing points are evenly distributed")
  assert(width>1 && height>1 && width+height > 15, "gradient should be created with some reasonable sizes")
  spreadMethod=spreadMethod ?? GRADSPREAD.PAD
  if (transform != null)
    transform = " ".join(transform.reduce(function(prev, v, k) {prev.append($"{k}({v}))"); return prev;}, []))
  let gradientTransformStr = transform!=null ? $"gradientTransform='{transform}'" : ""
  let header = $"<svg xmlns='http://www.w3.org/2000/svg' version='1.1'><defs>\n  <linearGradient spreadMethod='{spreadMethod}' id='gradient' {gradientTransformStr} x1='{x1}' y1='{y1}' x2='{x2}' y2='{y2}'>"
  let footer = $"  </linearGradient>\n</defs>\n<rect width='{width}' height='{height}' y='0' x='0' fill='url(#gradient)'/></svg>"
  assert(points.len()>1, "gradient can't be build with one point only")
  let body = "\n    ".join(points.map(mkGradPointStyle))
  return $"{header}\n    {body}\n{footer}"
}

let mkLinearGradientImg = kwarg(function(points, width, height, x1=0, y1=0, x2=null, y2=0, spreadMethod=GRADSPREAD.PAD, transform=null, blendMode=BLEND_MODE_PREMULTIPLIED, immediate=false) {
  let svg = mkLinearGradSvgTxtImpl(points, width, height, x1,y1,x2,y2, spreadMethod, transform)
  let text = encodeString(svg)
  let prefix = blendModesPrefix?[blendMode] ?? ""
  let pic = immediate ? PictureImmediate : Picture
  return pic($"{prefix}b64://{text}.svg:{width}:{height}?Ac")
})

function mkRadialGradSvgTxtImpl(points, width, height, cx=null, cy=null, r=null, fx=null, fy=null, spreadMethod=GRADSPREAD.PAD, transform=null){
  assert(type(points)=="array", "points should be array of objects with color=[r,g,b,optional alpha] and optional offset. If offset is missing points are evenly distributed")
  assert(width>1 && height>1 && width+height > 15, "gradient should be created with some reasonable sizes")
  spreadMethod=spreadMethod ?? GRADSPREAD.PAD
  if (transform != null)
    transform = " ".join(transform.reduce(function(prev, v, k) {prev.append($"{k}({v}))"); return prev;}, []))
  let focus = " ".join([
    fx != null ? $"fx='{fx}'" : "",
    fy != null ? $"fy='{fy}'" : ""
  ])
  r = r==null ? math.min(width, height) * 0.5 : r
  let center = " ".join([
    cx!=null ? $"cx='{cx}'" : "",
    cy!=null ? $"cy='{cy}'" : "",
  ])
  let gradientTransformStr = transform!=null ? $"gradientTransform='{transform}'" : ""
  let header = $"<svg xmlns='http://www.w3.org/2000/svg' version='1.1'><defs>\n  <radialGradient spreadMethod='{spreadMethod}' id='gradient' {gradientTransformStr} {center} r='{r}' {focus}>"
  let footer = $"  </radialGradient>\n</defs>\n<rect width='{width}' height='{height}' y='0' x='0' fill='url(#gradient)'/></svg>"
  assert(points.len()>1, "gradient can't be build with one point only")
  let body = "\n    ".join(points.map(mkGradPointStyle))
  return $"{header}\n    {body}\n{footer}"
}

/*Example:
let red = [255,0,0]
let green = [0, 255, 0]
let blue = [0, 0, 255]
{
  rendObj = ROBJ_IMAGE
  image = mkRadialGradientImg({points=[red, {color = green, offset=66}, blue], width=256, height=256, transform = {rotate = 90}}))
  size = flex()
}
*/
let mkRadialGradientImg = kwarg(function(points, width, height, cx=null, cy=null, r=null, fx=null, fy=null, spreadMethod=GRADSPREAD.PAD, transform=null, blendMode=BLEND_MODE_PREMULTIPLIED){
  let svg = mkRadialGradSvgTxtImpl(points, width, height, cx,cy,r,fx,fy, spreadMethod, transform)
  let text = encodeString(svg)
  let prefix = blendModesPrefix?[blendMode] ?? ""
  return Picture($"{prefix}b64://{text}.svg:{width}:{height}?Ac")
})

return {
  GRADSPREAD
  BLEND_MODE_PREMULTIPLIED
  BLEND_MODE_NONPREMULTIPLIED
  BLEND_MODE_ADDITIVE
  blendModesPrefix
  mkLinearGradientImg
  mkLinearGradSvgTxt = kwarg(mkLinearGradSvgTxtImpl)
  mkRadialGradientImg
  mkRadialGradSvgTxt = kwarg(mkRadialGradSvgTxtImpl)
}
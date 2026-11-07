//https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial/Gradients
//see example at https://briangrinstead.com/gradient/
from "%darg/ui_imports.nut" import *
from "%darg/helpers/bitmap.nut" import mkBitmapPicture, mkBitmapPictureLazy
from "base64" import encodeString
import "math" as math
from "math" import sqrt, pow, sin, PI, clamp, fabs, max
from "types" import Function, Array

const BLEND_MODE_PREMULTIPLIED = "PREMULTIPLIED"
const BLEND_MODE_NONPREMULTIPLIED = "NONPREMULTIPLIED"
const BLEND_MODE_ADDITIVE = "ADDITIVE"

const blendModesPrefix = {
  [BLEND_MODE_PREMULTIPLIED] = "",
  [BLEND_MODE_NONPREMULTIPLIED] = "!",
  [BLEND_MODE_ADDITIVE] = "+"
}

function mkGradPointStyle(point, idx, points): string {
  let offset = point?.offset ?? (100 * idx/(points.len()-1))
//  assert(offset<=100 && offset >=0 && (["integer", "float"].contains(type(offset))))
  local color = point?.color
  if (color==null && point instanceof Array)
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

function mkLinearGradSvgTxtImpl(points, width, height, x1=0, y1=0, x2=null, y2=0, spreadMethod=GRADSPREAD.PAD, transform=null): string {
  x2 = x2 ?? width
  assert(points instanceof Array, "points should be array of objects with color=[r,g,b,optional alpha] and optional offset. If offset is missing points are evenly distributed")
  assert(width>1 && height>1 && width+height > 7, "gradient should be created with some reasonable sizes")
  spreadMethod=spreadMethod ?? GRADSPREAD.PAD
  if (transform != null)
    transform = " ".join(transform.reduce(function(prev, v, k) {prev.append($"{k}({v})"); return prev;}, []))
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

function mkRadialGradSvgTxtImpl(points, width, height, cx=null, cy=null, r=null, fx=null, fy=null, spreadMethod=GRADSPREAD.PAD, transform=null): string {
  assert(points instanceof Array, "points should be array of objects with color=[r,g,b,optional alpha] and optional offset. If offset is missing points are evenly distributed")
  assert(width>1 && height>1 && width+height > 15, "gradient should be created with some reasonable sizes")
  spreadMethod=spreadMethod ?? GRADSPREAD.PAD
  if (transform != null)
    transform = " ".join(transform.reduce(function(prev, v, k) {prev.append($"{k}({v})"); return prev;}, []))
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

// bitmap gradients below: per-pixel pictures via mkBitmapPicture

const gradCircCornerSize = 20
const getDistance = @[pure] (x:number, y:number) sqrt(x * x + y * y)
const mkWhite = @[pure] (part:int):int part + (part << 8) + (part << 16) + (part << 24)
const EPS = 1e-4 // min control point distance; closer pairs make the TPS system ill-conditioned

let cbrt = @[pure] (v:number) pow(v, 1.0 / 3.0) //v < 0.0 ? -pow(-v, 1.0 / 3.0) : ... but we have only positive inputs

let srgbToLinear = @[pure] (c:number) c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4)

function [pure] linearToSrgb(c:number) {
  c = clamp(c, 0.0, 1.0)
  return c <= 0.0031308 ? c * 12.92 : 1.055 * pow(c, 1.0 / 2.4) - 0.055
}

let colorParts = @[pure] (color:int):table {
  r = (color >> 16) & 0xFF
  g = (color >> 8) & 0xFF
  b = color & 0xFF
  a = (color >> 24) & 0xFF
}

let partsToColor = @[pure] (c:table) Color(c.r+0.5, c.g+0.5, c.b+0.5, c.a+0.5) // will internally round down and cast to integer

function lerpColorParts(c1:table, c2:table, tmp:table, k:number) {
  if (k <= 0)
    return c1
  if (k >= 1)
    return c2
  let q = 1-k
  tmp.r = c1.r * q + c2.r * k
  tmp.g = c1.g * q + c2.g * k
  tmp.b = c1.b * q + c2.b * k
  tmp.a = c1.a * q + c2.a * k
  return tmp
}

// 0xAARRGGBB -> [L, a, b, alpha01]
function [pure] colorToOklab(color:int):array {
  let al = (color >> 24) & 0xFF
  let r = srgbToLinear(((color >> 16) & 0xFF) / 255.0)
  let g = srgbToLinear(((color >> 8) & 0xFF) / 255.0)
  let b = srgbToLinear((color & 0xFF) / 255.0)

  let l_ = cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b)
  let m_ = cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b)
  let s_ = cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b)

  return [
    0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
    1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
    0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_,
    al / 255.0
  ]
}

// [L, a, b, alpha01] -> 0xAARRGGBB
function [pure] oklabToColor(lab:array):int {
  let l_ = lab[0] + 0.3963377774 * lab[1] + 0.2158037573 * lab[2]
  let m_ = lab[0] - 0.1055613458 * lab[1] - 0.0638541728 * lab[2]
  let s_ = lab[0] - 0.0894841775 * lab[1] - 1.2914855480 * lab[2]

  let l = l_ * l_ * l_
  let m = m_ * m_ * m_
  let s = s_ * s_ * s_

  let r = linearToSrgb( 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s)
  let g = linearToSrgb(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s)
  let b = linearToSrgb(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s)

  let ai = (clamp(lab[3], 0.0, 1.0) * 255.0 + 0.5).tointeger()
  return (ai << 24)
    | ((r * 255.0 + 0.5).tointeger() << 16)
    | ((g * 255.0 + 0.5).tointeger() << 8)
    | (b * 255.0 + 0.5).tointeger()
}

// Gauss-Jordan with partial pivoting. A: m x m, B: m x k, both modified in place.
// returns m x k solution
function solveLinear(A:array, B:array):array {
  let m = A.len()
  let k = B[0].len()

  for (local c = 0; c < m; c++) {
    local piv = c
    for (local r = c + 1; r < m; r++)
      if (fabs(A[r][c]) > fabs(A[piv][c]))
        piv = r

    if (piv != c) {
      let ta = A[c]
      A[c] = A[piv]
      A[piv] = ta
      let tb = B[c]
      B[c] = B[piv]
      B[piv] = tb
    }

    let d = A[c][c]
    assert(fabs(d) > 1e-12, "singular system (duplicate points?)")

    for (local r = 0; r < m; r++) {
      if (r == c)
        continue
      let f = A[r][c] / d
      for (local j = c; j < m; j++)
        A[r][j] -= f * A[c][j]
      for (local j = 0; j < k; j++)
        B[r][j] -= f * B[c][j]
    }
  }

  let X = []
  for (local r = 0; r < m; r++) {
    let row = []
    for (local j = 0; j < k; j++)
      row.append(B[r][j] / A[r][r])
    X.append(row)
  }
  return X
}

function [pure] makeHermitEasingFunc(...):function {
  let n = vargv.len()
  assert(n >= 2, "need at least 2 points")

  let xs = []
  let ts = []

  local lastx = 0.0
  foreach (i, p in vargv) {
    if (i==0) {
      xs.append(0.0)
      ts.append(p instanceof Array ? p[1].tofloat() : p.tofloat())
    }
    else if (i == n-1) {
      xs.append(1.0)
      ts.append(p instanceof Array ? p[1].tofloat() : p.tofloat())
    }
    else {
      let x = p[0].tofloat()
      assert(x > lastx && x<1.0)
      xs.append(x)
      lastx = x
      ts.append(p[1].tofloat())
    }
  }

  // secant slopes of each segment
  let ds = array(n - 1, 0.0)
  for (local i = 0; i < n - 1; i += 1)
    ds[i] = (ts[i + 1] - ts[i]) / (xs[i + 1] - xs[i])

  // initial tangents
  let ms = array(n, 0.0)
  ms[0] = ds[0]
  ms[n - 1] = ds[n - 2]
  for (local i = 1; i < n - 1; i += 1) {
    // zero tangent at local extrema / flat neighbors, average otherwise
    if (ds[i - 1] * ds[i] <= 0.0)
      ms[i] = 0.0
    else
      ms[i] = (ds[i - 1] + ds[i]) * 0.5
  }

  // Fritsch-Carlson limiting: prevents overshoot within each segment
  for (local i = 0; i < n - 1; i += 1) {
    if (ds[i] == 0.0) {
      ms[i] = 0.0
      ms[i + 1] = 0.0
    }
    else {
      let a = ms[i] / ds[i]
      let b = ms[i + 1] / ds[i]
      let s = a * a + b * b
      if (s > 9.0) {
        let tau = 3.0 / sqrt(s)
        ms[i] = tau * a * ds[i]
        ms[i + 1] = tau * b * ds[i]
      }
    }
  }

  // cubic Hermite evaluation
  return function [pure] (x:number):number {
    if (x <= 0.0)
      return ts[0]
    if (x >= 1.0)
      return ts[n - 1]

    local i = 0
    while (i < n - 2 && x > xs[i + 1])
      i += 1

    let h = xs[i + 1] - xs[i]
    let u = (x - xs[i]) / h
    let u2 = u * u
    let u3 = u2 * u

    return clamp((2.0 * u3 - 3.0 * u2 + 1.0) * ts[i]
      + (u3 - 2.0 * u2 + u) * h * ms[i]
      + (-2.0 * u3 + 3.0 * u2) * ts[i + 1]
      + (u3 - u2) * h * ms[i + 1], 0.0, 1.0)
  }
}

const gradientEasings = {
  function [pure] easeInOutCubic(x:number):number {
    return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2
  }
  function [pure] easeOutSine(x:number):number{
    return sin((x * PI) / 2)
  }
  function [pure] easeOutQuad(x:number):number {
    return 1 - (1 - x) * (1 - x)
  }

  function [pure] easeInQuad(x:number):number {
    return x * x
  }
  function [pure] easeInExpo(x:number):number{
    return x == 0 ? 0 : pow(2, 10 * x - 10)
  }
  linear = @[pure] (x:number):number x
}

let resolveEasing = @[pure](easing:function|string) easing instanceof Function ? easing : gradientEasings[easing]

let mkSmoothBWGradientY = function ({height = 12, isAlphaPremultiplied = true, easing = gradientEasings.easeOutQuad}) {
  let w = 4
  let h = height.tointeger()
  let ease = resolveEasing(easing)
  return mkBitmapPicture(w, h,
    function(_, bmp) {
      for (local y = 0; y < h; y++) {
        let t = ease(y.tofloat()/h)

        let color = Color(t*255, t*255, t*255, t*255)
        for (local x = 0; x < w; x++)
          bmp.setPixel(x, y, color)
      }
    }, isAlphaPremultiplied ? "" : "!")
}

let mkSmoothBWGradientX = function ({width = 12, isAlphaPremultiplied = true, easing = gradientEasings.easeOutQuad}) {
  let h = 4
  let w = width.tointeger()
  let ease = resolveEasing(easing)
  return mkBitmapPicture(w, h,
    function(_, bmp) {
      for (local x = 0; x < w; x++) {
        let t = ease(x.tofloat()/w)
        let color = Color(t*255, t*255, t*255, t*255)
        for (local y = 0; y < h; y++)
          bmp.setPixel(x, y, color)
      }
    }, isAlphaPremultiplied ? "" : "!")
}

let mkColoredGradientX = @({colorLeft, colorRight, width = 12, isAlphaPremultiplied = true})
  mkBitmapPicture(width, 4,
    function(params, bmp) {
      let { w, h } = params
      let c1 = colorParts(colorLeft)
      let c2 = colorParts(colorRight)
      let tmp = {r=0, g=0, b=0, a=0}
      for (local x = 0; x < w; x++) {
        let color = partsToColor(lerpColorParts(c1, c2, tmp, x.tofloat() / (w - 1)))
        for (local y = 0; y < h; y++)
          bmp.setPixel(x, y, color)
      }
    }, isAlphaPremultiplied ? "" : "!")


// call to get the cached picture; lazy to avoid pinning a texture at module load
let gradRadial = mkBitmapPictureLazy(gradCircCornerSize * 2, gradCircCornerSize * 2,
  function(_, bmp) {
    for (local y = 0; y < gradCircCornerSize * 2; y++)
      for (local x = 0; x < gradCircCornerSize * 2; x++) {
        let distance = getDistance(x - gradCircCornerSize, y - gradCircCornerSize)
        bmp.setPixel(x, y, mkWhite((0xFF * max(0.0, 1.0 - ((distance + 1) / gradCircCornerSize))).tointeger()))
      }
  })

// thin-plate spline radial basis r^2*ln(r), via r^2 to avoid sqrt
let tpsKernel = @[pure] (r2:number):float r2 <= 0.0 ? 0.0 : 0.5 * r2 * math.log(r2)

// corners are in bitmap space: tl=(0,0), tr=(1,0), bl=(0,1), br=(1,1), y grows down
// extra points: [x, y, value], x/y in [0..1]
// value: float 0..1 for B&W, int 0xAARRGGBB for colored (lerped in Oklab)
function [pure] mkPlanarGradientFunc(colored:bool, tl, tr, bl, br, extra:array):function {
  let xs = [0.0, 1.0, 0.0, 1.0]
  let ys = [0.0, 0.0, 1.0, 1.0]
  let raw = [tl, tr, bl, br]

  foreach (p in extra) {
    assert(p instanceof Array && p.len() == 3, "extra point must be [x, y, value]")
    let x = p[0].tofloat()
    let y = p[1].tofloat()
    assert(x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0, "point out of [0..1] range")
    foreach (i, ox in xs)
      assert(fabs(x - ox) > EPS || fabs(y - ys[i]) > EPS, "duplicate point")
    xs.append(x)
    ys.append(y)
    raw.append(p[2])
  }

  let vals = []
  foreach (v in raw) {
    if (colored) {
      assert(typeof v == "integer", "colored gradient expects int 0xAARRGGBB values")
      vals.append(colorToOklab(v))
    }
    else {
      assert((typeof v == "float" || typeof v == "integer") && v >= 0 && v <= 1,
        "B&W gradient expects float 0..1 values (set colored=true for colors)")
      vals.append([v.tofloat()])
    }
  }

  let n = xs.len()
  let channels = vals[0].len()
  let m = n + 3 // n weights + affine part (1, x, y)

  // [K P; P^T 0] [w; a] = [v; 0]
  let A = []
  let B = []
  for (local i = 0; i < m; i++) {
    A.append(array(m, 0.0))
    B.append(array(channels, 0.0))
  }

  for (local i = 0; i < n; i++) {
    for (local j = 0; j < n; j++) {
      let dx = xs[i] - xs[j]
      let dy = ys[i] - ys[j]
      A[i][j] = tpsKernel(dx * dx + dy * dy)
    }
    A[i][n] = 1.0
    A[n][i] = 1.0
    A[i][n + 1] = xs[i]
    A[n + 1][i] = xs[i]
    A[i][n + 2] = ys[i]
    A[n + 2][i] = ys[i]
    for (local c = 0; c < channels; c++)
      B[i][c] = vals[i][c]
  }

  let W = solveLinear(A, B)

  return function [pure] (x:number, y:number):number {
    x = clamp(x.tofloat(), 0.0, 1.0)
    y = clamp(y.tofloat(), 0.0, 1.0)

    let out = array(channels, 0.0)
    for (local i = 0; i < n; i++) {
      let dx = x - xs[i]
      let dy = y - ys[i]
      let k = tpsKernel(dx * dx + dy * dy)
      for (local c = 0; c < channels; c++)
        out[c] += W[i][c] * k
    }
    for (local c = 0; c < channels; c++)
      out[c] += W[n][c] + W[n + 1][c] * x + W[n + 2][c] * y

    return colored ? oklabToColor(out) : clamp(out[0], 0.0, 1.0)
  }
}

// returns function(x, y) -> float 0..1 (B&W) or int 0xAARRGGBB (colored)
// vargv: extra [x, y, value] points the surface must pass through
let makePlanarGradient = function [pure] ({colored = false, tl, tr, bl, br}, ...):function {
  return mkPlanarGradientFunc(colored, tl, tr, bl, br, vargv)
}

// vargv: extra [x, y, value] points, same as makePlanarGradient
let make2DGradient = function ({tl, tr, bl, br, width = 16, height = 16,
    isAlphaPremultiplied = true, colored = false}, ...) {
  let w = width.tointeger()
  let h = height.tointeger()
  assert(w >= 2 && h >= 2, "gradient bitmap must be at least 2x2")

  let grad = mkPlanarGradientFunc(colored, tl, tr, bl, br, vargv)
  let pixel = colored
    ? grad
    : function [pure] (px:number, py:number):int {
        let t = grad(px, py)
        return Color(t * 255, t * 255, t * 255, t * 255)
      }

  return mkBitmapPicture(w, h,
    function(_, bmp) {
      let maxX = (w - 1).tofloat()
      let maxY = (h - 1).tofloat()
      for (local y = 0; y < h; y++)
        for (local x = 0; x < w; x++)
          bmp.setPixel(x, y, pixel(x / maxX, y / maxY))
    }, isAlphaPremultiplied ? "" : "!")
}

return freeze({
  GRADSPREAD
  BLEND_MODE_PREMULTIPLIED
  BLEND_MODE_NONPREMULTIPLIED
  BLEND_MODE_ADDITIVE
  blendModesPrefix
  mkLinearGradientImg
  mkLinearGradSvgTxt = kwarg(mkLinearGradSvgTxtImpl)
  mkRadialGradientImg
  mkRadialGradSvgTxt = kwarg(mkRadialGradSvgTxtImpl)
  makePlanarGradient
  make2DGradient
  mkSmoothBWGradientX
  mkSmoothBWGradientY
  mkColoredGradientX
  gradRadial
  gradientEasings
  makeHermitEasingFunc
})

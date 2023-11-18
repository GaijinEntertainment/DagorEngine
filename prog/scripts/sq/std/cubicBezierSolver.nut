from "math" import fabs
/**
 * Solver for cubic bezier curve with implicit control points at (0,0) and (1.0, 1.0).
 * Adjust at http://cubic-bezier.com/
 */

function sampleCurveX(t, ax, bx, cx) {
  return ((ax * t + bx) * t + cx) * t
}

function sampleCurveY(t, ay, by, cy) {
  return ((ay * t + by) * t + cy) * t
}

function sampleCurveDerivativeX(t, ax, bx, cx) {
  return (3.0 * ax * t + 2.0 * bx) * t + cx
}

function solveCurveX(x, epsilon, ax, bx, cx) {
  local t0
  local t1
  local t2 = x
  local x2
  local d2
  local i

  // First try a few iterations of Newton's method -- normally very fast.
  for (i = 0; i < 8; ++i) {
    x2 = sampleCurveX(t2, ax, bx, cx) - x;
    if (fabs(x2) < epsilon)
      return t2;
    d2 = sampleCurveDerivativeX(t2, ax, bx, cx)
    if (fabs(d2) < epsilon)
      break;
    t2 = t2 - x2 / d2;
  }

  // No solution found - use bi-section.
  t0 = 0.0
  t1 = 1.0
  t2 = x

  if (t2 < t0)
    return t0
  if (t2 > t1)
    return t1

  while (t0 < t1) {
    x2 = sampleCurveX(t2, ax, bx, cx);
    if (fabs(x2 - x) < epsilon)
      return t2
    if (x > x2)
      t0 = t2
    else
      t1 = t2
    t2 = (t1 - t0) * 0.5 + t0
  }

  // Give up.
  return t2;
}


const epsilon = 0.000001 // Precision

function solveCubicBezier(t, p1x, p1y, p2x, p2y) {
  // Pre-calculate the polynomial coefficients.
  // First and last control points are implied to be (0,0) and (1.0, 1.0).
  let cx = 3.0 * p1x
  let bx = 3.0 * (p2x - p1x) - cx
  let ax = 1.0 - cx - bx

  let cy = 3.0 * p1y
  let by = 3.0 * (p2y - p1y) - cy
  let ay = 1.0 - cy - by

  return sampleCurveY(solveCurveX(t, epsilon, ax, bx, cx), ay, by, cy)
}


return {
  solveCubicBezier
}

// float-heavy inner loop
from "profile.nut" import vmBench

const W = 320
const H = 160
const MAXIT = 60

function mandel() {
  local inside = 0
  for (local py = 0; py < H; py++) {
    let ci = py * 2.0 / H - 1.0
    for (local px = 0; px < W; px++) {
      let cr = px * 3.0 / W - 2.0
      local zr = 0.0
      local zi = 0.0
      local it = 0
      while (it < MAXIT && zr * zr + zi * zi < 4.0) {
        let nzr = zr * zr - zi * zi + cr
        zi = 2.0 * zr * zi + ci
        zr = nzr
        it++
      }
      if (it == MAXIT)
        inside++
    }
  }
  return inside
}

vmBench("mandel", mandel)

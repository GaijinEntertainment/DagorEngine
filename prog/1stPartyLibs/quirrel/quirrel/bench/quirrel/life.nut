// 2d array access, integer logic (Game of Life)
from "profile.nut" import vmBench

const W = 48
const H = 48
const GENS = 300

function mkGrid() {
  let g = array(H)
  for (local y = 0; y < H; y++) {
    let row = array(W)
    for (local x = 0; x < W; x++)
      row[x] = (x * 7 + y * 13) % 5 == 0 ? 1 : 0
    g[y] = row
  }
  return g
}

function step(src, dst) {
  for (local y = 0; y < H; y++) {
    let up = src[(y + H - 1) % H]
    let mid = src[y]
    let down = src[(y + 1) % H]
    let out = dst[y]
    for (local x = 0; x < W; x++) {
      let xm = (x + W - 1) % W
      let xp = (x + 1) % W
      let n = up[xm] + up[x] + up[xp]
            + mid[xm]         + mid[xp]
            + down[xm] + down[x] + down[xp]
      out[x] = (n == 3 || (n == 2 && mid[x] == 1)) ? 1 : 0
    }
  }
}

function life() {
  local a = mkGrid()
  local b = mkGrid()
  for (local g = 0; g < GENS; g++) {
    step(a, b)
    let tmp = a
    a = b
    b = tmp
  }
  local alive = 0
  foreach (row in a)
    foreach (cell in row)
      alive += cell
  return alive
}

vmBench("life", life)

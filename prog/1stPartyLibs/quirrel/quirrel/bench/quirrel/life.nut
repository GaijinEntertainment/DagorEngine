// 2d array access, integer logic (Game of Life)
let { clock } = require("datetime")

const W = 48
const H = 48
const GENS = 300

function mkGrid() {
  let g = []
  for (local y = 0; y < H; y++) {
    let row = []
    for (local x = 0; x < W; x++)
      row.append((x * 7 + y * 13) % 5 == 0 ? 1 : 0)
    g.append(row)
  }
  return g
}

function step(src, dst) {
  for (local y = 0; y < H; y++) {
    let ym = (y + H - 1) % H
    let yp = (y + 1) % H
    for (local x = 0; x < W; x++) {
      let xm = (x + W - 1) % W
      let xp = (x + 1) % W
      local n = src[ym][xm] + src[ym][x] + src[ym][xp]
              + src[y][xm]               + src[y][xp]
              + src[yp][xm] + src[yp][x] + src[yp][xp]
      dst[y][x] = (n == 3 || (n == 2 && src[y][x] == 1)) ? 1 : 0
    }
  }
}

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local a = mkGrid()
  local b = mkGrid()
  for (local g = 0; g < GENS; g++) {
    step(a, b)
    local tmp = a; a = b; b = tmp
  }
  local alive = 0
  for (local y = 0; y < H; y++)
    for (local x = 0; x < W; x++)
      alive += a[y][x]
  println("BENCH life", ((clock() - t0) * 1000), $"ms (r={alive})")
}

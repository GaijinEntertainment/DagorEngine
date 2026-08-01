// float-heavy inner loop
let { clock } = require("datetime")

const W = 320
const H = 160
const MAXIT = 60

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local inside = 0
  for (local py = 0; py < H; py++) {
    local ci = py * 2.0 / H - 1.0
    for (local px = 0; px < W; px++) {
      local cr = px * 3.0 / W - 2.0
      local zr = 0.0
      local zi = 0.0
      local it = 0
      while (it < MAXIT && zr * zr + zi * zi < 4.0) {
        local nzr = zr * zr - zi * zi + cr
        zi = 2.0 * zr * zi + ci
        zr = nzr
        it++
      }
      if (it == MAXIT)
        inside++
    }
  }
  println("BENCH mandel", ((clock() - t0) * 1000), $"ms (r={inside})")
}

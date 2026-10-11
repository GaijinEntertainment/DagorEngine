// The Computer Language Benchmarks Game spectral-norm
dofile("profile.nut")

const N = 500

local function A(i, j) {
  local ij = i + j
  return 1.0 / (ij * (ij + 1) / 2 + i + 1)
}

local function Av(x, y) {
  for (local i = 0; i < N; i++) {
    local a = 0.0
    for (local j = 0; j < N; j++)
      a += A(i, j) * x[j]
    y[i] = a
  }
}

local function Atv(x, y) {
  for (local i = 0; i < N; i++) {
    local a = 0.0
    for (local j = 0; j < N; j++)
      a += A(j, i) * x[j]
    y[i] = a
  }
}

local function AtAv(x, y, t) {
  Av(x, t)
  Atv(t, y)
}

local function spectralNorm() {
  local u = array(N, 1.0)
  local v = array(N, 0.0)
  local t = array(N, 0.0)
  for (local i = 0; i < 10; i++) {
    AtAv(u, v, t)
    AtAv(v, u, t)
  }
  local vBv = 0.0
  local vv = 0.0
  for (local i = 0; i < N; i++) {
    vBv += u[i] * v[i]
    vv += v[i] * v[i]
  }
  return format("%.9f", sqrt(vBv / vv))
}

benchmark("spectral norm", 10, spectralNorm)

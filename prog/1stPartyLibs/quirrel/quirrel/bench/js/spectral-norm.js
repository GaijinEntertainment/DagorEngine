// The Computer Language Benchmarks Game spectral-norm
import { benchmark } from "./profile.js";

const { sqrt } = Math;
const N = 500;

function A(i, j) {
  const ij = i + j;
  return 1.0 / (ij * (ij + 1) / 2 + i + 1);
}

function Av(x, y) {
  for (let i = 0; i < N; i++) {
    let a = 0.0;
    for (let j = 0; j < N; j++)
      a += A(i, j) * x[j];
    y[i] = a;
  }
}

function Atv(x, y) {
  for (let i = 0; i < N; i++) {
    let a = 0.0;
    for (let j = 0; j < N; j++)
      a += A(j, i) * x[j];
    y[i] = a;
  }
}

function AtAv(x, y, t) {
  Av(x, t);
  Atv(t, y);
}

function spectralNorm() {
  const u = new Array(N).fill(1.0);
  const v = new Array(N).fill(0.0);
  const t = new Array(N).fill(0.0);
  for (let i = 0; i < 10; i++) {
    AtAv(u, v, t);
    AtAv(v, u, t);
  }
  let vBv = 0.0;
  let vv = 0.0;
  for (let i = 0; i < N; i++) {
    vBv += u[i] * v[i];
    vv += v[i] * v[i];
  }
  return sqrt(vBv / vv).toFixed(9);
}

benchmark("spectral norm", 10, spectralNorm);

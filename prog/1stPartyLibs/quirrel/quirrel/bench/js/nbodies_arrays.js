// The Computer Language Benchmarks Game n-body, bodies as arrays
// https://benchmarksgame-team.pages.debian.net/benchmarksgame/
import { benchmark } from "./profile.js";

const { sqrt, PI } = Math;
const SOLAR_MASS = 4.0 * PI * PI;
const DAYS_PER_YEAR = 365.24;
const DT = 0.01;
const STEPS = 500000;

// body: [x, y, z, vx, vy, vz, mass]
const solarSystem = [
  [ // Sun
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
    SOLAR_MASS,
  ],
  [ // Jupiter
    4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * DAYS_PER_YEAR,
    7.69901118419740425e-03 * DAYS_PER_YEAR,
    -6.90460016972063023e-05 * DAYS_PER_YEAR,
    9.54791938424326609e-04 * SOLAR_MASS,
  ],
  [ // Saturn
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * DAYS_PER_YEAR,
    4.99852801234917238e-03 * DAYS_PER_YEAR,
    2.30417297573763929e-05 * DAYS_PER_YEAR,
    2.85885980666130812e-04 * SOLAR_MASS,
  ],
  [ // Uranus
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * DAYS_PER_YEAR,
    2.37847173959480950e-03 * DAYS_PER_YEAR,
    -2.96589568540237556e-05 * DAYS_PER_YEAR,
    4.36624404335156298e-05 * SOLAR_MASS,
  ],
  [ // Neptune
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * DAYS_PER_YEAR,
    1.62824170038242295e-03 * DAYS_PER_YEAR,
    -9.51592254519715870e-05 * DAYS_PER_YEAR,
    5.15138902046611451e-05 * SOLAR_MASS,
  ],
];

function offsetMomentum(bodies) {
  let px = 0.0, py = 0.0, pz = 0.0;
  for (const b of bodies) {
    px += b[3] * b[6];
    py += b[4] * b[6];
    pz += b[5] * b[6];
  }
  const sun = bodies[0];
  sun[3] = -px / SOLAR_MASS;
  sun[4] = -py / SOLAR_MASS;
  sun[5] = -pz / SOLAR_MASS;
}

function energy(bodies) {
  const n = bodies.length;
  let e = 0.0;
  for (let i = 0; i < n; i++) {
    const bi = bodies[i];
    e += 0.5 * bi[6] * (bi[3] * bi[3] + bi[4] * bi[4] + bi[5] * bi[5]);
    for (let j = i + 1; j < n; j++) {
      const bj = bodies[j];
      const dx = bi[0] - bj[0], dy = bi[1] - bj[1], dz = bi[2] - bj[2];
      e -= bi[6] * bj[6] / sqrt(dx * dx + dy * dy + dz * dz);
    }
  }
  return e;
}

// velocities and masses carry dt
function scale(bodies, s) {
  for (const b of bodies) {
    b[6] = b[6] * s * s;
    b[3] *= s;
    b[4] *= s;
    b[5] *= s;
  }
}

function advance(bodies) {
  const n = bodies.length;
  for (let i = 0; i < n; i++) {
    const bi = bodies[i];
    const bix = bi[0], biy = bi[1], biz = bi[2], bimass = bi[6];
    let bivx = bi[3], bivy = bi[4], bivz = bi[5];
    for (let j = i + 1; j < n; j++) {
      const bj = bodies[j];
      const dx = bix - bj[0], dy = biy - bj[1], dz = biz - bj[2];
      const d2 = dx * dx + dy * dy + dz * dz;
      const mag = 1.0 / (d2 * sqrt(d2));
      const bim = bimass * mag, bjm = bj[6] * mag;
      bivx -= dx * bjm;
      bivy -= dy * bjm;
      bivz -= dz * bjm;
      bj[3] += dx * bim;
      bj[4] += dy * bim;
      bj[5] += dz * bim;
    }
    bi[3] = bivx;
    bi[4] = bivy;
    bi[5] = bivz;
    bi[0] += bivx;
    bi[1] += bivy;
    bi[2] += bivz;
  }
}

function simulate() {
  scale(solarSystem, DT);
  for (let i = 0; i < STEPS; i++)
    advance(solarSystem);
  scale(solarSystem, 1.0 / DT);
  return energy(solarSystem).toFixed(9);
}

offsetMomentum(solarSystem);
benchmark("n-bodies-arrays", 10, simulate);

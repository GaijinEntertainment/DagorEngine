import { benchmark } from "./profile.js";

const COUNT = 50000;
const PASSES = 100;

const swarm = Array.from({ length: COUNT }, (_, i) => ({
  x: i + 0.1, y: i + 0.2, z: i + 0.3, vx: 1.1, vy: 2.1, vz: 3.1,
}));

function move(particles) {
  const n = particles.length;
  for (let pass = 0; pass < PASSES; pass++) {
    for (let i = 0; i < n; i++) {
      const p = particles[i];
      p.x += p.vx;
      p.y += p.vy;
      p.z += p.vz;
    }
  }
  return particles[COUNT - 1].z.toFixed(6);
}

benchmark("particles kinematics", 20, () => move(swarm));

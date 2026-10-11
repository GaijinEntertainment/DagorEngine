export function benchmark(name, runs, workload, repeats = 1) {
  let best = Infinity;
  let result;
  for (let i = 0; i < runs; i++) {
    const start = performance.now();
    result = workload();
    const elapsed = performance.now() - start;
    if (elapsed < best)
      best = elapsed;
  }
  const tail = result == null ? "" : `, ${result}`;
  console.log(`"${name}", ${(best / 1000 / repeats).toFixed(8)}, ${runs * repeats}${tail}`);
}

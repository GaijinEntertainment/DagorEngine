// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_functionRef.h>

namespace daparallel
{
// The caller and the pool workers that a kernel loop runs on; threads 0 takes the whole pool. 1 with no pool, or in a
// build with daParallel = no
uint32_t worker_count(int threads);

// cb(begin, end, worker) over [0, n) in chunks of quant > 0, on the caller and workers - 1 pool jobs;
// one worker runs it on the caller alone, with no call into the pool
void parallel_run(uint32_t workers, uint32_t n, uint32_t quant, dag::FunctionRef<void(uint32_t, uint32_t, uint32_t)> cb);

// Most threads inside parallel_run bodies at one time since the last reset
// Loops count threads only after the first call
uint32_t parallel_peak_threads(bool reset);

// One chunk of a parallel loop wrote [from, to) of its worker's output
struct Chunk
{
  uint32_t worker, from, to;
};

// body(begin, end, worker) over [0, n) in chunks of quant > 0; size(worker) is the length of that worker's output,
// so the chunks list the outputs in loop order whatever the thread count
template <typename Body, typename Size>
void parallel_chunks(uint32_t workers, uint32_t n, uint32_t quant, dag::Vector<Chunk> &chunks, const Body &body, const Size &size)
{
  if (workers == 1)
    quant = n ? n : 1;
  chunks.resize((n + quant - 1) / quant);
  auto run = [&](uint32_t begin, uint32_t end, uint32_t worker) {
    const uint32_t from = size(worker);
    body(begin, end, worker);
    chunks[begin / quant] = Chunk{worker, from, uint32_t(size(worker))};
  };
  parallel_run(workers, n, quant, run);
}
} // namespace daparallel

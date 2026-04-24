// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/cpuBenchmark.h>
#include <perfMon/dag_perfTimer.h>
#include <math/random/dag_random.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>
#include <osApiWrappers/dag_cpuJobs.h>
#if DAGOR_DBGLEVEL > 0
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#endif

template <>
void CpuBenchmark::update()
{
  if (lastCpuTimeUsec > 0.f && numWarmCpuFrames > WARM_CPU_FRAMES)
  {
    totalCpuTimeUsec += lastCpuTimeUsec;
    ++numCpuFrames;
  }
  else
    ++numWarmCpuFrames;
}

template <>
void CpuBenchmark::run()
{
  int64_t reft = profile_ref_ticks();
  runBenchmarks();
  lastCpuTimeUsec = profile_time_usec(reft);
}

template <>
void CpuBenchmark::resetTimings()
{
  lastCpuTimeUsec = 0.f;
  totalCpuTimeUsec = 0.f;
  numCpuFrames = 0;
  numWarmCpuFrames = 0;
}

template <>
float CpuBenchmark::getAvgCpuTimeUsec() const
{
  return totalCpuTimeUsec / max<uint64_t>(numCpuFrames, 1);
}

MatrixMulBenchmark::MatrixMulBenchmark() : matrixA{initRandomMatrix()}, matrixB{initRandomMatrix()}, identity{identityMatrix()} {}

auto MatrixMulBenchmark::initRandomMatrix() -> MatrixT
{
  MatrixT result;
  for (auto &row : result)
    for (auto &elem : row)
      elem = gauss_rnd();
  return result;
}
auto MatrixMulBenchmark::identityMatrix() -> MatrixT
{
  MatrixT result;
  for (uint32_t i = 0; i < MATRIX_SIZE; ++i)
    result.data()[i].data()[i] = 1.f;
  return result;
}

MatrixMulBenchmark::MatrixT operator*(const MatrixMulBenchmark::MatrixT &l, const MatrixMulBenchmark::MatrixT &r)
{
  MatrixMulBenchmark::MatrixT result;
  constexpr auto MATRIX_SIZE = MatrixMulBenchmark::MATRIX_SIZE;
  for (uint32_t i = 0; i < MATRIX_SIZE; ++i)
    for (uint32_t j = 0; j < MATRIX_SIZE; ++j)
      for (uint32_t k = 0; k < MATRIX_SIZE; ++k)
        result.data()[i].data()[j] += l.data()[i].data()[k] * r.data()[k].data()[j];
  return result;
}
MatrixMulBenchmark::MatrixT operator+(const MatrixMulBenchmark::MatrixT &l, const MatrixMulBenchmark::MatrixT &r)
{
  MatrixMulBenchmark::MatrixT result;
  constexpr auto MATRIX_SIZE = MatrixMulBenchmark::MATRIX_SIZE;
  for (uint32_t i = 0; i < MATRIX_SIZE; ++i)
    for (uint32_t j = 0; j < MATRIX_SIZE; ++j)
      result.data()[i].data()[j] = l.data()[i].data()[j] + r.data()[i].data()[j];
  return result;
}
MatrixMulBenchmark::MatrixT operator-(const MatrixMulBenchmark::MatrixT &l, const MatrixMulBenchmark::MatrixT &r)
{
  MatrixMulBenchmark::MatrixT result;
  constexpr auto MATRIX_SIZE = MatrixMulBenchmark::MATRIX_SIZE;
  for (uint32_t i = 0; i < MATRIX_SIZE; ++i)
    for (uint32_t j = 0; j < MATRIX_SIZE; ++j)
      result.data()[i].data()[j] = l.data()[i].data()[j] - r.data()[i].data()[j];
  return result;
}

void MatrixMulBenchmark::run()
{
  auto matrixAB = matrixA * matrixB;
  auto matrixBA = matrixB * matrixA;
  matrixA = (identity + matrixB) * matrixA - matrixBA;
  matrixB = (identity + matrixA) * matrixB - matrixAB;
}

void SortBenchmark::run()
{
  FRAMEMEM_VALIDATE;
  constexpr size_t n = 1'000'00;
  dag::Vector<int, framemem_allocator> unsortedVec(n);
  eastl::iota(unsortedVec.rbegin(), unsortedVec.rend(), 0);
  eastl::sort(unsortedVec.begin(), unsortedVec.end());
}

GameOfLifeBenchmark::GameOfLifeBenchmark() : area{AREA_SIZE, dag::Vector<uint8_t>(AREA_SIZE)}, prevArea{area}
{
  for (auto &row : area)
    for (auto &elem : row)
      elem = rnd_int(0, 1);
}

void GameOfLifeBenchmark::run()
{
  eastl::swap(prevArea, area);
  for (int32_t i = 0; i < AREA_SIZE; ++i)
    for (int32_t j = 0; j < AREA_SIZE; ++j)
    {
      const auto neighbors = getNeighborCount(i, j);
      const auto wasAlive = prevArea.data()[i].data()[j];
      if (!wasAlive && neighbors == 3)
        area.data()[i].data()[j] = 1;
      else if (wasAlive && (neighbors == 2 || neighbors == 3))
        area.data()[i].data()[j] = 1;
      else
        area.data()[i].data()[j] = 0;
    }
}

uint8_t GameOfLifeBenchmark::getNeighborCount(int32_t i, int32_t j) const
{
  uint8_t total = 0;
  for (int32_t di = -1; di <= 1; ++di)
    for (int32_t dj = -1; dj <= 1; ++dj)
    {
      const auto x = (i + di + AREA_SIZE) % AREA_SIZE;
      const auto y = (j + dj + AREA_SIZE) % AREA_SIZE;
      total += prevArea.data()[x].data()[y];
    }
  return total - prevArea.data()[i].data()[j];
}

namespace cpu_benchmark
{
static int benchmark_score(float avg_cpu_time_usec)
{
  int score = lroundf((1 + 0.8 * (cpujobs::get_logical_core_count() - 1)) / (avg_cpu_time_usec * 1e-6f));
#if DAGOR_DBGLEVEL > 0
  // Force set debug score for dev version, because logs/asserts/optimizations may affect benchmark result
  auto scoreMul = ::dgs_get_settings()->getReal("cpuBenchmarkDebugScoreMul", 1.0);
  score = ::dgs_get_settings()->getInt("cpuBenchmarkDebugScoreOverride", lroundf(score * scoreMul));
#endif
  return score;
}

int run_benchmark()
{
  CpuBenchmark benchmark;
  benchmark.resetTimings();

  constexpr uint32_t RUN_COUNT_LIMIT = 100;
  uint32_t counter = 0;
  while (counter++ < RUN_COUNT_LIMIT)
  {
    benchmark.update();
    benchmark.run();
  }
  debug("CPU benchmark: %u iterations, avg time %.2f usec", RUN_COUNT_LIMIT, benchmark.getAvgCpuTimeUsec());
  return benchmark_score(benchmark.getAvgCpuTimeUsec());
}

void run_benchmark_and_log() { debug("cpu_benchmark_score: %d", cpu_benchmark::run_benchmark()); }
} // namespace cpu_benchmark

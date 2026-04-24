//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <EASTL/array.h>
#include <dag/dag_vector.h>

namespace cpu_benchmark
{
int run_benchmark();
void run_benchmark_and_log();
} // namespace cpu_benchmark

class MatrixMulBenchmark;
class SortBenchmark;
class GameOfLifeBenchmark;

template <typename... Ts>
class CpuBenchmarkCollection : private Ts...
{
public:
  CpuBenchmarkCollection() : Ts()... {}

  void update();
  void run();
  void resetTimings();

  float getAvgCpuTimeUsec() const;

private:
  void runBenchmarks() { (Ts::run(), ...); }

private:
  float lastCpuTimeUsec = 0;
  float totalCpuTimeUsec = 0;
  uint64_t numCpuFrames = 0;
  uint64_t numWarmCpuFrames = 0;
  static constexpr uint64_t WARM_CPU_FRAMES = 4;
};

using CpuBenchmark = CpuBenchmarkCollection<MatrixMulBenchmark, SortBenchmark, GameOfLifeBenchmark>;

class MatrixMulBenchmark
{
public:
  MatrixMulBenchmark();

  void run();

private:
  static constexpr size_t MATRIX_SIZE = 100;
  using MatrixT = eastl::array<eastl::array<float, MATRIX_SIZE>, MATRIX_SIZE>;

  static MatrixT initRandomMatrix();
  static MatrixT identityMatrix();

  friend MatrixT operator*(const MatrixT &l, const MatrixT &r);
  friend MatrixT operator+(const MatrixT &l, const MatrixT &r);
  friend MatrixT operator-(const MatrixT &l, const MatrixT &r);

private:
  MatrixT matrixA;
  MatrixT matrixB;
  MatrixT identity;
};

class SortBenchmark
{
public:
  void run();
};

class GameOfLifeBenchmark
{
public:
  GameOfLifeBenchmark();

  void run();

private:
  uint8_t getNeighborCount(int32_t i, int32_t j) const;

  static constexpr size_t AREA_SIZE = 256;
  dag::Vector<dag::Vector<uint8_t>> area;
  dag::Vector<dag::Vector<uint8_t>> prevArea;
};

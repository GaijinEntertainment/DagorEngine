#pragma once

struct AllocatedPoolBlock
{
  int pool, start, size;
  AllocatedPoolBlock() : pool(-1), start(-1), size(0) {}
};
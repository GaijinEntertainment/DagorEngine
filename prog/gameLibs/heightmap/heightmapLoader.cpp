// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/heightmapHandler.h>
#include <ioSys/dag_fileIo.h>

bool load_heightmap_raw32(HeightmapHandler &heightmap, IGenLoad &crd, float cell_size, const Point2 *at)
{
  uint64_t sz = crd.getTargetDataSize();
  uint32_t w = sqrt(sz / 4);
  if (w * w * 4 != sz || w == 0)
  {
    logerr("%s is not raw32 file", crd.getTargetName());
    return false;
  }
  Tab<float> r32;
  r32.resize(w * w);
  crd.read(r32.data(), w * w * 4);
  Tab<uint16_t> r16;
  r16.resize(w * w);
  float hMin = r32[0], hMax = r32[0];
  for (auto f : r32)
  {
    hMin = min(f, hMin);
    hMax = max(f, hMax);
  }
  hMax = max(hMax, hMin + 1e-32f);
  float scale = 65535.f / (hMax - hMin);
  auto to = r16.data();
  for (int y = 0; y < w; ++y)
  {
    auto from = r32.data() + (w - 1 - y) * w;
    for (int x = 0; x < w; ++x, ++to, ++from)
      *to = clamp<int>((*from - hMin) * scale + 0.5f, 0, 65535);
  }
  heightmap.initRaw(r16.data(), cell_size, w, w, hMin, hMax - hMin, at ? *at : -0.5 * cell_size * Point2(w, w));
  return true;
}

bool load_heightmap_raw32(HeightmapHandler &heightmap, const char *fn, float cell_size, const Point2 *at)
{
  if (!fn)
    return false;
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    logerr("can't open %s", fn);
    return false;
  }
  return load_heightmap_raw32(heightmap, crd, cell_size, at);
}

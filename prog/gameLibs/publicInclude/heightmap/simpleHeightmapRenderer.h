//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class ShaderMaterial;
class ShaderElement;
class LodGrid;
class LodGridData;

class SimpleHeightmapRenderer
{
public:
  bool init(const char *shader_name, bool do_fatal, int dim_bits);
  bool isInited() const { return shElem != NULL; }
  static void render(const LodGridCullData &cull_data, const ShaderElement *shElem, int dim_bits);
  ~SimpleHeightmapRenderer() { close(); }
  void close();
  int getDim() const { return 1 << dimBits; }
  int getDimBits() const { return dimBits; }
  const ShaderElement *getShElem() const { return shElem; }

protected:
  ShaderMaterial *shmat = nullptr;
  ShaderElement *shElem = nullptr;
  int dimBits = 0;
};

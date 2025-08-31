// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// #include <de3_grassSrv.h>
#include <de3_gpuGrassService.h>
#include <render/gpuGrass.h>

class GPUGrass;

struct RandomGPUGrassRenderHelper : IRandomGrassRenderHelper
{
  BBox3 box;
  IRenderingService *hmap = nullptr;

  bool beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &glob_tm, const TMatrix4 &proj_tm) override;
  void endRender() override;
  void renderColor() override;
  void renderMask() override;
  bool isValid() const;
};

class GPUGrassService : public IGPUGrassService
{
  eastl::unique_ptr<GPUGrass> grass;
  bool enabled = false;
  RandomGPUGrassRenderHelper grassHelper;
  eastl::vector<GPUGrassType> grassTypes;
  eastl::vector<GPUGrassDecal> grassDecals;
  void enumerateGrassTypes(DataBlock &grassBlk);
  void enumerateGrassDecals(DataBlock &grassBlk);
  int findFreeDecalId() const;

public:
  bool srvEnabled;
  void createGrass(DataBlock &grassBlk) override;
  void closeGrass() override;
  DataBlock *createDefaultGrass() override;
  void enableGrass(bool flag) override;
  void beforeRender(Stage stage) override;
  void renderGeometry(Stage stage) override;
  BBox3 *getGrassBbox() override;

  bool isGrassEnabled() const override;
  int getTypeCount() const override;
  GPUGrassType &getType(int index) override;
  GPUGrassType &addType(const char *name) override;
  void removeType(const char *name) override;
  GPUGrassType *getTypeByName(const char *name) override;

  int getDecalCount() const override;
  GPUGrassDecal &getDecal(int index) override;
  GPUGrassDecal &addDecal(const char *name) override;
  void removeDecal(const char *name) override;
  GPUGrassDecal *getDecalByName(const char *name) override;
  bool findDecalId(int id) const override;

  void invalidate() override;
};
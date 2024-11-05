//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/integer/dag_IBBox2.h>
#include <EASTL/unique_ptr.h>
#include <terraform/terraform.h>
#include <terraform/terraformDig.h>

namespace danet
{
class BitStream;
}

class HeightmapHandler;

class TerraformComponent : public Terraform::Listener
{
public:
  using Pcd = Terraform::Pcd;
  using PcdAlt = Terraform::PcdAlt;
  using PrimMode = Terraform::PrimMode;

  TerraformComponent() = default;
  TerraformComponent(const TerraformComponent &);
  TerraformComponent(TerraformComponent &&) = delete;

  void init(Terraform *in_tform = NULL);
  void init(HeightmapHandler *in_hmap, const Terraform::Desc &terraform_desc);
  ~TerraformComponent();

  TerraformComponent &operator=(const TerraformComponent &rhs);
  TerraformComponent &operator=(TerraformComponent &&) = delete;

  void serialize(danet::BitStream &bs) const;
  void deserialize(danet::BitStream &bs);

  bool isEqual(const TerraformComponent &rhs) const;
  uint32_t getGen() const { return gen; }
  void setGen(uint32_t _gen) { gen = _gen; }

  void applyDelayedPatches();

  void copy(const TerraformComponent &rhs);

  void update();
  void storeSphereAlt(const Point2 &pos, float radius, float alt, PrimMode mode);
  void queueElevationChange(const Point2 &pos, float radius, float alt);
  void makeBombCraterPart(const Point2 &pos, float inner_radius, float inner_depth, float outer_radius, float outer_alt,
    const Point2 &part_pos, float part_radius);

  float getHmapHeightOrigValAtPos(const Point2 &pos) const;

protected:
  virtual void allocPatch(int patch_no);
  virtual void storePatchAlt(const Pcd &pcd, uint8_t alt);

  void changeTForm(Terraform *in_tform);
  void changeGen() { ++gen; }

private:
  // During deserialization when terraforming
  // might be uninitialized properly.
  // In case replication occurs before the hmap is loaded.
  struct DelayedPatch
  {
    uint16_t patchNo;
    uint16_t indexBase;

    uint8_t countIndexes;
    uint8_t altU8;
  };

  eastl::unique_ptr<TerraformDig> terraformDig;
  Terraform *tform = NULL;
  eastl::unique_ptr<Terraform> tformInst;
  Tab<DelayedPatch> delayedPatches;
  uint32_t gen = 0;
};

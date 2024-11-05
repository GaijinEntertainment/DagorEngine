//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <terraform/terraform.h>
#include <generic/dag_tab.h>

struct TerraformDigCtx;

class TerraformDig
{
public:
  using Pcd = Terraform::Pcd;
  using PcdAlt = Terraform::PcdAlt;

  struct DigContext;

  struct Desc
  {
    float maxPrimHeight = 1.0f;
    float consumeWeight = 1.0f;
    float heapAspect = 0.5f;
  };

  struct DigCell
  {
    IPoint2 coord;
    uint8_t alt;
  };

  TerraformDig(Terraform &in_tform, const Desc &in_desc);
  virtual ~TerraformDig();

  TerraformDigCtx *startDigging(const Point2 &pos, const Point2 &dir, float width, float d_alt);
  void stopDigging(TerraformDigCtx *dig_ctx);
  void advanceDigging(TerraformDigCtx *dig_ctx, const Point2 &pos, const Point2 &dir);

  Point3 getDiggingPrimSize(TerraformDigCtx *dig_ctx) const;
  float getDiggingMaxAlt(TerraformDigCtx *dig_ctx) const;
  void getDiggingMass(TerraformDigCtx *dig_ctx, float &out_total_mass, float &out_left_mass) const;
  BBox2 getDigRegion(TerraformDigCtx *dig_ctx) const;
  dag::ConstSpan<TerraformDigCtx *> getDigContexts() const;

  void makeBombCrater(const Point2 &pos, float inner_radius, float inner_depth, float outer_radius, float outer_alt);
  void makeBombCraterPart(const Point2 &pos, float inner_radius, float inner_depth, float outer_radius, float outer_alt,
    const Point2 &part_pos, float part_radius);
  void makeAreaPlate(const TMatrix &area_tm, float smoothing_area_width, float delta, float max_noise);
  void makeAreaCylinder(const TMatrix &area_tm, float smoothing_area_width, float delta, float max_noise);

  void getBBChanges(Tab<BBox2> &out_boxes) const;
  void clearBBChanges();

  void drawDebug();
  void testDebug(const BBox3 &box, const TMatrix &tm);

protected:
  virtual void doStorePatchAlt(const Pcd &pcd, uint8_t alt);
  virtual bool doSubmitQuad(const Terraform::QuadData &quad, uint16_t &prim_id);
  virtual void doRemovePrim(uint16_t prim_id);

  Terraform &tform;
  Desc desc;
  const int BLUR_RADIUS;

private:
  void storePrim(TerraformDigCtx *dig_ctx);

  Tab<TerraformDigCtx *> digContexts;
};

// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <terraform/terraformComponent.h>
void TerraformComponent::update() { G_ASSERT(0); }
void TerraformComponent::storeSphereAlt(class Point2 const &, float, float, TerraformComponent::PrimMode) { G_ASSERT(0); }
void TerraformComponent::queueElevationChange(const Point2 &pos, float radius, float alt) { G_ASSERT(0); }
void TerraformComponent::makeBombCraterPart(const Point2 &pos,
  float inner_radius,
  float inner_depth,
  float outer_radius,
  float outer_alt,
  const Point2 &part_pos,
  float part_radius)
{
  G_ASSERT(0);
}
float TerraformComponent::getHmapHeightOrigValAtPos(const Point2 &pos) const { G_ASSERT_RETURN(false, 0.0f); }
TerraformComponent &TerraformComponent::operator=(class TerraformComponent const &) { G_ASSERT_RETURN(false, *this); }

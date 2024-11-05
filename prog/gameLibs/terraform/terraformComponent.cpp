// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <terraform/terraformComponent.h>
#include <daNet/bitStream.h>


void TerraformComponent::init(Terraform *in_tform)
{
  G_ASSERT(in_tform);
  changeTForm(in_tform);
  changeGen();
  applyDelayedPatches();
}

void TerraformComponent::init(HeightmapHandler *in_hmap, const Terraform::Desc &terraform_desc)
{
  if (!tformInst || &tformInst->getHMap() != in_hmap)
    tformInst.reset(new Terraform(*in_hmap, terraform_desc));
  else
    tformInst->setDesc(terraform_desc);
  init(tformInst.get());
}

TerraformComponent::~TerraformComponent()
{
  changeTForm(NULL);
  tformInst.reset();
}

TerraformComponent::TerraformComponent(const TerraformComponent &rhs) { copy(rhs); }

TerraformComponent &TerraformComponent::operator=(const TerraformComponent &rhs)
{
  if (this == &rhs)
    return *this;
  copy(rhs);
  return *this;
}

void TerraformComponent::serialize(danet::BitStream &bs) const
{
  if (!tform)
    return;

  G_ASSERT_RETURN(tform->getPatchNum() - 1 <= UINT16_MAX, );
  // format for each patch: patchNo:16, patchBlockSize:16, [{alt:8, indexDelta:16c, countIndexes:8}]
  for (int patchNo = 0; patchNo < tform->getPatchNum(); ++patchNo)
  {
    int blockSizeOffset = -1;
    dag::ConstSpan<uint8_t> patch = tform->getPatch(patchNo);
    uint8_t lastAlt = tform->getZeroAlt();
    uint16_t altCounter = 0;
    uint16_t indexBase = 0;
    if (patch.size())
    {
      for (int index = 0; index < patch.size(); ++index)
      {
        G_ASSERT(index <= UINT16_MAX);
        if (patch[index] != tform->getZeroAlt())
        {
          if (blockSizeOffset == -1)
          {
            bs.Write((uint16_t)patchNo);
            blockSizeOffset = bs.GetWriteOffset();
            bs.Write(uint32_t(0)); // block size
          }
          if (patch[index] != lastAlt || altCounter >= UINT8_MAX)
          {
            if (altCounter > 0)
            {
              bs.Write((uint8_t)altCounter); // prev altitudes count
              altCounter = 0;
            }
            bs.Write(patch[index]);                          // altitude
            bs.WriteCompressed(uint16_t(index - indexBase)); // start index
            indexBase = index;
          }
          altCounter++;
        }
        else if (altCounter > 0) // end stride
        {
          bs.Write((uint8_t)altCounter); // altitudes count
          altCounter = 0;
        }
        lastAlt = patch[index];
      }
      if (altCounter > 0) // end stride
      {
        bs.Write((uint8_t)altCounter); // altitudes count
        altCounter = 0;
      }
      if (blockSizeOffset != -1) // end block
      {
        bs.WriteAt(uint32_t(bs.GetWriteOffset() - blockSizeOffset), blockSizeOffset);
        blockSizeOffset = -1;
      }
    }
  }
  if (bs.GetNumberOfBytesUsed() > 128 << 10)
    LOGWARN_CTX("data size = %d byte of terraform in serialize", bs.GetNumberOfBytesUsed());
}

void TerraformComponent::deserialize(danet::BitStream &bs)
{
  // format for each patch: patchNo:16, patchBlockSize:16, [{alt:8, indexDelta:16c, countIndexes:8}]
  bool ok = true;
  while (ok && bs.GetNumberOfUnreadBits() > 0)
  {
    uint16_t patchNo = 0;
    uint32_t blockSize = 0;
    uint16_t indexBase = 0;
    ok &= bs.Read(patchNo);
    uint32_t readPos = bs.GetReadOffset();
    ok &= bs.Read(blockSize);
    while (ok && bs.GetReadOffset() < readPos + blockSize)
    {
      uint8_t altU8 = 0;
      uint16_t indexDelta = 0;
      uint8_t countIndexes = 0;
      ok &= bs.Read(altU8);
      ok &= bs.ReadCompressed(indexDelta);
      ok &= bs.Read(countIndexes);
      indexBase += indexDelta;

      if (tform)
      {
        for (int i = 0; i < countIndexes; i++)
          tform->storePatchAlt({patchNo, uint16_t(indexBase + i)}, altU8);
      }
      else
        delayedPatches.push_back() = {patchNo, indexBase, countIndexes, altU8};
    }
  }
}

void TerraformComponent::applyDelayedPatches()
{
  if (!tform)
    return;

  for (const DelayedPatch &p : delayedPatches)
    for (int i = 0; i < p.countIndexes; i++)
      tform->storePatchAlt({p.patchNo, uint16_t(p.indexBase + i)}, p.altU8);

  clear_and_shrink(delayedPatches);
}

void TerraformComponent::copy(const TerraformComponent &rhs)
{
  if (tform && rhs.tform)
  {
    init(rhs.tform);
    tform->copy(*rhs.tform);
  }
  delayedPatches = rhs.delayedPatches;
}

void TerraformComponent::update()
{
  if (!tform)
    return;
  tform->update();
}

void TerraformComponent::storeSphereAlt(const Point2 &pos, float radius, float alt, PrimMode mode)
{
  if (!tform)
    return;
  tform->storeSphereAlt(pos, radius, alt, mode);
}

void TerraformComponent::queueElevationChange(const Point2 &pos, float radius, float alt)
{
  if (!tform)
    return;
  tform->queueElevationChange(pos, radius, alt);
}

void TerraformComponent::makeBombCraterPart(const Point2 &pos, float inner_radius, float inner_depth, float outer_radius,
  float outer_alt, const Point2 &part_pos, float part_radius)
{
  if (terraformDig)
  {
    float terraformIndependenceScale = tformInst->getWorldSize().x / tformInst->getHMapSizes().x;
    outer_alt *= terraformIndependenceScale; // It's a speculative fix for craters looks different on different maps
    terraformDig->makeBombCraterPart(pos, inner_radius, inner_depth, outer_radius, outer_alt, part_pos, part_radius);
  }
}

float TerraformComponent::getHmapHeightOrigValAtPos(const Point2 &pos) const
{
  if (tform)
    return tform->getHmapHeightOrigVal(tform->getHmapCoordFromCoord(tform->getCoordFromPos(pos)));
  return 0.0f;
}

bool TerraformComponent::isEqual(const TerraformComponent &rhs) const { return tform && rhs.tform && tform->isEqual(*rhs.tform); }

void TerraformComponent::changeTForm(Terraform *in_tform)
{
  if (tform)
    tform->unregisterListener(this);
  tform = in_tform;

  if (tform)
  {
    TerraformDig::Desc terraformDigDesc;
    terraformDigDesc.maxPrimHeight = in_tform->getDesc().getMaxLevel();
    terraformDig.reset(new TerraformDig(*tform, terraformDigDesc));
  }
  else
    terraformDig.reset(nullptr);

  if (tform)
    tform->registerListener(this);
}

void TerraformComponent::storePatchAlt(const Pcd &pcd, uint8_t alt)
{
  G_ASSERT(tform);
  G_UNUSED(pcd);
  G_UNUSED(alt);
  changeGen();
}

void TerraformComponent::allocPatch(int patch_no)
{
  G_ASSERT(tform);
  G_UNUSED(patch_no);
  changeGen();
}
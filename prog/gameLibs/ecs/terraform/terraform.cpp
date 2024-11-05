// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/terraform/terraform.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/baseIo.h>

#include <memory/dag_framemem.h>
#include <daNet/bitStream.h>
#include <ioSys/dag_dataBlock.h>

struct TerraformSerializer final : public ecs::ComponentSerializer
{
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TerraformComponent>::type);
    G_UNUSED(hint);

    const TerraformComponent *terraformComponent = (const TerraformComponent *)data;
    danet::BitStream bs(framemem_ptr());
    terraformComponent->serialize(bs);
    const uint32_t bitsCount = bs.GetNumberOfBitsUsed();
    ecs::write_compressed(cb, bitsCount);
    cb.write(bs.GetData(), bitsCount, 0);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TerraformComponent>::type);
    G_UNUSED(hint);

    uint32_t bitsCount = 0;
    bool isOk = ecs::read_compressed(cb, bitsCount);

    danet::BitStream bs(framemem_ptr());
    bs.reserveBits(bitsCount);
    bs.SetWriteOffset(bitsCount);
    isOk &= cb.read(bs.GetData(), bitsCount, 0);

    TerraformComponent *terraformComponent = (TerraformComponent *)data;
    terraformComponent->deserialize(bs);

    return isOk;
  }
} terraform_serializer;

struct TerraformConstruct : public TerraformComponent
{
  bool operator==(const TerraformConstruct &rhs) const { return isEqual(rhs); }

  bool replicateCompare(const TerraformConstruct &rhs)
  {
    if (getGen() == rhs.getGen())
    {
#if DAECS_EXTENSIVE_CHECKS
      if (!isEqual(rhs))
        logerr("TerraformComponent differs, while it's generation is same");
      else
        return false;
#else
      return false;
#endif
    }
    if (!isEqual(rhs))
    {
      *this = rhs;
      setGen(rhs.getGen());
      return true;
    }
    setGen(rhs.getGen());
    return false;
  }
};

ECS_REGISTER_MANAGED_TYPE(TerraformComponent, &terraform_serializer,
  typename ecs::CreatorSelector<TerraformComponent ECS_COMMA TerraformConstruct>::type);
ECS_AUTO_REGISTER_COMPONENT(TerraformComponent, "terraform", nullptr, 0);

void terraform::init(const DataBlock *terraform_blk, HeightmapHandler *hmap, TerraformComponent &terraformComponent)
{
  G_ASSERT(hmap);

  TerraformConstruct &tformConsruct = (TerraformConstruct &)terraformComponent;

  Terraform::Desc terraformDesc;
  terraformDesc.cellsPerMeter = terraform_blk->getInt("cellsPerMeter", 8);

  Point2 minMaxLevel = Point2(terraform_blk->getReal("minLevel", -2.0f), terraform_blk->getReal("maxLevel", 1.5f));
  terraformDesc.minLevel = minMaxLevel.x;
  terraformDesc.maxLevel = minMaxLevel.y;

  tformConsruct.init(hmap, terraformDesc);
}
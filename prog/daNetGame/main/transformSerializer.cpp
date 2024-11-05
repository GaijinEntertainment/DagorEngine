// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/quantization.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/baseIo.h>
#include <math/dag_mathUtils.h>

#define LOG_NON_ORTHO 0

// instead of 48 bytes we typically write 20/24 (12 position + 8 quat + (may be) 4 scale) for (scaled) ortho-normal matrices
// which is twice less
static struct TransformSerializer final : public ecs::ComponentSerializer
{
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TMatrix>::type);
    G_UNUSED(hint);
    const TMatrix &transform = *(const TMatrix *)data;
    float len0 = lengthSq(transform.getcol(0));
    const bool isOrthoUni =
      (fabsf(lengthSq(transform.getcol(1)) - len0) < 1e-3f && fabsf(lengthSq(transform.getcol(2)) - len0) < 1e-3f &&
        fabsf(dot(transform.getcol(0), transform.getcol(1))) < 1e-5f && fabsf(dot(transform.getcol(0), transform.getcol(2))) < 1e-5f &&
        fabsf(dot(transform.getcol(1), transform.getcol(2))) < 1e-5f &&
        dot(transform.getcol(0) % transform.getcol(1), transform.getcol(2)) > 0.f);
    cb.write(&isOrthoUni, 1, 0);
    if (isOrthoUni)
    {
      len0 = sqrtf(len0);
      const bool hasScale = fabsf(len0 - 1.0f) > 1e-5f;
      gamemath::QuantizedQuat62 quanitizedQuat(normalize(matrix_to_quat(hasScale ? transform * (1.f / len0) : transform)));
      cb.write(&quanitizedQuat.qquat, 62, 0);
      cb.write(&hasScale, 1, 0);
      if (hasScale)
        cb.write(&len0, sizeof(float) * 8, 0);
    }
    else
    {
#if LOG_NON_ORTHO
      debug("transform = %@ len0 %f len %f %f dot %f %f %f", transform, len0, fabsf(lengthSq(transform.getcol(1)) - len0),
        fabsf(lengthSq(transform.getcol(2)) - len0), fabsf(dot(transform.getcol(0), transform.getcol(1))) < 1e-5f,
        fabsf(dot(transform.getcol(0), transform.getcol(2))) < 1e-5f, fabsf(dot(transform.getcol(1), transform.getcol(2))) < 1e-5f);
#endif
      cb.write(transform[0], sizeof(float) * 9 * CHAR_BIT, 0);
    }
    cb.write(transform[3], sizeof(float) * 3 * CHAR_BIT, 0); // do not quanitized position
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TMatrix>::type);
    G_UNUSED(hint);
    TMatrix &transform = *(TMatrix *)data;
    bool isOrthoUni = false;
    bool isOk = cb.read(&isOrthoUni, 1, 0);
    if (isOrthoUni)
    {
      gamemath::QuantizedQuat62 quanitizedQuat;
      isOk &= cb.read(&quanitizedQuat.qquat, 62, 0);
      transform = quat_to_matrix(quanitizedQuat.unpackQuat());
      bool hasScale = false;
      isOk &= cb.read(&hasScale, 1, 0);
      if (hasScale)
      {
        float scale;
        isOk &= cb.read(&scale, sizeof(float) * 8, 0);
        transform *= scale;
      }
    }
    else
      isOk &= cb.read(transform[0], sizeof(float) * 9 * 8, 0);

    isOk &= cb.read(transform[3], sizeof(float) * 3 * 8, 0); // do not quanitized transform
    return isOk;
  }
} transform_serializer;

ECS_AUTO_REGISTER_COMPONENT(TMatrix, "transform", &transform_serializer, 0);
ECS_DEF_PULL_VAR(transformSerializer);

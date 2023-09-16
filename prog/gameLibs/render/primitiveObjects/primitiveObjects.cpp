#include <render/primitiveObjects.h>
#include <math/dag_Point4.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_materialData.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d.h>


#define MAX_SPHERE_SLICES 64
#define MAX_SPHERE_STACKS 64


void calc_sphere_vertex_face_count(uint32_t slices, uint32_t stacks, bool /*hemisphere*/, uint32_t &out_vertex_count,
  uint32_t &out_face_count)
{
  out_face_count = 2 * (stacks) * (slices);
  out_vertex_count = (stacks + 1) * (slices + 1);
}

void create_sphere_mesh(dag::Span<uint8_t> pVertex, dag::Span<uint8_t> pwFace, float radius, uint32_t slices, uint32_t stacks,
  uint32_t stride, bool norm, bool tex, bool use_32_instead_of_16_indices, bool hemisphere)
{
  G_ASSERT(stacks >= 2 && stacks <= MAX_SPHERE_STACKS);
  G_ASSERT(slices >= 2 && slices <= MAX_SPHERE_SLICES);

  uint32_t i, j;

  // Sin/Cos caches
  carray<float, MAX_SPHERE_SLICES + 1> sinI, cosI;
  carray<float, MAX_SPHERE_STACKS + 1> sinJ, cosJ;

  for (i = 0; i <= slices; i++)
    sincos(2.0f * PI * i / slices, sinI[i], cosI[i]);

  for (j = 0; j <= stacks; j++)
    sincos((hemisphere ? PI / 2 : PI) * j / (hemisphere ? stacks - 1 : stacks), sinJ[j], cosJ[j]);

  // Generate vertices
  uint32_t vert = 0;
  uint32_t texOffset = norm ? 24 : 12;

#define SET_POS(a, b)                         \
  do                                          \
  {                                           \
    (*((Point3 *)(&a[vert * stride])) = (b)); \
  } while (0)
#define SET_NORM(a, b)                               \
  do                                                 \
  {                                                  \
    if (norm)                                        \
      (*((Point3 *)(&a[vert * stride] + 12)) = (b)); \
  } while (0)
#define SET_TEX_U(a, b)                                    \
  do                                                       \
  {                                                        \
    if (tex)                                               \
      (*((float *)(&a[vert * stride] + texOffset)) = (b)); \
  } while (0)
#define SET_TEX_V(a, b)                                        \
  do                                                           \
  {                                                            \
    if (tex)                                                   \
      (*((float *)(&a[vert * stride] + texOffset + 4)) = (b)); \
  } while (0)

  // Stacks
  for (j = 0; j <= stacks; j++)
  {
    for (i = 0; i <= slices; i++)
    {
      Point3 curNorm = Point3(sinJ[j] * cosI[i], cosJ[j], sinJ[j] * sinI[i]);

      SET_POS(pVertex, curNorm * radius);
      SET_NORM(pVertex, curNorm);
      SET_TEX_U(pVertex, i / (float)slices);
      SET_TEX_V(pVertex, j / (float)stacks);

      vert++;
    }
  }
  // Generate indices
  uint32_t ind = 0;
  uint32_t indSize = use_32_instead_of_16_indices ? sizeof(uint32_t) : sizeof(uint16_t);

#define SET_IND(a, b, c)                              \
  if (use_32_instead_of_16_indices)                   \
    (*((uint32_t *)(&a[(ind + b) * indSize])) = (c)); \
  else                                                \
    (*((uint16_t *)(&a[(ind + b) * indSize])) = (c));


  for (int v = 0; v < stacks; ++v)
  {
    for (int h = 0; h < slices; ++h)
    {
      short lt = (short)(h + v * (slices + 1));
      short rt = (short)((h + 1) + v * (slices + 1));

      short lb = (short)(h + (v + 1) * (slices + 1));
      short rb = (short)((h + 1) + (v + 1) * (slices + 1));

      SET_IND(pwFace, 0, lt);
      SET_IND(pwFace, 1, rt);
      SET_IND(pwFace, 2, lb);

      SET_IND(pwFace, 3, rt);
      SET_IND(pwFace, 4, rb);
      SET_IND(pwFace, 5, lb);

      ind += 6;
    }
  }
#undef SET_IND
}

void calc_cylinder_vertex_face_count(uint32_t slices, uint32_t stacks, uint32_t &out_vertex_count, uint32_t &out_face_count)
{
  out_face_count = 2 * stacks * (slices - 1);
  out_vertex_count = (stacks + 1) * slices;
}

void create_cylinder_mesh(dag::Span<uint8_t> pVertex, dag::Span<uint8_t> pwFace, float radius, float height, uint32_t slices,
  uint32_t stacks, uint32_t stride, bool norm, bool tex, bool use_32_instead_of_16_indices)
{
  G_ASSERT(stacks >= 3 && stacks <= MAX_SPHERE_STACKS);
  G_ASSERT(slices >= 2 && slices <= MAX_SPHERE_SLICES);

  // Generate vertices
  uint32_t vert = 0;
  uint32_t texOffset = norm ? 24 : 12;

#define SET_POS(a, b)                         \
  do                                          \
  {                                           \
    (*((Point3 *)(&a[vert * stride])) = (b)); \
  } while (0)
#define SET_NORM(a, b)                               \
  do                                                 \
  {                                                  \
    if (norm)                                        \
      (*((Point3 *)(&a[vert * stride] + 12)) = (b)); \
  } while (0)
#define SET_TEX_U(a, b)                                    \
  do                                                       \
  {                                                        \
    if (tex)                                               \
      (*((float *)(&a[vert * stride] + texOffset)) = (b)); \
  } while (0)
#define SET_TEX_V(a, b)                                        \
  do                                                           \
  {                                                            \
    if (tex)                                                   \
      (*((float *)(&a[vert * stride] + texOffset + 4)) = (b)); \
  } while (0)

  for (int i = 0; i < slices; ++i)
  {
    for (int j = 0; j <= stacks; ++j)
    {
      float sinJ;
      float cosJ;
      sincos(2 * PI * j / stacks, sinJ, cosJ);

      Point3 curNorm = Point3(cosJ, 0.0f, sinJ);
      Point3 pos = Point3(curNorm.x * radius, height * (-0.5f + i / (slices - 1)), curNorm.z * radius);

      SET_POS(pVertex, pos);
      SET_NORM(pVertex, curNorm);
      SET_TEX_U(pVertex, i / (float)slices);
      SET_TEX_V(pVertex, j / (float)stacks);
      vert++;
    }
  }

  // Generate indices
  uint32_t ind = 0;
  uint32_t indSize = use_32_instead_of_16_indices ? sizeof(uint32_t) : sizeof(uint16_t);
  uint32_t uRowA, uRowB;

#define SET_IND(a, b, c)                              \
  if (use_32_instead_of_16_indices)                   \
    (*((uint32_t *)(&a[(ind + b) * indSize])) = (c)); \
  else                                                \
    (*((uint16_t *)(&a[(ind + b) * indSize])) = (c));

  for (int i = 1; i < slices; ++i)
  {
    uRowA = (i - 1) * (stacks + 1);
    uRowB = i * (stacks + 1);

    for (int j = 0; j < stacks; ++j)
    {
      SET_IND(pwFace, 0, uRowA + j);
      SET_IND(pwFace, 1, uRowB + j);
      SET_IND(pwFace, 2, uRowA + j + 1);
      ind += 3;

      SET_IND(pwFace, 0, uRowA + j + 1);
      SET_IND(pwFace, 1, uRowB + j);
      SET_IND(pwFace, 2, uRowB + j + 1);
      ind += 3;
    }
  }
#undef SET_IND
}

void create_cubic_indices(dag::Span<uint8_t> pwFace, int count, bool use_32_instead_of_16_indices)
{
  uint32_t indSize = use_32_instead_of_16_indices ? sizeof(uint32_t) : sizeof(uint16_t);

#define SET_IND(a, b, c)                                               \
  if (use_32_instead_of_16_indices)                                    \
    (*((uint32_t *)(&a[(b + startIndex) * indSize])) = ((uint32_t)c)); \
  else                                                                 \
    (*((uint16_t *)(&a[(b + startIndex) * indSize])) = ((uint16_t)c));

#define SET_FACE(a, s, v1, v2, v3, v4, v5, v6) \
  SET_IND(a, s + 0, index0 + v1);              \
  SET_IND(a, s + 1, index0 + v2);              \
  SET_IND(a, s + 2, index0 + v3);              \
  SET_IND(a, s + 3, index0 + v4);              \
  SET_IND(a, s + 4, index0 + v5);              \
  SET_IND(a, s + 5, index0 + v6);

  for (int i = 0; i < count; i++)
  {
    int index0 = i * 8 - 1;
    int startIndex = i * 36;

    // top 123 324
    SET_FACE(pwFace, 0, 1, 2, 3, 3, 2, 4);

    // bottom 576 678
    SET_FACE(pwFace, 6, 5, 7, 6, 6, 7, 8);

    // front 135 537
    SET_FACE(pwFace, 12, 1, 3, 5, 5, 3, 7);

    // back 426 468
    SET_FACE(pwFace, 18, 4, 2, 6, 4, 6, 8);

    // right 215 256
    SET_FACE(pwFace, 24, 2, 1, 5, 2, 5, 6);

    // left 347 487
    SET_FACE(pwFace, 30, 3, 4, 7, 4, 8, 7);
  }
#undef SET_IND
#undef SET_FACE
}

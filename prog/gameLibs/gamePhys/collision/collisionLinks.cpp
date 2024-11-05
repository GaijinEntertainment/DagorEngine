// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/collisionLinks.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_mathAng.h>
#include <vecmath/dag_vecMath.h>

namespace dacoll
{

static FastNameMap collision_names;

void load_collision_links(CollisionLinks &links, const DataBlock &blk, float scale)
{
  CollisionLinkData &link = links.emplace_back();
  link.axis = blk.getPoint3("axis", Point3(0.f, 1.f, 0.f));
  link.offset = blk.getPoint3("offset", Point3(0.f, 0.f, 0.f)) * scale;
  link.capsuleRadiusScale = blk.getReal("capsuleRadiusScale", 1.0f);
  link.oriParamMult = blk.getReal("oriParamMult", 0.f);
  link.haveCollision = blk.getBool("haveCollision", false);
  link.nameId = collision_names.addNameId(blk.getBlockName());

  for (int i = 0, nb = blk.blockCount(); i < nb; ++i)
    load_collision_links(links, *blk.getBlock(i), scale);
}

static mat44f generate_collisions(const mat44f &tm, const Point2 &ori_param, const CollisionLinkData &link,
  tmp_collisions_t &out_collisions)
{
  mat44f curTm = tm;

  if (link.oriParamMult != 0.f)
  {
    mat33f rot;
    vec3f oriLen = v_length2(v_ldu_half(&ori_param.x));
    vec3f yAngle = v_splat_x(v_mul_x(v_atan2_x(v_set_x(ori_param.y), v_set_x(ori_param.x)), oriLen));
    mat33f m33;
    v_mat33_from_mat44(m33, curTm);
    v_mat33_make_rot_cw_y(rot, yAngle);
    v_mat33_mul(m33, m33, rot);
    v_mat33_make_rot_cw_z(rot, v_mul(v_splats(-link.oriParamMult), oriLen));
    v_mat33_mul(m33, m33, rot);
    curTm.set33(m33);
  }
  curTm.col3 = v_mat44_mul_vec3p(curTm, v_ldu(&link.offset.x));

  vec3f from = tm.col3;
  vec3f to = curTm.col3;
  vec3f dir = v_sub(to, from);
  vec3f len = v_length3(dir);
  dir = v_safediv(dir, len);

  mat44f colTm;
  colTm.col1 = dir;
  vec3f left = v_cross3(curTm.col0, dir);
  if (v_extract_x(v_length3_sq_x(left)) < 1e-5f)
  {
    colTm.col0 = v_norm3(v_cross3(dir, curTm.col2));
    colTm.col2 = v_norm3(v_cross3(colTm.col0, dir));
  }
  else
  {
    colTm.col2 = v_norm3(left);
    colTm.col0 = v_norm3(v_cross3(dir, colTm.col2));
  }
  colTm.col3 = v_mul(v_add(from, to), V_C_HALF);
  CollisionCapsuleProperties &colProps = *(CollisionCapsuleProperties *)out_collisions.push_back_uninitialized();
  v_mat_43cu_from_mat44(colProps.tm.array, colTm);
  colProps.scale.set(link.capsuleRadiusScale, v_extract_x(len) / link.capsuleRadiusScale, 1.f);
  colProps.nameId = link.nameId;
  colProps.haveCollision = link.haveCollision;

  return curTm;
}

void generate_collisions(const TMatrix &tm, const Point2 &ori_param, const CollisionLinks &links, tmp_collisions_t &out_collisions)
{
  mat44f curTm;
  v_mat44_make_from_43cu_unsafe(curTm, tm.array);
  curTm.col3 = v_zero(); // To local coords
  for (const auto &link : links)
    curTm = generate_collisions(curTm, ori_param, link, out_collisions);
  // To world coords
  vec3f vtmcol3 = v_ldu(tm.m[3]);
  if (DAGOR_LIKELY(out_collisions.size() == 2))
  {
    v_stu_p3(&out_collisions[0].tm.m[3][0], v_add(v_ldu(out_collisions[0].tm.m[3]), vtmcol3));
    v_stu_p3(&out_collisions[1].tm.m[3][0], v_add(v_ldu(out_collisions[1].tm.m[3]), vtmcol3));
  }
  else
    for (auto &coll : out_collisions)
      v_stu_p3(&coll.tm.m[3][0], v_add(v_ldu(coll.tm.m[3]), vtmcol3));
}

void lerp_collisions(tmp_collisions_t &a_collisions, const tmp_collisions_t &b_collisions, float factor)
{
  for (int i = 0; i < a_collisions.size(); ++i)
  {
    CollisionCapsuleProperties &acol = a_collisions[i];
    for (int j = 0; j < b_collisions.size(); ++j)
    {
      const CollisionCapsuleProperties &bcol = b_collisions[j];
      if (bcol.nameId != acol.nameId)
        continue;

      v_stu_p3(&acol.scale.x, v_lerp_vec4f(v_splats(factor), v_ldu(&bcol.scale.x), v_ldu(&acol.scale.x)));

      mat44f atm, btm;
      v_mat44_make_from_43cu_unsafe(atm, acol.tm.array);
      v_mat44_make_from_43cu_unsafe(btm, bcol.tm.array);
      mat44f rtm = v_mat44_lerp(v_splats(factor), btm, atm);
      v_mat_43cu_from_mat44(acol.tm.array, rtm);
      break;
    }
  }
}

int get_link_name_id(const char *name) { return collision_names.getNameId(name); }

} // namespace dacoll

//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_debug.h>
#include <vecmath/dag_vecMath.h>

template <class FabrikBone, class FabrikJoint, class SolveJoint>
inline void backward_reach(FabrikBone *bones, const FabrikJoint *joints, const int chain_count, SolveJoint solverRev)
{
  for (int i = chain_count - 2; i >= 0; i--)
    solverRev(bones[i], bones[i + 1], joints[i + 1]);
}

template <class FabrikBone, class FabrikJoint, class GetBonePos, class SetBonePos, class SolveJoint>
inline void forward_reach(FabrikBone *bones, const FabrikJoint *joints, const int chain_count, GetBonePos getPos, SetBonePos setPos,
  SolveJoint solveFwd)
{
  for (int i = 1; i < chain_count; i++)
    setPos(bones[i], solveFwd(bones[i - 1], getPos(bones[i]), joints[i - 1]));
}

template <class FabrikBone, class FabrikJoint, class GetBonePos, class SetBonePos, class SetEndBonePos, class GetEndBonePos,
  class SolveJointFwd, class SolveJointRev>
inline bool solve_fabrik_chain(FabrikBone *chain, const FabrikJoint *joints, const int chain_count, const vec3f effector_pos,
  int num_iterations, float tolerance, float min_iteration_change, float total_chain_len, GetBonePos getPos, SetBonePos setPos,
  SetEndBonePos setEndPos, GetEndBonePos getEndPos, SolveJointFwd solverFwd, SolveJointRev solverRev)
{
  vec3f rootPos = getPos(chain[0]);

  // currently it is switched off
  if (total_chain_len >= 0) // this is optimization
  {                         // check if out of reach
    vec3f len = v_length3_x(v_sub(rootPos, effector_pos));
    if (v_extract_x(len) > total_chain_len)
    {
      for (int i = 1; i < chain_count; i++)
        setPos(chain[i], solverFwd(chain[i - 1], effector_pos, joints[i - 1]));
      solverFwd(chain[chain_count - 1], effector_pos, joints[chain_count - 1]);
      return false;
    }
  }
  float currentDistance = MAX_REAL;
  for (int i = 0; i < num_iterations; ++i)
  {
    vec3f len = v_length3_x(v_sub(getEndPos(chain[chain_count - 1]), effector_pos));
    float cDist = v_extract_x(len);
    if (cDist < tolerance)
      return true;
    if (fabsf(cDist - currentDistance) < min_iteration_change)
      return true;
    currentDistance = cDist;
    setEndPos(chain[chain_count - 1], effector_pos);
    backward_reach(chain, joints, chain_count, solverRev);

    setPos(chain[0], rootPos);
    forward_reach(chain, joints, chain_count, getPos, setPos, solverFwd);
    solverFwd(chain[chain_count - 1], effector_pos, joints[chain_count - 1]);
  }
  return false;
}

// very simple unconstrained solver
inline bool simple_solve_fabrik_chain(vec3f *chain, const int chain_count, // joints are simple length of bone stored in bone[].w
  const vec3f effector_pos, int num_iterations, float tolerance, float min_iteration_change, float total_chain_len)
{
  return solve_fabrik_chain(
    chain, chain, chain_count, effector_pos, num_iterations, tolerance, min_iteration_change, total_chain_len,
    [](vec3f bone) { return bone; },                                    // getPos
    [](vec3f &bone, vec3f a) { bone = v_sel(a, bone, V_CI_MASK0001); }, // setter
    [](vec3f &bone, vec3f a) { bone = v_sel(a, bone, V_CI_MASK0001); }, // setEndPos
    [](vec3f bone) { return bone; },                                    // getEndPos
    [](vec3f bone0Pos, vec3f dstPos, const vec4f &)                     // solve forward to
    {
      vec4f len = v_splat_w(bone0Pos);
      vec3f boneDir = v_norm3(v_sub(dstPos, bone0Pos));
      vec3f posSolved = v_add(bone0Pos, v_mul(boneDir, len));
      return posSolved;
    },
    [](vec3f &dst, vec3f bone0Pos, const vec4f &) // solve reverse
    {
      vec4f len = v_splat_w(dst);
      vec3f posSolved = v_add(bone0Pos, v_mul(v_norm3(v_sub(dst, bone0Pos)), len));
      dst = v_sel(posSolved, dst, V_CI_MASK0001);
    });
};

inline bool solve_rotational_constraint(vec3f dir, vec3f right_dir, const vec3f &up_dir, vec3f &pos,
  const vec4f &maxAngleTan) // maxAngleTan = (-tg(maxAngleLeft), tg(maxAngleRight), -tg(maxAngleDown), tg(maxAngleUp)
{
  vec4f lineProjLen = v_dot3(dir, pos);
  vec4f proj = v_mul(lineProjLen, dir);
  vec3f adjust = v_sub(pos, proj);
  proj = v_xor(proj, v_and(lineProjLen, v_msbit()));

  vec3f x_aspect = v_dot3_x(adjust, right_dir);
  vec3f y_aspect = v_dot3_x(adjust, up_dir);
  vec3f x_axis_limit = v_sel(v_splat_x(maxAngleTan), v_splat_y(maxAngleTan), v_cmp_gt(v_zero(), x_aspect));
  vec3f y_axis_limit = v_sel(v_splat_z(maxAngleTan), v_splat_w(maxAngleTan), v_cmp_gt(v_zero(), y_aspect));

  vec4f projLenSq = v_length3_sq_x(proj);
  vec3f x_aspect_sq = v_mul_x(x_aspect, x_aspect), y_aspect_sq = v_mul_x(y_aspect, y_aspect);
  vec3f ellipse =
    v_add_x(v_div_x(x_aspect_sq, v_mul_x(x_axis_limit, x_axis_limit)), v_div_x(y_aspect_sq, v_mul_x(y_axis_limit, y_axis_limit)));
  if (v_test_vec_x_eqi_0(v_and(v_cmp_ge(projLenSq, ellipse), v_cmp_ge(lineProjLen, v_zero())))) // not in bounds
  {
    vec3f xy_inv_len = v_rsqrt_x(v_add_x(x_aspect_sq, y_aspect_sq));
    vec3f x = v_mul_x(x_axis_limit, v_mul_x(x_aspect, xy_inv_len));
    vec3f y = v_mul_x(y_axis_limit, v_mul_x(y_aspect, xy_inv_len));
    vec3f pos2d = v_madd(right_dir, v_splat_x(x), v_mul(up_dir, v_splat_x(y)));
    pos = v_mul(v_norm3(v_add(proj, v_mul(v_splat_x(v_sqrt_x(projLenSq)), pos2d))), v_length3(pos));
    return true;
  }
  else
    return false;
}

inline void v_quat_mul_mat3(quat4f quat, vec3f &col0, vec3f &col1, vec3f &col2)
{
  col0 = v_quat_mul_vec3(quat, col0);
  col1 = v_quat_mul_vec3(quat, col1);
  col2 = v_quat_mul_vec3(quat, col2);
}

struct ConstrainedFABRIKSolverFwd
{
  vec3f operator()(mat44f &bone0, vec3f bone1Pos, vec4f maxAngle) const
  {
    vec4f len = v_splat_w(bone0.col3);
    vec3f boneDir = v_norm3(v_sub(bone1Pos, bone0.col3));
    vec3f posSolved = v_mul(boneDir, len); // relative to bone0.col3
    if (v_extract_x(maxAngle) > 0)
    {
      if (solve_rotational_constraint(bone0.col0, bone0.col1, bone0.col2, posSolved, maxAngle))
        boneDir = v_norm3(posSolved); // if changed
    }

    quat4f quat = v_quat_from_unit_arc(bone0.col0, boneDir);
    v_quat_mul_mat3(quat, bone0.col0, bone0.col1, bone0.col2);

    posSolved = v_add(bone0.col3, posSolved); // back non relative
    return posSolved;
  }
};

struct ConstrainedFABRIKSolverRev
{
  void operator()(mat44f &dst, mat44f_cref bone0, vec4f maxAngle) const
  {
    vec4f len = v_splat_w(dst.col3);
    vec3f bone1Pos = dst.col3;
    vec3f boneDir = v_norm3(v_sub(bone1Pos, bone0.col3));
    vec3f posSolved = v_mul(boneDir, len); // relative to bone0.col3

    if (v_extract_x(maxAngle) > 0)
    {
      if (solve_rotational_constraint(v_neg(bone0.col0), v_neg(bone0.col1), v_neg(bone0.col2), posSolved, maxAngle))
        boneDir = v_norm3(posSolved); // if changed
    }

    boneDir = v_neg(boneDir);
    quat4f quat = v_quat_from_unit_arc(dst.col0, boneDir);
    v_quat_mul_mat3(quat, dst.col0, dst.col1, dst.col2);

    posSolved = v_add(bone0.col3, posSolved); // back non relative
    dst.col3 = v_sel(posSolved, dst.col3, V_CI_MASK0001);
  }
};
#define V3D(a) v_extract_x(a), v_extract_y(a), v_extract_z(a)
// very simple constrained solver
inline bool constrained_solve_fabrik_chain(mat44f *chain, vec4f *angles, const int chain_count, // joints are simple length of bone
                                                                                                // stored in bone[].w
  const vec3f effector_pos, int num_iterations, float tolerance, float min_iteration_change, float total_chain_len)
{
  return solve_fabrik_chain(
    chain, angles, chain_count, effector_pos, num_iterations, tolerance, min_iteration_change, total_chain_len,
    [](mat44f_cref bone) { return bone.col3; },                                    // getPos
    [](mat44f &bone, vec3f a) { bone.col3 = v_sel(a, bone.col3, V_CI_MASK0001); }, // setPos
    [](mat44f &bone0, vec3f bone1Pos)                                              // simple FABRIK solve
    {
      vec3f boneDir = v_norm3(v_sub(bone1Pos, bone0.col3));
      vec3f posSolved = v_mul(boneDir, v_splat_w(bone0.col3)); // relative to bone0.col3
      quat4f quat = v_quat_from_unit_arc(bone0.col0, boneDir);
      v_quat_mul_mat3(quat, bone0.col0, bone0.col1, bone0.col2);
      bone0.col3 = v_sel(v_sub(bone1Pos, posSolved), bone0.col3, V_CI_MASK0001);
    },                                                                                   // setEndPos
    [](mat44f_cref bone) { return v_madd(v_splat_w(bone.col3), bone.col0, bone.col3); }, // getEndPos
    ConstrainedFABRIKSolverFwd(), ConstrainedFABRIKSolverRev());
};

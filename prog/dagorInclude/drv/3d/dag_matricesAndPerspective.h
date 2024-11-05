//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class TMatrix4;
class TMatrix;
struct mat44f;
struct Driver3dPerspective;

using Matrix44 = TMatrix4;

namespace d3d
{
/**
 * @brief Set the transformation matrix for the specified index.
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @param tm Pointer to a Matrix44 object.
 * @return True if the transformation matrix is set successfully, false otherwise.
 */
bool settm(int which, const Matrix44 *tm);

/**
 * @brief Set the transformation matrix for the specified index.
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @param tm Reference to a TMatrix object.
 * @return True if the transformation matrix is set successfully, false otherwise.
 */
bool settm(int which, const TMatrix &tm);

/**
 * @brief Set the transformation matrix for the specified index.
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @param tm The output mat44f object.
 * @return True if the transformation matrix is set successfully, false otherwise.
 */
void settm(int which, const mat44f &tm);

/**
 * @brief Get the transformation matrix for the specified index.
 *
 * @deprecated
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @param out_tm Pointer to a Matrix44 object to store the result.
 * @return True if the transformation matrix is retrieved successfully, false otherwise.
 */
bool gettm(int which, Matrix44 *out_tm);

/**
 * @brief Get the transformation matrix for the specified index.
 *
 * @deprecated
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @param out_tm Reference to a TMatrix object to store the result.
 * @return True if the transformation matrix is retrieved successfully, false otherwise.
 */
bool gettm(int which, TMatrix &out_tm);

/**
 * @brief Get the transformation matrix for the specified index.
 *
 * @deprecated
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @param out_tm The output mat44f object.
 */
void gettm(int which, mat44f &out_tm);

/**
 * @brief Get the constant reference to the transformation matrix for the specified index.
 *
 * @deprecated
 *
 * @param which The index of the transformation matrix. One of TM_XXX enum.
 * @return The constant reference to the transformation matrix.
 */
const mat44f &gettm_cref(int which);

/**
 * @brief Get the model to view matrix.
 *
 * @deprecated
 *
 * @param out_m2v Reference to a TMatrix object to store the result.
 */
void getm2vtm(TMatrix &out_m2v);

/**
 * @brief Get the current globtm matrix.
 *
 * @deprecated
 *
 * @param out Reference to a Matrix44 object to store the result.
 */
void getglobtm(Matrix44 &out);

/**
 * @brief Set the custom globtm matrix.
 *
 * @param globtm Reference to a Matrix44 object representing the custom globtm matrix.
 */
void setglobtm(Matrix44 &globtm);

/**
 * @brief Get the current globtm matrix.
 *
 * @deprecated
 *
 * @param out Reference to a mat44f object to store the result.
 */
void getglobtm(mat44f &out);

/**
 * @brief Set the custom globtm matrix.
 *
 * @param globtm Reference to a mat44f object representing the custom globtm matrix.
 */
void setglobtm(const mat44f &globtm);

/**
 * @brief Calculate and set the perspective matrix.
 *
 * @param p The Driver3dPerspective object representing the perspective parameters.
 * @param proj_tm Pointer to a TMatrix4 object to store the perspective matrix. Optional.
 * @return True if the perspective matrix is calculated and set successfully, false otherwise.
 */
bool setpersp(const Driver3dPerspective &p, TMatrix4 *proj_tm = nullptr);

/**
 * @brief Get the last values set by setpersp().
 *
 * @deprecated
 *
 * @param p Reference to a Driver3dPerspective object to store the result.
 * @return True if the last values are retrieved successfully, false otherwise.
 */
bool getpersp(Driver3dPerspective &p);

/**
 * @brief Check if the given perspective parameters are valid.
 *
 * @param p The Driver3dPerspective object representing the perspective parameters.
 * @return True if the perspective parameters are valid, false otherwise.
 */
bool validatepersp(const Driver3dPerspective &p);

/**
 * @brief Calculate the perspective matrix without setting it.
 *
 * @param p The Driver3dPerspective object representing the perspective parameters.
 * @param proj_tm Reference to a mat44f object to store the perspective matrix.
 * @return True if the perspective matrix is calculated successfully, false otherwise.
 */
bool calcproj(const Driver3dPerspective &p, mat44f &proj_tm);

/**
 * @brief Calculate the perspective matrix without setting it.
 *
 * @param p The Driver3dPerspective object representing the perspective parameters.
 * @param proj_tm Reference to a TMatrix4 object to store the perspective matrix.
 * @return True if the perspective matrix is calculated successfully, false otherwise.
 */
bool calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm);

/**
 * @brief Calculate the globtm matrix without setting it.
 *
 * @param view_tm The view matrix.
 * @param proj_tm The perspective matrix.
 * @param result Reference to a mat44f object to store the result.
 */
void calcglobtm(const mat44f &view_tm, const mat44f &proj_tm, mat44f &result);

/**
 * @brief Calculate the globtm matrix without setting it.
 *
 * @param view_tm The view matrix.
 * @param persp The Driver3dPerspective object representing the perspective parameters.
 * @param result Reference to a mat44f object to store the result.
 */
void calcglobtm(const mat44f &view_tm, const Driver3dPerspective &persp, mat44f &result);

/**
 * @brief Calculate the globtm matrix without setting it.
 *
 * @param view_tm The view matrix.
 * @param proj_tm The perspective matrix.
 * @param result Reference to a TMatrix4 object to store the result.
 */
void calcglobtm(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &result);

/**
 * @brief Calculate the globtm matrix without setting it.
 *
 * @param view_tm The view matrix.
 * @param persp The Driver3dPerspective object representing the perspective parameters.
 * @param result Reference to a TMatrix4 object to store the result.
 */
void calcglobtm(const TMatrix &view_tm, const Driver3dPerspective &persp, TMatrix4 &result);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool settm(int which, const Matrix44 *tm) { return d3di.settm(which, tm); }
inline bool settm(int which, const TMatrix &tm) { return d3di.settm(which, tm); }
inline void settm(int which, const mat44f &out_tm) { return d3di.settm(which, out_tm); }

inline bool gettm(int which, Matrix44 *out_tm) { return d3di.gettm(which, out_tm); }
inline bool gettm(int which, TMatrix &out_tm) { return d3di.gettm(which, out_tm); }
inline void gettm(int which, mat44f &out_tm) { return d3di.gettm(which, out_tm); }
inline const mat44f &gettm_cref(int which) { return d3di.gettm_cref(which); }

inline void getm2vtm(TMatrix &out_m2v) { return d3di.getm2vtm(out_m2v); }
inline void getglobtm(Matrix44 &m) { return d3di.getglobtm(m); }
inline void setglobtm(Matrix44 &m) { return d3di.setglobtm(m); }

inline void getglobtm(mat44f &m) { return d3di.getglobtm_vec(m); }
inline void setglobtm(const mat44f &m) { return d3di.setglobtm_vec(m); }

inline bool setpersp(const Driver3dPerspective &p, TMatrix4 *tm) { return d3di.setpersp(p, tm); }

inline bool getpersp(Driver3dPerspective &p) { return d3di.getpersp(p); }

inline bool validatepersp(const Driver3dPerspective &p) { return d3di.validatepersp(p); }

inline bool calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm) { return d3di.calcproj_0(p, proj_tm); }
inline bool calcproj(const Driver3dPerspective &p, mat44f &proj_tm) { return d3di.calcproj_1(p, proj_tm); }

inline void calcglobtm(const mat44f &v, const mat44f &p, mat44f &r) { d3di.calcglobtm_0(v, p, r); }
inline void calcglobtm(const mat44f &v, const Driver3dPerspective &p, mat44f &r) { d3di.calcglobtm_1(v, p, r); }
inline void calcglobtm(const TMatrix &v, const TMatrix4 &p, TMatrix4 &r) { d3di.calcglobtm_2(v, p, r); }
inline void calcglobtm(const TMatrix &v, const Driver3dPerspective &p, TMatrix4 &r) { d3di.calcglobtm_3(v, p, r); }
} // namespace d3d
#endif

// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/** \addtogroup de3Common
  @{
*/

class TMatrix;
class Point3;

/**
Callback interface for game cameras.
*/

class IGameCamera
{
public:
  virtual ~IGameCamera() {}

  /**
  \brief Updates camera matrix.
  */
  virtual void act() = 0;

  /**
  \brief Updates view matrix.
  */
  virtual void setView() = 0;

  /**
  \brief Sets inverse view matrix.

  See #getInvViewMatrix().

  \param[in] in_itm inverse view matrix.

  @see TMatrix getInvViewMatrix()
  */
  virtual void setInvViewMatrix(const TMatrix &in_itm) = 0;

  /**
  \brief Retrieves inverse view matrix.

  See #setInvViewMatrix().

  \param[out] out_itm matrix to save inverse view matrix to.

  @see TMatrix setInvViewMatrix()
  */
  virtual void getInvViewMatrix(TMatrix &out_itm) const = 0;
};

/**
  \brief Sets camera position (xyz format).

  See #getInvViewMatrix().

  \param[in] x value.
  \param[in] y value.
  \param[in] z value.
  \param[out] cam camera to save position to.

  @see IGameCamera
  */
void set_camera_pos(IGameCamera &cam, float x, float y, float z);

/**
  \brief Sets camera position (#Point3 format).

  \param[in] pos new camera position.
  \param[out] cam camera to save position to.

  @see IGameCamera Point3
  */
void set_camera_pos(IGameCamera &cam, const Point3 &pos);
/** @} */

extern void dagor_use_reversed_depth(bool rev = true);
extern bool dagor_is_reversed_depth();
#define DAGOR_FAR_DEPTH (dagor_is_reversed_depth() ? 0.0f : 1.0f)

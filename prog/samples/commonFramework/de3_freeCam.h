#pragma once

/** \addtogroup de3Common
  @{
*/

#include "de3_ICamera.h"

/**
\brief Free camera class.
*/

class IFreeCameraDriver : public IGameCamera
{
public:
  /**
  \brief Scales free camera sensivity.

  \param[in] move_sens_scale move sensivity scale.
  \param[in] rotate_sens_scale rotate sensivity scale.

  */
  virtual void scaleSensitivity(float move_sens_scale, float rotate_sens_scale) = 0;

  static float zNear, zFar;
  static float turboScale;
  static bool moveWithWASD;   // =true by default
  static bool moveWithArrows; // =true by default
  static bool isActive;
  float accelTime;
};

/**
  \brief Creates free camera for WIN32.

  \return Pointer to created free camera.

  @see IFreeCameraDriver create_gamepad_free_camera()
  */
IFreeCameraDriver *create_mskbd_free_camera();

/**
  \brief Creates free camera for XBOX.

  \return Pointer to created free camera.

  @see IFreeCameraDriver create_mskbd_free_camera()
  */
IFreeCameraDriver *create_gamepad_free_camera();

extern bool exit_button_pressed;
/** @} */

#pragma once

/** \addtogroup de3Common
  @{
*/

/**
  \brief Loads splash texture from file.

  \param[in] filename splash file name

  @see show_splash() unload_splash()
  */
void load_splash(const char *filename);

/**
  \brief Draws splash.

  @see load_splash() unload_splash()
  */
void show_splash(bool own_render = true);

/**
  \brief Unloads splash texture.

  @see load_splash() show_splash()
  */
void unload_splash();
/** @} */

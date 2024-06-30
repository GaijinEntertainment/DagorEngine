//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
 * @file This file contains PC-only functions for shader and resource management.
 */
#pragma once

#if !(_TARGET_PC | _TARGET_XBOX)
  !error: must not be included for non-PC target!
#endif

#include <3d/dag_drv3d.h>
#include <generic/dag_tabFwd.h>

class String;

  namespace d3d
  {
#if _TARGET_PC_WIN
  /**
   * @brief Creates vertex program from hlsl shader code.
   * 
   * @param [in]    hlsl_text   Buffer containing shader code.
   * @param [in]    len         (unused, perhaps, hlsl buffer length)
   * @param [in]    entry       Name of the entry point function for the shader.
   * @param [in]    profile     Shader profile name.
   * @param [out]   out_err     A pointer to a string for output on error.
   * @return                    A handle to the created vertex program.
   */
  VPROG create_vertex_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile, String *out_err = NULL);

  /**
   * @brief Creates pixel shader from hlsl shader code.
   *
   * @param [in]    hlsl_text   Buffer containing shader code.
   * @param [in]    len         (unused, perhaps, hlsl buffer length)
   * @param [in]    entry       Name of the entry point function for the shader.
   * @param [in]    profile     Shader profile name.
   * @param [out]   out_err     A pointer to a string for output on error.
   * @return                    A handle to the created pixel shader.
   */
  FSHADER create_pixel_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
    String *out_err = NULL);

  /**
   * @brief Creates compute shader from hlsl shader code.
   *
   * @param [in]    hlsl_text   Buffer containing shader code.
   * @param [in]    len         (unused, perhaps, hlsl buffer length)
   * @param [in]    entry       Name of the entry point function for the shader.
   * @param [in]    profile     Shader profile name.
   * @param [out]   shader_bin  Output buffer for compiled shader bytecode.
   * @param [out]   out_err     A pointer to a string for output on error.
   * @return                    \c true on success, \c false otherwise.
   */
  bool compile_compute_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
    Tab<uint32_t> &shader_bin, String &out_err);
#endif

#if !_TARGET_D3D_MULTI
  /**
   * @brief Retrives vertex declaration of the vertex program.
   * 
   * @param [in] vprog  A handle to the vertex program to retrieve vertex declaration from.
   * @return            A handle to the retrieved vertex declaration.
   */
  VDECL get_program_vdecl(PROGRAM);

  /**
   * @brief Sets vertex program for the current render state.
   * 
   * @param [in] ps A handle to the vertex program to assign.
   * @return        \c true on success, \c false otherwise.
   */
  bool set_vertex_shader(VPROG ps);

  /**
   * @brief Sets pixel shader for the current render state.
   *
   * @param [in] ps A handle to the pixel shader to assign.
   * @return        \c true on success, \c false otherwise.
   */
  bool set_pixel_shader(FSHADER ps);
#if _TARGET_PC_WIN
  namespace pcwin32
  {

  /**
   * @brief Retrieves format of the texture.
   * 
   * @param [in] tex    Texture to retrieve the format of.
   * @return            \c D3DFORMAT texture format.
   */
  unsigned get_texture_format(BaseTexture *tex);

  /**
   * @brief Retrieves format of the texture.
   *
   * @param [in] tex    Texture to retrieve the format of.
   * @return            String storing \c D3DFORMAT texture format.
   */
  const char *get_texture_format_str(BaseTexture *tex);

  //@cond
  //unimplemented
  void *get_native_surface(BaseTexture *tex);
  } // namespace pcwin32
#endif

  VPROG create_vertex_shader_asm(const char *asm_text);
  VPROG create_vertex_shader_dagor(const VPRTYPE *p, int n);

  FSHADER create_pixel_shader_asm(const char *asm_text);
  FSHADER create_pixel_shader_dagor(const FSHTYPE *p, int n);
  //@endcond
#if _TARGET_PC_WIN
  // additional d3d::pcwin32 interface (PC specific)
  namespace pcwin32
  {

  /**
   * @brief Sets the window to present the front buffer.
   * 
   * @param [in] hwnd A handle to the window to assign.
   */
  void set_present_wnd(void *hwnd);

  /**
   * @brief Turns on/off capturing the whole frame buffer instead of only window data when \ref capture_screen is called.
   * 
   * @param [in] ison   Indicates whether the whole frame buffer should be captured instead of only window data.
   * @return            \c true if capturing the whole frame buffer was enabled before the call, \c false otherwise.
   */
  bool set_capture_full_frame_buffer(bool ison); // returns previous state
  }                                              // namespace pcwin32
#endif

  /**
   * @brief Returns the current state of VSync.
   * 
   * @return \c true if VSync is enabled, \c false otherwise.
   */
  bool get_vsync_enabled();
  
  /**
   * @brief Turns on/off strong VSync (flips only on VBLANK). 
   * 
   * @param [in] enable Indicates whether strong VSync should be enabled.
   * @return            \c true on success, \c false otherwise.
   */
  bool enable_vsync(bool enable);
  //! retrieve list of available display modes
  //! 
  /**
   * @brief Retrieves a list of available display modes.
   * 
   * @param [out] list List of available display modes.
   */
  void get_video_modes_list(Tab<String> &list);
#endif
  }; // namespace d3d

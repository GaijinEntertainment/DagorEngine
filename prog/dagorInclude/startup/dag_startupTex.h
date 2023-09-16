//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resId.h>

typedef bool (*user_get_file_data_t)(const char *rel_fn, const char *mount_path, class String &out_fn, int &out_base_ofs,
  bool sync_wait_ready, bool dont_load, const char *src_fn, void *arg);

//! registers file texture factory as the default
void set_default_file_texture_factory();
//! registers symbolic texture factory as the default
void set_default_sym_texture_factory();

//! sets texture name to be used instead of missing textures in texture manager; default="tex/missing"
void set_missing_texture_name(const char *tex_fname, bool replace_bad_shader_textures);
void set_missing_texture_usage(bool replace_missing_tex = true);

//! registers factory for loading .JPG files
void register_jpeg_tex_load_factory();
//! registers factory for loading .TGA files
void register_tga_tex_load_factory();
//! registers factory for loading .TIF and .TIFF files
void register_tiff_tex_load_factory();
//! registers factory for loading .PSD files
void register_psd_tex_load_factory();
//! registers factory for loading .PNG files
void register_png_tex_load_factory();
//! registers factory for loading .avif files
void register_avif_tex_load_factory();
//! registers factory for loading .SVG files
void register_svg_tex_load_factory();
//! registers factory for loading images from files without extension (tries with specified extensions and order)
void register_any_tex_load_factory(const char *try_order = "jpg|jpeg|avif|png|tga");
//! registers factory for loading images from URLs using auxilary callback
void register_url_tex_load_factory(user_get_file_data_t cb_get, void *cb_arg, const char *url = "http|https",
  const char *ext = "jpg|jpeg|avif|png|tga");
//! registers factory for loading lottie animation files
void register_lottie_tex_load_factory(unsigned int atlas_w = 512, unsigned int atlas_h = 512);

//! registers factory for texture creating from files without extension that reside in vromfs
void register_any_vromfs_tex_create_factory(const char *try_order = "ddsx|jpg|jpeg|avif|tga|png");
//! registers factory for texture creating from .DDS files
void register_dds_tex_create_factory();
//! registers factory for texture creating from any loadable image
void register_loadable_tex_create_factory();


//! registers common factores for game
//! (only DDS/DDSx tex creator; default file factory)
void register_common_game_tex_factories();

//! registers common factores for tools
//! (JPG, TGA, any load format; DDS/DDSx, loadable tex creator; default file factory)
void register_common_tool_tex_factories();

//! tries and sets callback for reloading DDSx texture from file
void set_ddsx_reloadable_callback(BaseTexture *t, TEXTUREID tid, const char *fn, int ofs, TEXTUREID bt_ref);

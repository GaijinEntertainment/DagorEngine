//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <osApiWrappers/dag_files.h>
#include <generic/dag_tab.h>
#include <libTools/util/strUtil.h>

// external classes forward declarations
class ITextureNumerator;
class ILogWriterEx;
namespace mkbindump
{
class BinDumpSaveCB;
};


enum ValidateMessageFlags
{
  VM_FILE_NOT_FOUND = 1 << 0,     // file not found
  VM_FILE_NOT_DDS = 1 << 1,       // file is non-DDS texture
  VM_FILE_NOT_HDR = 1 << 2,       // file is non-HDR texture
  VM_FILE_NOT_TEXTURE = 1 << 3,   // file is non-DDS and non-HDR texture
  VM_UNSUPPORTED_FORMAT = 1 << 4, // unsupported texture's format
  VM_WIDTH_NOT_POW2 = 1 << 5,     // width is not in power of 2
  VM_HEIGHT_NOT_POW2 = 1 << 6,    // height is not in power of 2

  VM_CHECK_ALL = VM_FILE_NOT_FOUND | VM_FILE_NOT_TEXTURE | VM_UNSUPPORTED_FORMAT | VM_WIDTH_NOT_POW2 | VM_HEIGHT_NOT_POW2,
};


void load_exp_shaders_for_target(unsigned tc);

// TODO: complete validate_texture function
// bool validate_texture(const char* path, int flags = VM_CHECK_ALL);


// old SCN reading
bool get_scene_vertexes_faces_count(const char *path, int &faces_cnt);
bool check_scene_vertex_count(const char *path, const char *lm_dir, int max_faces);

void add_built_scene_textures(const char *scene_dir, ITextureNumerator &tn);
void store_built_scene_data(const char *scene_dir, mkbindump::BinDumpSaveCB &cwr, const ITextureNumerator &tn);
bool check_built_scene_data(const char *scene_dir, int taget_code);
bool check_built_scene_data_exist(const char *scene_dir, int taget_code);

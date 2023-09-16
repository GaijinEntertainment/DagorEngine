#ifndef __GAIJIN_DDX_SAVE_H__
#define __GAIJIN_DDX_SAVE_H__
#pragma once

#include <libTools/dtx/dtxHeader.h>

// forward declrataions for external classes
class IGenSave;
class IGenLoad;
class String;

namespace ddstexture
{
bool write_to_cb(IGenSave &cwr, const char *filename, Header *header = NULL, bool write_file_size = true);
bool write_cb_to_cb(IGenSave &cwr, IGenLoad &crd, Header *header, int size);

bool write_to_file(IGenLoad &crd, const char *filename, int data_size = -1);

} // namespace ddstexture

#endif

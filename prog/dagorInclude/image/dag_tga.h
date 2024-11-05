//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
class IGenLoad;
struct TexImage32;
struct TexImage8;
struct TexImage;
struct TexImage8a;
struct TexPixel32;
struct TexPixel8a;


// load TGA file
TexImage32 *load_tga32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha = NULL);
bool load_tga32(TexImage32 *im, char *fn, bool *out_used_alpha = NULL);

TexImage32 *load_tga32(const char *fn, IMemAlloc *, bool *out_used_alpha = NULL);
TexImage8a *load_tga8a(const char *fn, IMemAlloc *);
TexImage8 *load_tga8(const char *fn, IMemAlloc *);

// data_len is inout param. if not null then app_data_len is both maximum possible value and will return the actual len
// returns real size of data (even if read less than that) or ~0u on error
// NULL - means won't be read/written
TexImage32 *load_tga32(IGenLoad &crd, IMemAlloc *mem, bool *used_alpha, unsigned char *data, unsigned int *data_len);
bool load_tga32(TexImage32 *im, char *fn, bool *out_used_alpha, unsigned char *data, unsigned int *data_len);
TexImage32 *load_tga32(const char *fn, IMemAlloc *, bool *out_used_alpha, unsigned char *data, unsigned int *data_len);

// save TGA file
int save_tga8(const char *fn, unsigned char *ptr, int wd, int ht, int stride, unsigned char *app_data = 0,
  unsigned int app_data_len = 0);
int save_tga8a(const char *fn, TexPixel8a *ptr, int wd, int ht, int stride, unsigned char *app_data = 0,
  unsigned int app_data_len = 0);
int save_tga32(const char *fn, TexImage32 *, unsigned char *app_data = 0, unsigned int app_data_len = 0);
int save_tga32(const char *fn, TexPixel32 *im, int wd, int ht, int stride, unsigned char *app_data = 0, unsigned int app_data_len = 0);
int save_tga24(const char *fn, TexImage *im, unsigned char *app_data = 0, unsigned int app_data_len = 0);
int save_tga24(const char *fn, char *ptr, int wd, int ht, int stride, unsigned char *app_data = 0, unsigned int app_data_len = 0);

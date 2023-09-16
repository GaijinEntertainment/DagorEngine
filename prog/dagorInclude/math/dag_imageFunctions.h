//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace imagefunctions
{
// faster then SIMD version
extern void exponentiate4_c(const unsigned char *__restrict data, float *__restrict dest, int num, float power);


// slower then SIMD versions
extern void convert_to_linear_c(const float *__restrict data, unsigned char *__restrict dest, int num, float power);
extern void convert_to_float_c(const unsigned char *__restrict data, float *__restrict dest, int num);
extern void convert_from_float_c(const unsigned char *__restrict data, float *__restrict dest, int num);
extern void downsample4x_c(unsigned char *destData, const unsigned char *srcData, int destW, int destH);


// aligned memory
// num_pixels >= 4

// 2 times slower then C version!
extern void exponentiate4_simda(const unsigned char *__restrict data, float *__restrict dest, int num_pixels, float gamma);

// a bit faster then C version
extern void convert_to_linear_simda(const float *__restrict data, unsigned char *__restrict dest, int num_pixels, float invgamma);

extern void convert_to_float_simda(const unsigned char *__restrict data, float *__restrict dest, int num);
extern void convert_from_float_simda(const float *__restrict data, unsigned char *__restrict dest, int num);
//! destW >= 2
extern void downsample4x_simda(unsigned char *destData, const unsigned char *srcData, int destW, int destH);
//! destW >= 2, using global gamma (2.2) to convert to linear space, and then back to gamma space
extern void downsample4x_simda_gamma_correct(unsigned char *destData, const unsigned char *srcData, int destW, int destH);

//! destW >= 1
extern void downsample4x_simda(float *destData, const float *srcData, int destW, int destH);

// unaligned memory
// num_pixels >= 4
// 2.5 times slower then C version!
extern void exponentiate4_simdu(const unsigned char *__restrict data, float *__restrict dest, int num_pixels, float gamma);

// a bit faster then C version
extern void convert_to_linear_simdu(const float *__restrict data, unsigned char *__restrict dest, int num_pixels, float invgamma);

//! destW >= 1
extern void downsample4x_simdu(unsigned char *destData, const unsigned char *srcData, int destW, int destH);
extern void downsample4x_simdu(float *destData, const float *srcData, int destW, int destH);

// prepare_kaiser_kernel prepares kernel for kaiser filtering.
// kernel should be aligned to 16, 12 floats
extern void prepare_kaiser_kernel(float alpha, float stretch, float *kernel);

// allocate temporal images
extern void *kaiser_allocate_tmp_float_image(int max_dest_w, int max_dest_h);
extern void *kaiser_allocate_tmp_char_image(int max_dest_w, int max_dest_h);
// restore temporal images
extern void kaiser_free_tmp_image(void *image);

// kaiser kernel should be 16 aligned memory prepared with prepare_kaiser_kernel
// kaiser function will allocate additional 1/2 of src_image for temp memory
extern void kaiser_downsample(int destW, int destH, const float *src_image, float *dst_image, float *kernel, void *tmp_image);

// 5% faster, then float version.
extern void kaiser_downsample(int destW, int destH, const unsigned char *src_image, unsigned char *dst_image, float *kernel,
  void *tmp_image);

// downsample only one axis, for one-pixel textures
extern void downsample2x_simdu(float *destData, const float *srcData, int destW);
extern void downsample2x_simdu(unsigned char *destData, const unsigned char *srcData, int destW);

}; // namespace imagefunctions

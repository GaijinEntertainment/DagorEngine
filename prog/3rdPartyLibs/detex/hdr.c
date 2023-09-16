/*

Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <fenv.h>

#include "detex.h"
#include "half-float.h"
#include "hdr.h"
#include "misc.h"

// Gamma/HDR parameters.

__thread float detex_gamma = 1.0f;
__thread float detex_gamma_range_min = 0.0f;
__thread float detex_gamma_range_max = 1.0f;
__thread float *detex_gamma_corrected_half_float_table = NULL;
__thread float detex_corrected_half_float_table_gamma;

void detexSetHDRParameters(float gamma, float range_min, float range_max) {
	detex_gamma = gamma;
	detex_gamma_range_min = range_min;
	detex_gamma_range_max = range_max;
	detex_gamma_corrected_half_float_table = NULL;
}

// Update gamma-corrected half-float table when required.
static void ValidateGammaCorrectedHalfFloatTable(float gamma) {
	if (detex_gamma_corrected_half_float_table != NULL &&
	detex_corrected_half_float_table_gamma == gamma)
		return;
	if (detex_gamma_corrected_half_float_table == NULL)
		detex_gamma_corrected_half_float_table = malloc(65536 * sizeof(float));
	float *float_table = detex_gamma_corrected_half_float_table;
	detexValidateHalfFloatTable();
	memcpy(float_table, detex_half_float_table, 65536 * sizeof(float));
	for (int i = 0; i <= 0xFFFF; i++)
		if (float_table[i] >= 0.0f)
			float_table[i] = powf(float_table[i], 1.0f / gamma);
		else
			float_table[i] = - powf(- float_table[i], 1.0f / gamma);
}

static DETEX_INLINE_ONLY void CalculateRangeFloat(float *buffer, int n,
float *range_min_out, float *range_max_out) {
	float range_min = FLT_MAX;
	float range_max = - FLT_MAX;
	for (int i = 0; i < n; i++) {
		float f = buffer[i];
		if (f < range_min)
			range_min = f;
		if (f > range_max)
			range_max = f;
	}
	*range_min_out = range_min;
	*range_max_out = range_max;
}

static DETEX_INLINE_ONLY void CalculateRangeHalfFloat(uint16_t *buffer, int n,
float *range_min_out, float *range_max_out) {
	detexValidateHalfFloatTable();
	float range_min = FLT_MAX;
	float range_max = - FLT_MAX;
	for (int i = 0; i < n; i ++) {
		float f = detexGetFloatFromHalfFloat(buffer[i]);
		if (f < range_min)
			range_min = f;
		if (f > range_max)
			range_max = f;
	}
	*range_min_out = range_min;
	*range_max_out = range_max;
}


bool detexCalculateDynamicRange(uint8_t *pixel_buffer, int nu_pixels, uint32_t pixel_format,
float *range_min_out, float *range_max_out) {
	if (~(pixel_format & DETEX_PIXEL_FORMAT_FLOAT_BIT)) {
		detexSetErrorMessage("detexCalculateDynamicRange: Pixel buffer not in float format");
		return false;
	}
	if (pixel_format & DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT) {
		CalculateRangeHalfFloat((uint16_t *)pixel_buffer,
			nu_pixels * detexGetPixelSize(pixel_format) / 2,
			range_min_out, range_max_out);
		return true;
	}
	else if (pixel_format & DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT) {
		CalculateRangeFloat((float *)pixel_buffer,
			nu_pixels * detexGetPixelSize(pixel_format) / 4,
			range_min_out, range_max_out);
		return true;
	}
	else {
		detexSetErrorMessage("detexCalculateDynamicRange: Unable to handle pixel buffer format");
		return false;
	}
}

// Convert half floats to unsigned 16-bit integers in place with gamma value of 1.
static DETEX_INLINE_ONLY void detexConvertHDRHalfFloatToUInt16Gamma1(uint16_t *buffer, int n) {
	detexValidateHalfFloatTable();
	float range_min = detex_gamma_range_min;
	float range_max = detex_gamma_range_max;
	fesetround(FE_DOWNWARD);
	if (range_min == 0.0f && range_max == 1.0f) {
		for (int i = 0; i < n; i++) {
			float f = detexGetFloatFromHalfFloat(buffer[i]);
			int u = lrintf(detexClamp0To1(f) * 65535.0f + 0.5f);
			buffer[i] = (uint16_t)u;
		}
		return;
	}
	float factor = 1.0f / (range_max - range_min);
	for (int i = 0; i < n; i++) {
		float f = detexGetFloatFromHalfFloat(buffer[i]);
		int u = lrintf(detexClamp0To1((f - range_min) * factor) * 65535.0f + 0.5f);
		buffer[i] = (uint16_t)u;
	}
}

static DETEX_INLINE_ONLY void detexConvertHDRHalfFloatToUInt16SpecialGamma(uint16_t *buffer, int n) {
	float gamma = detex_gamma;
	float range_min = detex_gamma_range_min;
	float range_max = detex_gamma_range_max;
	ValidateGammaCorrectedHalfFloatTable(gamma);
	float *corrected_half_float_table = detex_gamma_corrected_half_float_table;
	float corrected_range_min, corrected_range_max;
	if (range_min >= 0.0f)
		corrected_range_min = powf(range_min, 1.0f / gamma);
	else
		corrected_range_min = - powf(- range_min, 1.0f / gamma);
	if (range_max >= 0.0f)
		corrected_range_max = powf(range_max, 1.0f / gamma);
	else
		corrected_range_max = - powf(- range_max, 1.0f / gamma);
	float factor = 1.0f / (corrected_range_max - corrected_range_min);
	for (int i = 0; i < n; i++) {
		float f = corrected_half_float_table[buffer[i]];
		int u = lrintf(detexClamp0To1((f - corrected_range_min) * factor) * 65535.0f + 0.5f);
		buffer[i] = (uint16_t)u;
	}
}

void detexConvertHDRHalfFloatToUInt16(uint16_t *buffer, int n) {
	if (detex_gamma == 1.0f)
		detexConvertHDRHalfFloatToUInt16Gamma1(buffer, n);
	else
		detexConvertHDRHalfFloatToUInt16SpecialGamma(buffer, n);
}

static DETEX_INLINE_ONLY void detexConvertHDRFloatToFloatGamma1(float *buffer, int n) {
	float range_min = detex_gamma_range_min;
	float range_max = detex_gamma_range_max;
	fesetround(FE_DOWNWARD);
	if (range_min == 0.0f && range_max == 1.0f) {
		for (int i = 0; i < n; i++) {
			float f = buffer[i];
			buffer[i] = detexClamp0To1(f);
		}
		return;
	}
	float factor = 1.0f / (range_max - range_min);
	for (int i = 0; i < n; i++) {
		float f = buffer[i];
		buffer[i] = detexClamp0To1((f - range_min) * factor);
	}
}

static DETEX_INLINE_ONLY void detexConvertHDRFloatToFloatSpecialGamma(float *buffer, int n) {
	float gamma = detex_gamma;
	float range_min = detex_gamma_range_min;
	float range_max = detex_gamma_range_max;
	float corrected_range_min, corrected_range_max;
	if (range_min >= 0.0f)
		corrected_range_min = powf(range_min, 1.0f / gamma);
	else
		corrected_range_min = - powf(- range_min, 1.0f / gamma);
	if (range_max >= 0.0f)
		corrected_range_max = powf(range_max, 1.0f / gamma);
	else
		corrected_range_max = - powf(- range_max, 1.0f / gamma);
	float factor = 1.0f / (corrected_range_max - corrected_range_min);
	for (int i = 0; i < n; i++) {
		float f = buffer[i];
		buffer[i] = detexClamp0To1((f - corrected_range_min) * factor);
	}
}

void detexConvertHDRFloatToFloat(float *buffer, int n) {
	if (detex_gamma == 1.0f)
		detexConvertHDRFloatToFloatGamma1(buffer, n);
	else
		detexConvertHDRFloatToFloatSpecialGamma(buffer, n);
}


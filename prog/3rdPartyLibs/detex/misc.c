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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>

#include "detex.h"

// Generate bit mask from bit0 to bit1 (inclusive).
static DETEX_INLINE_ONLY uint64_t GenerateMask(int bit0, int bit1) {
	return (((uint64_t)1 << (bit1 + 1)) - 1) ^
		(((uint64_t)1 << bit0) - 1);
}

/* Return the component bitfield masks for a pixel format (pixel size must be at most 64 bits). */
DETEX_API bool detexGetComponentMasks(uint32_t pixel_format, uint64_t *red_mask_out, uint64_t *green_mask_out,
uint64_t *blue_mask_out, uint64_t *alpha_mask_out) {
	if (detexGetPixelSize(pixel_format) > 64)
		return false;
	int component_size = detexGetComponentSize(pixel_format) * 8;
	int nu_components = detexGetNumberOfComponents(pixel_format);
	uint64_t red_mask, green_mask, blue_mask, alpha_mask;
	red_mask = 0;
	green_mask = 0;
	blue_mask = 0;
	alpha_mask = 0;
	if (nu_components == 1 && (pixel_format & DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT)) {
		alpha_mask = GenerateMask(0, component_size - 1);
		goto end;
	}
	red_mask = GenerateMask(0, component_size - 1);
	if (nu_components > 1) {
		green_mask = GenerateMask(component_size, component_size * 2 - 1);
		if (nu_components > 2) {
			blue_mask = GenerateMask(component_size * 2, component_size * 3 - 1);
			if (nu_components > 3)
				alpha_mask = GenerateMask(component_size * 3, component_size * 4 - 1);
		}
	}
	if (pixel_format & DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT) {
		// Swap red and blue.
		uint64_t temp = red_mask;
		red_mask = blue_mask;
		blue_mask = temp;
	}
end:
	*red_mask_out = red_mask;
	*green_mask_out = green_mask;
	*blue_mask_out = blue_mask;
	*alpha_mask_out = alpha_mask;
	return true;
}

// Error handling.

static __thread char *detex_error_message = NULL;

void detexSetErrorMessage(const char *format, ...) {
	if (detex_error_message != NULL)
		free(detex_error_message);
	va_list args;
	va_start(args, format);
	char *message;
	// Allocate and set message.
	int r = vasprintf(&message, format, args);
	if (r < 0)
		message = strdup("detexSetErrorMessage: vasprintf returned error");
	va_end(args);
	detex_error_message = message;
}

const char *detexGetErrorMessage() {
	return detex_error_message;
}

// General texture file loading.

// Load texture file (type autodetected from extension) with mipmaps.
bool detexLoadTextureFileWithMipmaps(const char *filename, int max_mipmaps, detexTexture ***textures_out,
int *nu_levels_out) {
	int filename_length = strlen(filename);
	if (filename_length > 4 && strncasecmp(filename + filename_length - 4, ".ktx", 4) == 0)
		return detexLoadKTXFileWithMipmaps(filename, max_mipmaps, textures_out, nu_levels_out);
	else if (filename_length > 4 && strncasecmp(filename + filename_length - 4, ".dds", 4) == 0)
		return detexLoadDDSFileWithMipmaps(filename, max_mipmaps, textures_out, nu_levels_out);
	else {
		detexSetErrorMessage("detexLoadTextureFileWithMipmaps: Do not recognize filename extension");
		return false;
	}
}

// Load texture file (type autodetected from extension).
bool detexLoadTextureFile(const char *filename, detexTexture **texture_out) {
	int nu_mipmaps;
	detexTexture **textures;
	bool r = detexLoadTextureFileWithMipmaps(filename, 1, &textures, &nu_mipmaps);
	if (!r)
		return false;
	*texture_out = textures[0];
	free(textures);
	return true;
}


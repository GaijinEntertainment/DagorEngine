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
#include <stdio.h>
#include <string.h>

#include "detex.h"
#include "file-info.h"
#include "misc.h"

// Load texture from raw file (first mip-map only) given the format and dimensions
// in texture. Returns true if successful.
// The texture->data is allocated, free with free().
bool detexLoadRawFile(const char *filename, detexTexture *texture) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		detexSetErrorMessage("detexLoadRawFile: Could not open file %s", filename);
		return false;
	}
	size_t size;
	if (detexFormatIsCompressed(texture->format))
		size = detexGetCompressedBlockSize(texture->format) * texture->width_in_blocks *
			texture->height_in_blocks;
	else
		size = detexGetPixelSize(texture->format) * texture->width * texture->height;
	texture->data = (uint8_t *)malloc(size);
	if (fread(texture->data, 1, size, f) < size) {
		free(texture->data);
		detexSetErrorMessage("detexLoadRawFile: Error reading file %s", filename);
		return false;
	}
	fclose(f);
	return true;
}

// Save texture to raw file (first mip-map only) given the format and dimensions
// in texture. Returns true if successful.
bool detexSaveRawFile(detexTexture *texture, const char *filename) {
	FILE *f = fopen(filename, "wb");
	if (f == NULL) {
		detexSetErrorMessage("detexSaveRawFile: Could not open file %s for writing", filename);
		return false;
	}
	size_t size;
	if (detexFormatIsCompressed(texture->format))
		size = detexGetCompressedBlockSize(texture->format) * texture->width_in_blocks *
			texture->height_in_blocks;
	else
		size = detexGetPixelSize(texture->format) * texture->width * texture->height;
	if (fwrite(texture->data, 1, size, f) < size) {
		free(texture->data);
		detexSetErrorMessage("detexSaveRawFile: Error writing to file %s", filename);
		return false;
	}
	fclose(f);
	return true;
}



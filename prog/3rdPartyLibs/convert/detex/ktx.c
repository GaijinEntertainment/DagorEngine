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

static const uint8_t ktx_id[12] = {
	0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

// Load texture from KTX file with mip-maps. Returns true if successful.
// nu_mipmaps is a return parameter that returns the number of mipmap levels found.
// textures_out is a return parameter for an array of detexTexture pointers that is allocated,
// free with free(). textures_out[i] are allocated textures corresponding to each level, free
// with free();
bool detexLoadKTXFileWithMipmaps(const char *filename, int max_mipmaps, detexTexture ***textures_out,
int *nu_levels_out) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Could not open file %s", filename);
		return false;
	}
	int header[16];
	size_t s = fread(header, 1, 64, f);
	if (s != 64) {
		detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Error reading file %s", filename);
		return false;
	}
	if (memcmp(header, ktx_id, 12) != 0) {
		// KTX signature not found.
		detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Couldn't find KTX signature");
		return false;
	}
	int wrong_endian = 0;
	if (header[3] == 0x01020304) {
		// Wrong endian .ktx file.
		wrong_endian = 1;
		for (int i = 3; i < 16; i++) {
			uint8_t *b = (uint8_t *)&header[i];
			uint8_t temp = b[0];
			b[0] = b[3];
			b[3] = temp;
			temp = b[1];
			b[1] = b[2];
			b[2] = temp;
		}
	}
	int glType = header[4];
	int glFormat = header[6];
	int glInternalFormat = header[7];
//	int pixel_depth = header[11];
	const detexTextureFileInfo *info = detexLookupKTXFileInfo(glInternalFormat, glFormat, glType);
	if (info == NULL) {
		detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Unsupported format in .ktx file "
			"(glInternalFormat = 0x%04X)", glInternalFormat);
		return false;
	}
	int bytes_per_block;
	if (detexFormatIsCompressed(info->texture_format))
		bytes_per_block = detexGetCompressedBlockSize(info->texture_format);
	else
		bytes_per_block = detexGetPixelSize(info->texture_format);
	int block_width = info->block_width;
	int block_height = info->block_height;
//	printf("File is %s texture.\n", info->text1);
	int width = header[9];
	int height = header[10];
	int extended_width = ((width + block_width - 1) / block_width) * block_width;
	int extended_height = ((height + block_height - 1) / block_height) * block_height;
	int nu_file_mipmaps = header[14];
//	if (nu_file_mipmaps > 1 && max_mipmaps == 1) {
//		detexSetErrorMessage("Disregarding mipmaps beyond the first level.\n");
//	}
	int nu_mipmaps;
	if (nu_file_mipmaps > max_mipmaps)
		nu_mipmaps = max_mipmaps;
	else
		nu_mipmaps = nu_file_mipmaps;
 	if (header[15] > 0) {
		// Skip metadata.
		uint8_t *metadata = (unsigned char *)malloc(header[15]);
		if (fread(metadata, 1, header[15], f) < header[15]) {
			detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Error reading file %s", filename);
			return false;
		}
		free(metadata);
	}
	detexTexture **textures = (detexTexture **)malloc(sizeof(detexTexture *) * nu_mipmaps);
	for (int i = 0; i < nu_mipmaps; i++) {
		uint32_t image_size_buffer[1];
		size_t r = fread(image_size_buffer, 1, 4, f);
		if (r != 4) {
			for (int j = 0; j < i; j++)
				free(textures[j]);
			free(textures);
			detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Error reading file %s", filename);
			return false;
		}
		if (wrong_endian) {
			uint8_t *image_size_bytep = (uint8_t *)&image_size_buffer[0];
			unsigned char temp = image_size_buffer[0];
			image_size_bytep[0] = image_size_bytep[3];
			image_size_bytep[3] = temp;
			temp = image_size_bytep[1];
			image_size_bytep[1] = image_size_bytep[2];
			image_size_bytep[2] = temp;
		}
		int image_size = image_size_buffer[0];
		int n = (extended_height / block_height) * (extended_width / block_width);
		if (image_size != n * bytes_per_block) {
			for (int j = 0; j < i; j++)
				free(textures[j]);
			free(textures);
			detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Error loading file %s: "
				"Image size field of mipmap level %d does not match (%d vs %d)",
				filename, i, image_size, n * bytes_per_block);
			return false;
		}
		// Allocate texture.
		textures[i] = (detexTexture *)malloc(sizeof(detexTexture));
		textures[i]->format = info->texture_format;
		textures[i]->data = (uint8_t *)malloc(n * bytes_per_block);
		textures[i]->width = width;
		textures[i]->height = height;
		textures[i]->width_in_blocks = extended_width / block_width;
		textures[i]->height_in_blocks = extended_height / block_height;
		if (fread(textures[i]->data, 1, n * bytes_per_block, f) < n * bytes_per_block) {
			for (int j = 0; j <= i; j++)
				free(textures[j]);
			free(textures);
			detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Error reading file %s", filename);
			return false;
		}
		// Divide by two for the next mipmap level, rounding down.
		width >>= 1;
		height >>= 1;
		extended_width = ((width + block_width - 1) / block_width) * block_width;
		extended_height = ((height + block_height - 1) / block_height) * block_height;
		// Read mipPadding. But not if we have already read everything specified.
		char buffer[4];
		if (i + 1 < nu_mipmaps) {
			int nu_bytes = 3 - ((image_size + 3) % 4);
			if (fread(buffer, 1, nu_bytes, f) != nu_bytes) {
				for (int j = 0; j <= i; j++)
					free(textures[j]);
				free(textures);
				detexSetErrorMessage("detexLoadKTXFileWithMipmaps: Error reading file %s", filename);
				return false;
			}
		}
	}
	fclose(f);
	*nu_levels_out = nu_mipmaps;
	*textures_out = textures;
	return true;
}

// Load texture from KTX file (first mip-map only). Returns true if successful.
// The texture is allocated, free with free().
bool detexLoadKTXFile(const char *filename, detexTexture **texture_out) {
	int nu_mipmaps;
	detexTexture **textures;
	bool r = detexLoadKTXFileWithMipmaps(filename, 1, &textures, &nu_mipmaps);
	if (!r)
		return false;
	*texture_out = textures[0];
	free(textures);
	return true;
}

enum {
	DETEX_ORIENTATION_DOWN = 1,
	DETEX_ORIENTATION_UP = 2
};

static const char ktx_orientation_key_down[24] = {
	'K', 'T', 'X', 'o', 'r', 'i', 'e', 'n', 't', 'a', 't', 'i', 'o', 'n', 0,
	'S', '=', 'r', ',', 'T', '=', 'd', 0, 0		// Includes one byte of padding.
};

static const char ktx_orientation_key_up[24] = {
	'K', 'T', 'X', 'o', 'r', 'i', 'e', 'n', 't', 'a', 't', 'i', 'o', 'n', 0,
	'S', '=', 'r', ',', 'T', '=', 'u', 0, 0		// Includes one byte of padding.
};

// Save textures to KTX file (multiple mip-maps levels). Return true if succesful.
bool detexSaveKTXFileWithMipmaps(detexTexture **textures, int nu_levels, const char *filename) {
	FILE *f = fopen(filename, "wb");
	if (f == NULL) {
		detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Could not open file %s for writing", filename);
		return false;
	}
	uint32_t header[16];
	memset(header, 0, 64);
	memcpy(header, ktx_id, 12);	// Set id.
	header[3] = 0x04030201;
	const detexTextureFileInfo *info = detexLookupTextureFormatFileInfo(textures[0]->format);
	if (info == NULL) {
		detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Could not match texture format with file format");
		return false;
	}
	if (!info->ktx_support) {
		detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Could not match texture format with KTX file format");
		return false;
	}
	int glType = 0;
	int glTypeSize = 1;
	int glFormat = 0;
	glType = info->gl_type;
	glFormat = info->gl_format;
	int glInternalFormat = info->gl_internal_format;
	header[4] = glType;			// glType
	header[5] = glTypeSize;			// glTypeSize
	header[6] = glFormat;			// glFormat
	header[7] = glInternalFormat;
	header[9] = textures[0]->width;
	header[10] = textures[0]->height;
	header[11] = 0;
	header[13] = 1;				// Number of faces.
	header[14] = nu_levels;			// Mipmap levels.
	int data[1];
	const int option_orientation = 0;
	if (option_orientation == 0) {
		header[15] = 0;
		size_t r = fwrite(header, 1, 64, f);
		if (r != 64) {
			detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", filename);
			return false;
		}
	}
	else {
		header[15] = 28;	// Key value data bytes.
		size_t r = fwrite(header, 1, 64, f);
		if (r != 64) {
			detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", filename);
			return false;
		}
		data[0] = 27;		// Key and value size.
		r = fwrite(data, 1, 4, f);
		if (r != 4) {
			detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", filename);
			return false;
		}
		if (option_orientation == DETEX_ORIENTATION_DOWN)
			r = fwrite(ktx_orientation_key_down, 1, 24, f);
		else
			r = fwrite(ktx_orientation_key_up, 1, 24, f);
		if (r != 24) {
			detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", filename);
			return false;
		}
	}
	for (int i = 0; i < nu_levels; i++) {
		uint32_t pixel_size = detexGetPixelSize(textures[i]->format);
		// Block size is block size for compressed textures and the pixel size for
		// uncompressed textures.
		int n;
		int block_size;
		if (detexFormatIsCompressed(textures[i]->format)) {
			n = textures[i]->width_in_blocks * textures[i]->height_in_blocks;
			block_size = detexGetCompressedBlockSize(textures[i]->format);
		}
		else {
			n = textures[i]->width * textures[i]->height;
			block_size = pixel_size;
		}
//		if (!option_quiet)
//			printf("Writing mipmap level %d of size %d x %d.\n", i, textures[i]->width, textures[i]->height);
		// Because of per row 32-bit alignment is mandated by the KTX specification, we have to handle
		// special cases of unaligned uncompressed textures.
		if (detexFormatIsCompressed(textures[i]->format) || (pixel_size & 3) == 0) {
			// Regular 32-bit aligned texture.
			data[0] = n * block_size;	// Image size.
			size_t r1 = fwrite(data, 1, 4, f);
			size_t r2 = fwrite(textures[i]->data, 1, n * block_size, f);
			if (r1 != 4 || r2 != n * block_size) {
				detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", filename);
				return false;
			}
		}
		else {
			// Uncompressed texture with pixel size that is not a multiple of four.
			int row_size = (textures[i]->width * pixel_size  + 3) & (~3);
			data[0] = textures[i]->height * row_size;	// Image size.
			size_t r1 = fwrite(data, 1, 4, f);
			if (r1 != 4) {
				detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", filename);
				return false;
			}
			uint8_t *row = (uint8_t *)malloc(row_size);
			for (int y = 0; y < textures[i]->height; y++) {
				memcpy(row, &textures[i]->data[y * textures[i]->width * pixel_size],
					textures[i]->width * pixel_size);
				for (int j = textures[i]->width * pixel_size; j < row_size; j++)
					row[j] = 0;
				size_t r2 = fwrite(row, 1, row_size, f);
				if (r2 != row_size) {
					detexSetErrorMessage("detexSaveKTXFileWithMipmaps: Error writing to file %s", 							filename);
					return false;
				}
			}
			free(row);
		}
	}
	fclose(f);
	return true;
}

// Save texture to KTX file (single mip-map level). Returns true if succesful.
bool detexSaveKTXFile(detexTexture *texture, const char *filename) {
	detexTexture *textures[1];
	textures[0] = texture;
	return detexSaveKTXFileWithMipmaps(textures, 1, filename);
}


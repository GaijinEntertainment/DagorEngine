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
#include <strings.h>

#include "detex.h"
#include "file-info.h"
#include "misc.h"

/*
	The TextureInfo structure has the following fields:

	int type;			// The texture type. Various properties of the texture type can be derived from bits in the texture type.
	int ktx_support;		// Whether the program supports loading/saving this format in ktx files.
	int dds_support;		// Whether the program supports loading/saving this format in dds files.
	const char *text1;		// A short text identifier, also used with the --format option.
	const char *text2;		// An alternative identifier.
	int block_width;		// The block width (1 for uncompressed textures).
	int block_height;		// The block height (1 for uncompressed textures).
	int gl_internal_format;		// The OpenGL glInternalFormat identifier for this texture type.
	int gl_format;			// The matching glFormat.
	int gl_type;			// The matching glType.
	const char *dx_four_cc;		// The DDS file four character code matching this texture type. If "DX10", dx10_format is valid.
	int dx10_format;		// The DX10 format identifier matching this texture type.

	There is also a synonym table for KTX and DDS file formats with alternative IDs. When loading a texture file
	the synonyms will be recognized and treated as the corresponding texture type in the primary table. When
	saving a file the format used will be that of the primary table.
*/

static const detexTextureFileInfo texture_info[] = {
//	  Texture format			KTX/DDS	Textual representations		Block
//						support					size 	OpenGL ID in KTX files		DDS file IDs
//												internalFormat, format, type	FourCC, DX10 format
// Uncompressed formats (texture format = pixel format).
	{ DETEX_PIXEL_FORMAT_RGB8,		1, 1,	"RGB8", "",			1, 1,	0x1907, 0x1907,	0x1401,		"", 0 },
	{ DETEX_PIXEL_FORMAT_RGBA8,		1, 1,	"RGBA8", "",			1, 1, 	0x1908, 0x1908, 0x1401,		"DX10", 28 },
	{ DETEX_PIXEL_FORMAT_R8,		1, 1,	"R8", "",			1, 1,	0x8229, 0x1903, 0x1401,		"DX10", 61 },
	{ DETEX_PIXEL_FORMAT_SIGNED_R8,		1, 1,	"SIGNED_R8", "",		1, 1,	0x8F49, 0x1903, 0x1400,		"DX10", 63 },
	{ DETEX_PIXEL_FORMAT_RG8,		1, 1,	"RG8", "",			1, 1,	0x822B, 0x8227, 0x1401,		"DX10", 49 },
	{ DETEX_PIXEL_FORMAT_SIGNED_RG8,	1, 1,	"SIGNED_RG8", "",		1, 1,	0x8F95, 0x8227, 0x1400,		"DX10", 51 },
	{ DETEX_PIXEL_FORMAT_R16,		1, 1,	"R16", "",			1, 1,	0x822A, 0x1903,	0x1403,		"DX10", 56 },
	{ DETEX_PIXEL_FORMAT_SIGNED_R16,	1, 1,	"SIGNED_R16", "",		1, 1,	0x8F98, 0x1903,	0x1402,		"DX10", 58 },
	{ DETEX_PIXEL_FORMAT_RG16,		1, 1,	"RG16", "",			1, 1,	0x8226, 0x8227,	0x1403,		"DX10", 35 },
	{ DETEX_PIXEL_FORMAT_SIGNED_RG16,	1, 1,	"SIGNED_RG16", "",		1, 1,	0x8F99, 0x8227,	0x1402,		"DX10", 37 },
	{ DETEX_PIXEL_FORMAT_RGB16,		1, 0,	"RGB16", "",			1, 1,	0x8054, 0x1907,	0x1403,		"", 0 },
	{ DETEX_PIXEL_FORMAT_RGBA16,		1, 1,	"RGBA16", "",			1, 1,	0x805B, 0x8227,	0x1403,		"DX10", 11 },
	{ DETEX_PIXEL_FORMAT_FLOAT_R16,		1, 1,	"FLOAT_R16", "",		1, 1,	0x822D, 0x1903,	0x140B,		"DX10", 54 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RG16,	1, 1,	"FLOAT_RG16", "",		1, 1,	0x822F, 0x8227,	0x140B,		"DX10", 34 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGB16,	1, 0,	"FLOAT_RGB16", "",		1, 1,	0x1907, 0x1907, 0x140B,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGBA16,	1, 1,	"FLOAT_RGBA16", "",		1, 1,	0x1908,	0x1908,	0x140B,		"DX10", 10 },
	{ DETEX_PIXEL_FORMAT_FLOAT_R32,		1, 1,	"FLOAT_R32", "",		1, 1,   0x822E,	0x1903, 0x1406,		"DX10", 41 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RG32,	1, 1,	"FLOAT_RG32", "",		1, 1,   0x8230,	0x8227,	0x1406,		"DX10", 16 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGB32,	1, 1,	"FLOAT_RGB32", "",		1, 1,   0x8815,	0x1907,	0x1406,		"DX10", 6 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGBA32,	1, 1,	"FLOAT_RGBA32", "",		1, 1,   0x8814,	0x1908,	0x1406,		"DX10", 2 },
	{ DETEX_PIXEL_FORMAT_A8,		1, 1,	"A8", "",			1, 1,	0x1906, 0x1906, 0x1401,		"DX10", 65 },
// Compressed formats.
	{ DETEX_TEXTURE_FORMAT_BC1,		1, 1,	"BC1", "DXT1",			4, 4,	0x83F0, 0,	0,		"DXT1",	0 },
	{ DETEX_TEXTURE_FORMAT_BC1A,		1, 1,	"BC1A",	"DXT1A",		4, 4,	0x83F1, 0,	0, 		"", 0 },
	{ DETEX_TEXTURE_FORMAT_BC2,		1, 1,	"BC2", "DXT3",			4, 4,	0x83F2, 0,	0,		"DXT3", 0 },
 	{ DETEX_TEXTURE_FORMAT_BC3,		1, 1,	"BC3", "DXT5",			4, 4,	0x83F3, 0,	0,		"DXT5", 0 },
	{ DETEX_TEXTURE_FORMAT_RGTC1,		1, 1,	"RGTC1", "BC4_UNORM",		4, 4,	0x8DBB, 0,	0,		"DX10", 80 },
	{ DETEX_TEXTURE_FORMAT_SIGNED_RGTC1,	1, 1,	"SIGNED_RGTC1", "BC4_SNORM",	4, 4,	0x8DBC, 0,	0,		"DX10", 81 },
	{ DETEX_TEXTURE_FORMAT_RGTC2,		1, 1,	"RGTC2", "BC5_UNORM",		4, 4,	0x8DBD, 0,	0,		"DX10", 83 },
	{ DETEX_TEXTURE_FORMAT_SIGNED_RGTC2,	1, 1,	"SIGNED_RGTC2", "BC5_SNORM",	4, 4,	0x8DBE, 0,	0,		"DX10", 84 },
	{ DETEX_TEXTURE_FORMAT_BPTC_FLOAT,	1, 1,	"BPTC_FLOAT", "BC6H_UF16",	4, 4,	0x8E8F, 0,	0,		"DX10", 95 },
	{ DETEX_TEXTURE_FORMAT_BPTC_SIGNED_FLOAT, 1, 1,	"BPTC_SIGNED_FLOAT", "BC6H_SF16", 4, 4,	0x8E8E, 0,	0,		"DX10", 96 },
	{ DETEX_TEXTURE_FORMAT_BPTC,		1, 1,	"BPTC", "BC7",			4, 4,	0x8E8C, 0,	0,		"DX10",	98 },
	{ DETEX_TEXTURE_FORMAT_ETC1,		1, 0,	"ETC1", "",			4, 4,	0x8D64, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_ETC2,		1, 0,	"ETC2", "ETC2_RGB8",		4, 4,	0x9274, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_ETC2_PUNCHTHROUGH, 1, 0,	"ETC2_PUNCHTHROUGH", "",	4, 4,	0x9275, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_ETC2_EAC,	1, 0,	"ETC2_EAC", "EAC",		4, 4,	0x9278, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_EAC_R11,		1, 0, 	"EAC_R11", "",			4, 4,	0x9270, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_EAC_SIGNED_R11,	1, 0,	"EAC_SIGNED_R11", "",		4, 4,	0x9271, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_EAC_RG11,	1, 0,	"EAC_RG11", "",			4, 4,	0x9272, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_EAC_SIGNED_RG11,	1, 0,	"EAC_SIGNED_RG11", "",		4, 4,	0x9273, 0,	0,		"", 0 },
//	{ DETEX_TEXTURE_FORMAT_ETC2_SRGB8,	1, 0,	"SRGB_ETC2", "",		4, 4,	0x9275, 0,	0,		"", 0 },
//	{ DETEX_TEXTURE_FORMAT_ETC2_SRGB_EAC,	1, 0,	"SRGB_ETC2_EAC", "",		4, 4,	0x9279, 0,	0,		"", 0 },
//	{ DETEX_TEXTURE_FORMAT_ETC2_SRGB_PUNCHTHROUGH, 1, 0, "SRGB_ETC2_PUNCHTHROUGH, "", 4, 4,	0x9277, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_ASTC_4X4,	1, 0,	"ASTC_4x4", "",			4, 4,	0x93B0, 0,	0,		"DX10", 134 },
#if 0
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_5X4,			1, 0,	"astc_5x4", "",			5, 4,	0x93B1, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_5X5,			1, 0,	"astc_5x5", "",			5, 5,	0x93B2, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_6X5,			1, 0,	"astc_6x4", "",			6, 5,	0x93B3, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_6X6,			1, 0,	"astc_6x6", "",			6, 6,	0x93B4, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_8X5,			1, 0,	"astc_8x5", "",			8, 5,	0x93B5, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_8X6,			1, 0, 	"astc_8x6", "",			8, 6,	0x93B6, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_8X8,			1, 0,	"astc_8x8", "",			8, 8,	0x93B7, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_10X5,			1, 0,	"astc_10x5", "",		10, 5,	0x93B8, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_10X6,			1, 0,	"astc_10x6", "",		10, 6,	0x93B9, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_10X8,			1, 0,	"astc_10x8", "",		10, 8,	0x93BA, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_10X10,			1, 0,	"astc_10x10", "",		10, 10,	0x93BB, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_12X10,			1, 0,	"astc_12x10", "",		12, 10,	0x93BC, 0,	0,		"", 0 },
	{ DETEX_TEXTURE_FORMAT_RGBA_ASTC_12X12,			1, 0,	"astc_12x12", "",		12, 12,	0x93BD, 0,	0,		"", 0 },
#endif
// Pseudo-formats (not present in files, but used for name look-up).
	{ DETEX_PIXEL_FORMAT_RGBX8,		0, 0,	"RGBX8", "",			1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_BGRX8,		0, 0,	"BGRX8", "",			1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGBX16,	0, 0,	"FLOAT_RGBX16", "",		1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_BGRX16,	0, 0,	"FLOAT_BGRX16", "",		1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_R16_HDR,	0, 0,	"FLOAT_R16_HDR", "",		1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RG16_HDR,	0, 0,	"FLOAT_RG16_HDR", "",		1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGB16_HDR,	0, 0,	"FLOAT_RGB16_HDR", "",		1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGBA16_HDR,	0, 0,	"FLOAT_RGBA16_HDR", "",		1, 1,	0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_R32_HDR,	0, 0,	"FLOAT_R32_HDR", "",		1, 1,   0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RG32_HDR,	0, 0,	"FLOAT_RG32_HDR", "",		1, 1,   0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGB32_HDR,	0, 0,	"FLOAT_RGB32_HDR", "",		1, 1,   0,	0,	0,		"", 0 },
	{ DETEX_PIXEL_FORMAT_FLOAT_RGBA32_HDR,	0, 0,	"FLOAT_RGBA32_HDR", "",		1, 1,   0,	0,	0,		"", 0 },
};

#define DETEX_NU_TEXTURE_INFO_ENTRIES (sizeof(texture_info) / sizeof(texture_info[0]))

typedef struct {
	int texture_format;
	int gl_internal_format;
	int gl_format;
	int gl_type;
} OpenGLTextureFormatSynonym;


static OpenGLTextureFormatSynonym open_gl_synonym[] = {
	{ DETEX_PIXEL_FORMAT_RGB8, 0x8051, 0x1907, 0x1401 },	// GL_RGB8
	{ DETEX_PIXEL_FORMAT_RGBA8, 0x8058, 0x1908, 0x1401 },	// GL_RGBA8
	{ DETEX_PIXEL_FORMAT_FLOAT_RGB16, 0x881B, 0x1907, 0x140B }, // GL_RGB16F
	{ DETEX_PIXEL_FORMAT_FLOAT_RGBA16, 0x881A, 0x1908, 0x140B }, // GL_RGAB16F
	{ DETEX_PIXEL_FORMAT_A8, 0x803C, 0x1906, 0x1401 }, // GL_ALPHA8
	{ DETEX_TEXTURE_FORMAT_RGTC1, 0x8C70, 0, 0 },		// LATC1
	{ DETEX_TEXTURE_FORMAT_SIGNED_RGTC1, 0x8C71, 0, 0 },	// SIGNED_LATC1
	{ DETEX_TEXTURE_FORMAT_RGTC2, 0x8C72, 0, 0 },		// LATC1
	{ DETEX_TEXTURE_FORMAT_SIGNED_RGTC2, 0x8C73, 0, 0 },	// SIGNED_LATC1
};

#define DETEX_NU_OPEN_GL_SYNONYMS (sizeof(open_gl_synonym) / sizeof(open_gl_synonym[0]))

typedef struct {
	int texture_format;
	const char *dx10_four_cc;
	int dx10_format;
} DDSTextureFormatSynonym;

#define NU_DDS_SYNONYMS 22

static const DDSTextureFormatSynonym dds_synonym[] = {
	{ DETEX_PIXEL_FORMAT_RGBA8, "DX10", 27 },
	{ DETEX_PIXEL_FORMAT_RGBA8, "DX10", 30 },
	{ DETEX_PIXEL_FORMAT_RG16, "DX10", 36 },
	{ DETEX_PIXEL_FORMAT_R16, "DX10", 57 },
	{ DETEX_PIXEL_FORMAT_SIGNED_RG16, "DX10", 38 },
	{ DETEX_PIXEL_FORMAT_SIGNED_R16, "DX10", 59 }, 
	{ DETEX_PIXEL_FORMAT_RG8, "DX10", 50 },
	{ DETEX_PIXEL_FORMAT_R8, "DX10", 62 },
	{ DETEX_PIXEL_FORMAT_SIGNED_RG8, "DX10", 52 },
	{ DETEX_PIXEL_FORMAT_SIGNED_R8, "DX10", 64 },
	{ DETEX_PIXEL_FORMAT_RGBA16, "DX10", 12 },
	{ DETEX_TEXTURE_FORMAT_BC1, "DX10", 70 },
	{ DETEX_TEXTURE_FORMAT_BC1, "DX10", 71 },
	{ DETEX_TEXTURE_FORMAT_BC2, "DX10", 73 },
	{ DETEX_TEXTURE_FORMAT_BC2, "DX10", 74 },
	{ DETEX_TEXTURE_FORMAT_BC3, "DX10", 76 },
	{ DETEX_TEXTURE_FORMAT_BC3, "DX10", 77 },
        { DETEX_TEXTURE_FORMAT_RGTC1, "DX10", 79 },
	{ DETEX_TEXTURE_FORMAT_RGTC1, "BC4U", 0 },
	{ DETEX_TEXTURE_FORMAT_SIGNED_RGTC1, "BC4S", 0 },
        { DETEX_TEXTURE_FORMAT_RGTC2, "DX10", 82 },
	{ DETEX_TEXTURE_FORMAT_SIGNED_RGTC2, "BC5S", 0 },
	{ DETEX_TEXTURE_FORMAT_BPTC, "DX10", 97 },
	{ DETEX_TEXTURE_FORMAT_BPTC_FLOAT, "DX10", 94 },
	{ DETEX_TEXTURE_FORMAT_RGTC1, "ATI1", 0 },
	{ DETEX_TEXTURE_FORMAT_RGTC2, "ATI2", 0 }
};

#define DETEX_NU_DDS_SYNONYMS (sizeof(dds_synonym) / sizeof(dds_synonym[0]))

// Look-up texture file info for texture format.
const detexTextureFileInfo *detexLookupTextureFormatFileInfo(uint32_t texture_format) {
	for (int i = 0; i < DETEX_NU_TEXTURE_INFO_ENTRIES; i++)
		if (texture_info[i].texture_format == texture_format)
			return &texture_info[i];
	return NULL;
}

// Look-up texture file info for texture description.
const detexTextureFileInfo *detexLookupTextureDescription(const char *s) {
	for (int i = 0; i < DETEX_NU_TEXTURE_INFO_ENTRIES; i++)
		if (strcasecmp(texture_info[i].text1, s) == 0 || strcasecmp(texture_info[i].text2, s) == 0)
			return &texture_info[i];
	return NULL;
}

// Look-up texture file info for KTX file format based on GL format parameters.
const detexTextureFileInfo *detexLookupKTXFileInfo(int gl_internal_format, int gl_format, int gl_type) {
	for (int i = 0; i < DETEX_NU_TEXTURE_INFO_ENTRIES; i++)
		if (texture_info[i].gl_internal_format != 0 && texture_info[i].gl_internal_format == gl_internal_format) {
			if (texture_info[i].gl_format == 0)
				return &texture_info[i];
			if (texture_info[i].gl_format == gl_format && texture_info[i].gl_type == gl_type)
				return &texture_info[i];
		}
	for (int i = 0; i < DETEX_NU_OPEN_GL_SYNONYMS; i++)
		if (open_gl_synonym[i].gl_internal_format == gl_internal_format) {
			if (open_gl_synonym[i].gl_format == 0)
				return detexLookupTextureFormatFileInfo(open_gl_synonym[i].texture_format);
			if (open_gl_synonym[i].gl_format == gl_format && open_gl_synonym[i].gl_type == gl_type)
				return detexLookupTextureFormatFileInfo(open_gl_synonym[i].texture_format);
		}
	return NULL;
}

enum {
	DDPF_ALPHAPIXELS = 0x1,
	DDPF_ALPHA = 0x2,
	DDPF_RGB = 0x40,
};

// Look-up texture file info for DDS file format based on DX format parameters.
const detexTextureFileInfo *detexLookupDDSFileInfo(const char *four_cc, int dx10_format,
uint32_t pixel_format_flags, int bitcount, uint32_t red_mask, uint32_t green_mask,
uint32_t blue_mask, uint32_t alpha_mask) {
	for (int i = 0; i < DETEX_NU_TEXTURE_INFO_ENTRIES; i++)
		if (strncmp(four_cc, "DX10", 4) == 0) {
			if (texture_info[i].dx10_format == dx10_format)
				return &texture_info[i];
		}
		else
			if (texture_info[i].dx_four_cc[0] != '\0' &&
			strncmp(texture_info[i].dx_four_cc, four_cc, 4) == 0)
				return &texture_info[i];
			else {
				uint32_t format = texture_info[i].texture_format;
				if ((pixel_format_flags & DDPF_RGB) &&
				!detexFormatIsCompressed(format)) {
					// Uncompressed format. Match component masks.
					if (bitcount <= 32) {
						int format_bitcount = detexGetPixelSize(format) * 8;
						uint64_t format_red_mask, format_green_mask, format_blue_mask,
							format_alpha_mask;
						detexGetComponentMasks(format, &format_red_mask, &format_green_mask,
							&format_blue_mask, &format_alpha_mask);
						if (format_bitcount == bitcount &&
						format_red_mask == red_mask &&
						format_green_mask == green_mask &&
						format_blue_mask == blue_mask &&
						((pixel_format_flags & DDPF_ALPHAPIXELS) == 0 ||
						format_alpha_mask == alpha_mask))
							return &texture_info[i];
					}
				}
				// Detect old alpha format.
				if ((pixel_format_flags & DDPF_ALPHA) && bitcount == 8 &&
				format == DETEX_PIXEL_FORMAT_A8)
					return &texture_info[i];
			}
	for (int i = 0; i < DETEX_NU_DDS_SYNONYMS; i++)
		if (strncmp(four_cc, "DX10", 4) == 0) {
			if (dds_synonym[i].dx10_format == dx10_format)
				return detexLookupTextureFormatFileInfo(dds_synonym[i].texture_format);
		}
		else if (dds_synonym[i].dx10_four_cc[0] != '\0' &&
		strncmp(dds_synonym[i].dx10_four_cc, four_cc, 4) == 0)
			return detexLookupTextureFormatFileInfo(dds_synonym[i].texture_format);
	return NULL;
}

// Return a description of the texture format.
const char *detexGetTextureFormatText(uint32_t texture_format) {
	const detexTextureFileInfo *info;
	info = detexLookupTextureFormatFileInfo(texture_format);
	if (info == NULL) {
//		printf("Error -- invalid texture format.\n");
		return "Invalid";
	}
	return info->text1;
}

// Return alternative description of the texture format.
const char *detexGetAlternativeTextureFormatText(uint32_t texture_format) {
	const detexTextureFileInfo *info;
	info = detexLookupTextureFormatFileInfo(texture_format);
	if (info == NULL) {
		return "Invalid";
	}
	return info->text2;
}

/* Return OpenGL Texture2D/KTX file parameters for a texture format. */
bool detexGetOpenGLParameters(uint32_t texture_format, int *gl_internal_format,
uint32_t *gl_format, uint32_t *gl_type) {
	const detexTextureFileInfo *info = detexLookupTextureFormatFileInfo(texture_format);
	if (info == NULL) {
		detexSetErrorMessage("detexGetOpenGLParameters: Invalid texture format");
		return false;
	}
	*gl_internal_format = info->gl_internal_format;
	*gl_format = info->gl_format;
	*gl_type = info->gl_type;
	return true;
}

/* Return DirectX 10 format for a texture format. */
bool detexGetDX10Parameters(uint32_t texture_format, uint32_t *dx10_format) {
	const detexTextureFileInfo *info = detexLookupTextureFormatFileInfo(texture_format);
	if (info == NULL) {
		detexSetErrorMessage("detexGetDX10Parameters: Invalid texture format");
		return false;
	}
	if (strncmp(info->dx_four_cc, "DX10", 4) != 0) {
		detexSetErrorMessage("detexGetDX10Parameters: No DX10 format for texture format");
		return false;
	}
	*dx10_format = info->dx10_format;
	return true;
}


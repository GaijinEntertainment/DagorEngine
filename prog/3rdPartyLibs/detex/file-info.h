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

typedef struct {
	uint32_t texture_format;
	int ktx_support;
	int dds_support;
	const char *text1;
	const char *text2;
	int block_width;		// The block width (1 for uncompressed textures).
	int block_height;		// The block height (1 for uncompressed textures).
	int gl_internal_format;
	uint32_t gl_format;
	uint32_t gl_type;
	const char *dx_four_cc;
	int dx10_format;
} detexTextureFileInfo;

// Look-up texture file info for texture format.
const detexTextureFileInfo *detexLookupTextureFormatFileInfo(uint32_t texture_format);

// Look-up texture file info for texture description.
const detexTextureFileInfo *detexLookupTextureDescription(const char *s);

// Look-up texture file info for KTX file format based on GL format parameters.
const detexTextureFileInfo *detexLookupKTXFileInfo(int gl_internal_format, int gl_format, int gl_type);

// Look-up texture file info for DDS file format based on DX format parameters.
const detexTextureFileInfo *detexLookupDDSFileInfo(const char *four_cc, int dx10_format, uint32_t pixel_format_flags, int bitcount, uint32_t red_mask, uint32_t green_mask, uint32_t blue_mask, uint32_t alpha_mask);


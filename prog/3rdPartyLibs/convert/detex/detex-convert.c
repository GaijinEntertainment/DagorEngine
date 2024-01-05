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

/* Texture file converter. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <getopt.h>

#include "detex.h"
#include "detex-png.h"

static uint32_t input_format;
static uint32_t output_format;
static uint32_t option_flags;
static char *input_file;
static char *output_file;
static int output_file_type;

static const uint32_t supported_formats[] = {
	// Uncompressed formats.
	DETEX_PIXEL_FORMAT_RGB8,
	DETEX_PIXEL_FORMAT_RGBA8,
	DETEX_PIXEL_FORMAT_R8,
	DETEX_PIXEL_FORMAT_SIGNED_R8,
	DETEX_PIXEL_FORMAT_RG8,
	DETEX_PIXEL_FORMAT_SIGNED_RG8,
	DETEX_PIXEL_FORMAT_R16,
	DETEX_PIXEL_FORMAT_SIGNED_R16,
	DETEX_PIXEL_FORMAT_RG16,
	DETEX_PIXEL_FORMAT_SIGNED_RG16,
	DETEX_PIXEL_FORMAT_RGB16,
	DETEX_PIXEL_FORMAT_RGBA16,
	DETEX_PIXEL_FORMAT_FLOAT_R16,
	DETEX_PIXEL_FORMAT_FLOAT_RG16,
	DETEX_PIXEL_FORMAT_FLOAT_RGB16,
	DETEX_PIXEL_FORMAT_FLOAT_RGBA16,
	DETEX_PIXEL_FORMAT_FLOAT_R32,
	DETEX_PIXEL_FORMAT_FLOAT_RG32,
	DETEX_PIXEL_FORMAT_FLOAT_RGB32,
	DETEX_PIXEL_FORMAT_FLOAT_RGBA32,
	DETEX_PIXEL_FORMAT_A8,
	// Compressed formats.
	DETEX_TEXTURE_FORMAT_BC1,
	DETEX_TEXTURE_FORMAT_BC1A,
	DETEX_TEXTURE_FORMAT_BC2,
	DETEX_TEXTURE_FORMAT_BC3,
	DETEX_TEXTURE_FORMAT_RGTC1,
	DETEX_TEXTURE_FORMAT_SIGNED_RGTC1,
	DETEX_TEXTURE_FORMAT_RGTC2,
	DETEX_TEXTURE_FORMAT_SIGNED_RGTC2,
	DETEX_TEXTURE_FORMAT_BPTC_FLOAT,
	DETEX_TEXTURE_FORMAT_BPTC_SIGNED_FLOAT,
	DETEX_TEXTURE_FORMAT_BPTC,
	DETEX_TEXTURE_FORMAT_ETC1,
	DETEX_TEXTURE_FORMAT_ETC2,
	DETEX_TEXTURE_FORMAT_ETC2_PUNCHTHROUGH,
	DETEX_TEXTURE_FORMAT_ETC2_EAC,
	DETEX_TEXTURE_FORMAT_EAC_R11,
	DETEX_TEXTURE_FORMAT_EAC_SIGNED_R11,
	DETEX_TEXTURE_FORMAT_EAC_RG11,
	DETEX_TEXTURE_FORMAT_EAC_SIGNED_RG11,
};

#define NU_SUPPORTED_FORMATS (sizeof(supported_formats) / sizeof(supported_formats[0]))

enum {
	OPTION_FLAG_OUTPUT_FORMAT = 0x1,
	OPTION_FLAG_INPUT_FORMAT = 0x2,
	OPTION_FLAG_DECOMPRESS = 0x4,
	OPTION_FLAG_QUIET = 0x8,
};

static const struct option long_options[] = {
	// Option name, argument flag, NULL, equivalent short option character.
	{ "format", required_argument, NULL, 'f' },
	{ "output-format", required_argument, NULL, 'o' },
	{ "input-format", required_argument, NULL, 'i' },
	{ "decompress", no_argument, NULL, 'd' },
	{ "quiet", no_argument, NULL, 'q' },
	{ NULL, 0, NULL, 0 }
};

#define NU_OPTIONS (sizeof(long_options) / sizeof(long_options[0]))

enum {
	FILE_TYPE_NONE = 0,
	FILE_TYPE_KTX = 1,
	FILE_TYPE_DDS = 2,
	FILE_TYPE_RAW = 3,
	FILE_TYPE_PNG = 4
};

static void Message(const char *format, ...) {
	if (option_flags & OPTION_FLAG_QUIET)
		return;
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

static __attribute ((noreturn)) void FatalError(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	exit(1);
}

static void Usage() {
	Message("detex-convert %s\n", DETEX_VERSION);
	Message("Convert and decompress uncompressed and compressed texture files (KTX, DDS, raw)\n");
	Message("Usage: detex-convert [<OPTIONS>] <INPUTFILE> <OUTPUTFILE>\n");
	Message("Options:\n");
	for (int i = 0;; i++) {
		if (long_options[i].name == NULL)
			break;
		const char *value_str = " <VALUE>";
		if (long_options[i].has_arg)
			Message("    -%c%s, --%s%s, --%s=%s\n", long_options[i].val, value_str,
				long_options[i].name, value_str, long_options[i].name, &value_str[1]);
		else
			Message("    -%c, --%s\n", long_options[i].val, long_options[i].name);
	}
	Message("File formats supported: KTX, DDS, raw (no header), PNG\n");
	Message("Supported formats:\n");
	int column = 0;
	for (int i = 0; i < NU_SUPPORTED_FORMATS; i++) {
		const char *format_text1 = detexGetTextureFormatText(supported_formats[i]);
		const char *format_text2 = detexGetAlternativeTextureFormatText(supported_formats[i]);
		int length1 = strlen(format_text1);
		int length2 = strlen(format_text2);
		int length = length1;
		if (length2 > 0)
			length += 3 + length2;	
		if (column + length > 78) {
			Message("\n");
			column = 0;
		}
		Message("%s", format_text1);
		if (length2 > 0)
			Message(" (%s)", format_text2);
		column += length;
		if (i < NU_SUPPORTED_FORMATS - 1) {
			Message(", ");
			column += 2;
		}
	}
	Message("\n");
}

static uint32_t ParseFormat(const char *s) {
	for (int i = 0; i < NU_SUPPORTED_FORMATS; i++) {
		const char *format_text1 = detexGetTextureFormatText(supported_formats[i]);
		if (strcasecmp(s, format_text1) == 0)
			return supported_formats[i];
		const char *format_text2 = detexGetAlternativeTextureFormatText(supported_formats[i]);
		if (strlen(format_text2) > 0 && strcasecmp(s, format_text2) == 0)
			return supported_formats[i];
	}
	FatalError("Fatal error: Format %s not recognized\n" ,s);
}

static void ParseArguments(int argc, char **argv) {
	option_flags = 0;
	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "f:o:i:q", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'f' :	// -f, --format
		case 'o' :	// -o, --target-format
			output_format = ParseFormat(optarg);
			option_flags |= OPTION_FLAG_OUTPUT_FORMAT;
			break;
		case 'i' :	// -i, --input-format
			input_format = ParseFormat(optarg);
			option_flags |= OPTION_FLAG_INPUT_FORMAT;
			break;
		case 'd' :	// -d, --decompress
			option_flags |= OPTION_FLAG_DECOMPRESS;
			break;
		case 'q' :	// -q, --quiet
			option_flags |= OPTION_FLAG_QUIET;
			break;
		default :
			FatalError("");
			break;
		}
	}

	if (optind + 1 >= argc)
		FatalError("Fatal error: Expected input and output filename arguments\n");
	input_file = strdup(argv[optind]);
	output_file = strdup(argv[optind + 1]);
}

static int DetermineFileType(const char *filename) {
	int filename_length = strlen(filename);
	if (filename_length > 4 && strncasecmp(filename + filename_length - 4, ".ktx", 4) == 0)
		return FILE_TYPE_KTX;
	else if (filename_length > 4 && strncasecmp(filename + filename_length - 4, ".dds", 4) == 0)
		return FILE_TYPE_DDS;
	else if (filename_length > 4 && strncasecmp(filename + filename_length - 4, ".raw", 4) == 0)
		return FILE_TYPE_RAW;
	else if (filename_length > 4 && strncasecmp(filename + filename_length - 4, ".png", 4) == 0)
		return FILE_TYPE_PNG;
	else
		return FILE_TYPE_NONE;
}

int main(int argc, char **argv) {
	if (argc == 1) {
		Usage();
		exit(0);
	}
	ParseArguments(argc, argv);
	Message("detex-convert %s\n", DETEX_VERSION);

	detexTexture **input_textures;
	int nu_levels;
	int input_file_type = DetermineFileType(input_file);
	output_file_type = DetermineFileType(output_file);
	if (input_file_type == FILE_TYPE_KTX || input_file_type == FILE_TYPE_DDS) {
		bool r = detexLoadTextureFileWithMipmaps(input_file, 32, &input_textures, &nu_levels);
		if (!r)
			FatalError("%s\n", detexGetErrorMessage());
	}
	else if (input_file_type == FILE_TYPE_PNG) {
		detexTexture *texture;
		bool r = detexLoadPNGFile(input_file, &texture);
		if (!r)
			FatalError("");
		input_textures = (detexTexture **)malloc(sizeof(detexTexture *) * 1);
		input_textures[0] = texture;
		nu_levels = 1;
	}
	else if (input_file_type == FILE_TYPE_RAW)
		FatalError("Cannot handle RAW type input texture file\n");
	else
		FatalError("Input file extension not recognized\n");

	char s[80];
	if (option_flags & OPTION_FLAG_INPUT_FORMAT) {
		sprintf(s, "%s (specified)", detexGetTextureFormatText(input_format));
	}
	else {
		sprintf(s, "%s (detected)", detexGetTextureFormatText(input_textures[0]->format));
		input_format = input_textures[0]->format;
	}
	Message("Input file: %s, format %s\n", input_file, s);
	if (option_flags & OPTION_FLAG_OUTPUT_FORMAT) {
		sprintf(s, "%s (specified)", detexGetTextureFormatText(output_format));
	}
	else if ((option_flags & OPTION_FLAG_DECOMPRESS)
	|| (detexFormatIsCompressed(input_textures[0]->format) &&
	output_file_type == FILE_TYPE_PNG)) {
		if (!detexFormatIsCompressed(input_textures[0]->format))
			FatalError("Cannot decompress uncompressed texture\n");
		output_format = detexGetPixelFormat(input_textures[0]->format);
		// Decompression of compressed textures can result in a pixel format with an unused component,
		// which is not supported by KTX and DDS texture formats.
		if (output_format == DETEX_PIXEL_FORMAT_RGBX8)
			output_format = DETEX_PIXEL_FORMAT_RGB8;
		else if (output_format == DETEX_PIXEL_FORMAT_FLOAT_RGBX16)
			output_format = DETEX_PIXEL_FORMAT_FLOAT_RGB16;
		sprintf(s, "%s (decompressed input)", detexGetTextureFormatText(output_format));
	}
	else {
		sprintf(s, "%s (taken from input)", detexGetTextureFormatText(input_format));
		output_format = input_format;
	}
	Message("Output file: %s, format %s\n", output_file, s);

	detexTexture **output_textures;
	if (output_format == input_format) {
		output_textures = input_textures;
	}
	else {
		if (detexFormatIsCompressed(output_format))
			FatalError("Cannot convert to output format %s (detex-convert does not support compression)\n",
				detexGetTextureFormatText(output_format));
		output_textures = (detexTexture **)malloc(sizeof(detexTexture *) * nu_levels);
		for (int i = 0; i < nu_levels; i++) {
			uint32_t size;
			size = detexGetPixelSize(output_format) * input_textures[i]->width *
				input_textures[i]->height;
			output_textures[i] = (detexTexture *)malloc(sizeof(detexTexture));
			output_textures[i]->data = (uint8_t *)malloc(size); 
			bool r = detexDecompressTextureLinear(input_textures[i], output_textures[i]->data,
				output_format);
			if (!r)
				FatalError("%s\n", detexGetErrorMessage());
			output_textures[i]->format = output_format;
			output_textures[i]->width = input_textures[i]->width;
			output_textures[i]->height = input_textures[i]->height;
			output_textures[i]->width_in_blocks = input_textures[i]->width;
			output_textures[i]->height_in_blocks = input_textures[i]->height;
		}
//		FatalError("Conversion not yet implemented.\n");
	}

	switch (output_file_type) {
	case FILE_TYPE_KTX : {
		bool r = detexSaveKTXFileWithMipmaps(output_textures, nu_levels, output_file);
		if (!r)
			FatalError("%s\n", detexGetErrorMessage());
		break;
		}
	case FILE_TYPE_DDS : {
		bool r = detexSaveDDSFileWithMipmaps(output_textures, nu_levels, output_file);
		if (!r)
			FatalError("%s\n", detexGetErrorMessage());
		break;
		}
	case FILE_TYPE_RAW :
		if (nu_levels == 1) {
			bool r = detexSaveRawFile(output_textures[0], output_file);
			if (!r)
				FatalError("%s\n", detexGetErrorMessage());
		}
		else
			FatalError("Cannot write to RAW format with more than one mipmap level\n");
		break;
	case FILE_TYPE_PNG : {
		if (nu_levels > 1)
			Message("Saving only first mipmap level of %d levels", nu_levels);
		bool r = detexSavePNGFile(output_textures[0], output_file);
		if (!r)
			FatalError("");
		break;
		}
	case FILE_TYPE_NONE :
		FatalError("Do not recognize output file type\n");
	}

	exit(0);
}


/*
    texture.c -- part of texgenpack, a texture compressor using fgen.

    texgenpack -- a genetic algorithm texture compressor.
    Copyright 2013 Harm Hanemaaijer

    This file is part of texgenpack.

    texgenpack is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    texgenpack is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with texgenpack.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"
#ifndef __GNUC__
#include "strcasecmp.h"
#endif

/*

	The TextureInfo structure has the following fields:

	int type;			// The texture type. Various properties of the texture type can be derived from bits in the texture type.
	int ktx_support;		// Whether the program supports loading/saving this format in ktx files.
	int dds_support;		// Whether the program supports loading/saving this format in dds files.
	const char *text1;		// A short text identifier, also used with the --format option.
	const char *text2;		// An alternative identifier.
	int block_width;		// The block width (1 for uncompressed textures).
	int block_height;		// The block height (1 for uncompressed textures).
	int bits_per_block;		// The number of bits per block for compressed textures, the number of bits per pixel for uncompressed textures.
	int internal_bits_per_block;	// The number of bits per block as stored internally, the number of internal bits per pixel for uncompressed textures.
	int alpha_bits;			// The number of alpha bits per pixel in the format.
	int nu_components;		// The number of pixel components including alpha.
	int gl_internal_format;		// The OpenGL glInternalFormat identifier for this texture type.
	int gl_format;			// The matching glFormat.
	int gl_type;			// The matching glType.
	const char *dx_four_cc;		// The DDS file four character code matching this texture type. If "DX10", dx10_format is valid.
	int dx10_format;		// The DX10 format identifier matching this texture type.
	uint64_t red_mask, green_mask, blue_mask, alpha_mask;	// The bitmasks of the pixel components.

	There is also a synonym table for KTX and DDS file formats with alternative IDs. When loading a texture file
	the synonyms will be recognized and treated as the corresponding texture type in the primary table. When
	saving a file the format used will be that of the primary table.

	For uncompressed textures with 8-bit components, the internal texture storage is 32-bit. For uncompressed
        textures with half-float components, the internal texture storage is 64-bit, irrespective of the number of components.
	For other uncompressed textures with 16-bit components, the internal texture storage is 32-bit.
*/


#define NU_ITEMS 51

static TextureInfo texture_info[NU_ITEMS] = {
//	  Texture type					Support	text1, text2			Block size    Comp.	OpenGL ID in KTX files		DDS file IDs		Masks
//															internalFormat, format, type	FourCC, DX10 format	red, green, blue, alpha
	{ TEXTURE_TYPE_UNCOMPRESSED_RGB8,		1, 1,	"rgb8", "",			1, 1, 24, 32, 0, 3, 	0x1907, 0x1907,	0x1401,		"", 0, 		0xFF, 0xFF00, 0xFF0000, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RGBA8,		1, 1,	"rgba8", "",			1, 1, 32, 32, 8, 4,	0x1908, 0x1908, 0x1401,		"DX10", 28,	0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_UNCOMPRESSED_ARGB8,		0, 1,	"argb8", "",			1, 1, 32, 32, 8, 4,	0,	0,	0,		"", 0,		0xFF00, 0xFF0000, 0xFF000000, 0xFF },
	{ TEXTURE_TYPE_UNCOMPRESSED_RGB_HALF_FLOAT,	1, 0,	"rgb_half_float", "",		1, 1, 48, 64, 0, 3,	0x1907, 0x1907, 0x140B,		"", 0,		0xFFFF, 0xFFFF0000, 0xFFFF00000000, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RGBA_HALF_FLOAT,	1, 1,	"rgba_half_float", "",		1, 1, 64, 64, 16, 4,	0x1908, 0x1908, 0x140B,		"DX10", 10,	0xFFFF, 0xFFFF0000, 0xFFFF00000000, 0xFFFF000000000000 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RG16,		1, 1,	"rg16", "",			1, 1, 32, 32, 0, 2,	0x8226, 0x8227,	0x1403,		"DX10", 35,	0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG16,	1, 1,	"signed_rg16", "",		1, 1, 32, 32, 0, 2,	0x8F99, 0x8227,	0x1402,		"DX10", 37,	0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RG_HALF_FLOAT,	1, 1,	"rg_half_float", "",		1, 1, 32, 64, 0, 2,	0x822F, 0x8227,	0x140B,		"DX10", 34,	0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RG8,		1, 1,	"rg8", "",			1, 1, 16, 32, 0, 2,	0x822B, 0x8227, 0x1401,		"DX10", 49,	0xFF, 0xFF00, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG8,		1, 1,	"signed_rg8", "",		1, 1, 16, 32, 0, 2,	0x8F95, 0x8227, 0x1400,		"DX10", 51,	0xFF, 0xFF00, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_R16,		1, 1,	"r16", "",			1, 1, 16, 32, 0, 1,	0x822A, 0x1903,	0x1403,		"DX10", 56,	0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R16,		1, 1,	"signed_r16", "",		1, 1, 16, 32, 0, 1,	0x8F98, 0x1903,	0x1402,		"DX10", 58,	0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_R_HALF_FLOAT,	1, 1,	"r_half_float", "",		1, 1, 16, 64, 0, 1,	0x822D, 0x1903,	0x140B,		"DX10", 54,	0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_R8,			1, 1,	"r8", "",			1, 1, 8, 32, 0, 1,	0x8229, 0x1903, 0x1401,		"DX10", 61,	0xFF, 0, 0, 0 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R8,		1, 1,	"signed_r8", "",		1, 1, 8, 32, 0, 1,	0x8F49, 0x1903, 0x1400,		"DX10", 63,	0xFF, 0, 0, 0 },
	{ TEXTURE_TYPE_ETC1,				1, 0,	"etc1", "",			4, 4, 64, 64, 0, 3,	0x8D64, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0},
	{ TEXTURE_TYPE_ETC2_RGB8,			1, 0,	"etc2_rgb8", "etc2",		4, 4, 64, 64, 0, 3,	0x9274, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0},
	{ TEXTURE_TYPE_ETC2_EAC,			1, 0,	"etc2_eac", "eac",		4, 4, 128, 128, 8, 4,	0x9278, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_ETC2_PUNCHTHROUGH,		1, 0,	"etc2_punchthrough", "",	4, 4, 64, 64, 1, 4,	0x9275, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_R11_EAC,				1, 0, 	"r11_eac", "",			4, 4, 64, 64, 0, 1,	0x9270, 0,	0,		"", 0,		0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_RG11_EAC,			1, 0,	"rg11_eac", "",			4, 4, 128, 128, 0, 2,	0x9272, 0,	0,		"", 0,		0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_SIGNED_R11_EAC,			1, 0,	"signed_r11_eac", "",		4, 4, 64, 64, 0, 1,	0x9271, 0,	0,		"", 0,		0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_SIGNED_RG11_EAC,			1, 0,	"signed_rg11_eac", "",		4, 4, 128, 128, 0, 2,	0x9273, 0,	0,		"", 0,		0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_ETC2_SRGB8,			1, 0,	"etc2_srgb8", "",		4, 4, 64, 64, 0, 3,	0x9275, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0},
	{ TEXTURE_TYPE_ETC2_SRGB_EAC,			1, 0,	"etc2_sgrb_eac", "",		4, 4, 128, 128, 8, 3,	0x9279, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_ETC2_SRGB_PUNCHTHROUGH,		1, 0,	"etc2_sgrb_punchthrough", "",	4, 4, 64, 64, 1, 3,	0x9277, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_DXT1,				1, 1,	"dxt1", "bc1",			4, 4, 64, 64, 0, 3,	0x83F0, 0,	0,		"DXT1",	0,	0xFF, 0xFF00, 0xFF0000, 0},
	{ TEXTURE_TYPE_DXT1A,				1, 1,	"dxt1a", "bc1a",		4, 4, 64, 64, 1, 4,	0x83F1, 0,	0, 		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_DXT3,				1, 1,	"dxt3", "bc2",			4, 4, 128, 128, 8, 4, 	0x83F2, 0,	0,		"DXT3", 0,	0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
 	{ TEXTURE_TYPE_DXT5,				1, 1,	"dxt5", "bc3",			4, 4, 128, 128, 8, 4,	0x83F3, 0,	0,		"DXT5", 0,	0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_BPTC,				1, 1,	"bptc", "bc7",			4, 4, 128, 128, 8, 4,	0x8E8C, 0,	0,		"DX10",	98,	0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_BPTC_FLOAT,			1, 1,	"bptc_float", "bc6h_uf16",	4, 4, 128, 128, 0, 3,	0x8E8F, 0,	0,		"DX10", 95,	0xFFFF, 0xFFFF0000, 0xFFFF00000000, 0 },
	{ TEXTURE_TYPE_BPTC_SIGNED_FLOAT,		1, 1,	"bptc_signed_float", "bc6h_sf16", 4, 4, 128, 128, 0, 3,	0x8E8E, 0,	0,		"DX10", 96,	0xFFFF, 0xFFFF0000, 0xFFFF00000000, 0 },
	{ TEXTURE_TYPE_RGTC1,				1, 1,	"rgtc1", "bc4_unorm",		4, 4, 64, 64, 0, 1,	0x8DBB, 0,	0,		"DX10", 80,	0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_SIGNED_RGTC1,			1, 1,	"signed_rgtc1", "bc4_snorm",	4, 4, 64, 64, 0, 1,	0x8DBC, 0,	0,		"DX10", 81,	0xFFFF, 0, 0, 0 },
	{ TEXTURE_TYPE_RGTC2,				1, 1,	"rgtc2", "bc5_unorm",		4, 4, 128, 128, 0, 2, 	0x8DBD, 0,	0,		"DX10", 83,	0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_SIGNED_RGTC2,			1, 1,	"signed_rgtc2", "bc5_snorm",	4, 4, 128, 128, 0, 2, 	0x8DBE, 0,	0,		"DX10", 84,	0xFFFF, 0xFFFF0000, 0, 0 },
	{ TEXTURE_TYPE_RGBA_ASTC_4X4,			1, 0,	"astc_4x4", "",			4, 4, 128, 128, 8, 4,	0x93B0, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_5X4,			1, 0,	"astc_5x4", "",			5, 4, 128, 128, 8, 4, 	0x93B1, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_5X5,			1, 0,	"astc_5x5", "",			5, 5, 128, 128, 8, 4, 	0x93B2, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_6X5,			1, 0,	"astc_6x4", "",			6, 5, 128, 128, 8, 4,	0x93B3, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_6X6,			1, 0,	"astc_6x6", "",			6, 6, 128, 128, 8, 4, 	0x93B4, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_8X5,			1, 0,	"astc_8x5", "",			8, 5, 128, 128, 8, 4, 	0x93B5, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_8X6,			1, 0, 	"astc_8x6", "",			8, 6, 128, 128, 8, 4, 	0x93B6, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_8X8,			1, 0,	"astc_8x8", "",			8, 8, 128, 128, 8, 4,	0x93B7, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_10X5,			1, 0,	"astc_10x5", "",		10, 5, 128, 128, 8, 4,	0x93B8, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_10X6,			1, 0,	"astc_10x6", "",		10, 6, 128, 128, 8, 4, 	0x93B9, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_10X8,			1, 0,	"astc_10x8", "",		10, 8, 128, 128, 8, 4, 	0x93BA, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_10X10,			1, 0,	"astc_10x10", "",		10, 10, 128, 128, 8, 4,	0x93BB, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_12X10,			1, 0,	"astc_12x10", "",		12, 10, 128, 128, 8, 4,	0x93BC, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
	{ TEXTURE_TYPE_RGBA_ASTC_12X12,			1, 0,	"astc_12x12", "",		12, 12, 128, 128, 8, 4,	0x93BD, 0,	0,		"", 0,		0xFF, 0xFF00, 0xFF0000, 0xFF000000 },
};

typedef struct {
	int type;
	int gl_internal_format;
	int gl_format;
	int gl_type;
} OpenGLTextureFormatSynonym;

#define NU_OPEN_GL_SYNONYMS 6

static OpenGLTextureFormatSynonym open_gl_synonym[NU_OPEN_GL_SYNONYMS] = {
	{ TEXTURE_TYPE_UNCOMPRESSED_RGB8, 0x8051, 0x1907, 0x1401 },	// GL_RGB8
	{ TEXTURE_TYPE_UNCOMPRESSED_RGBA8, 0x8058, 0x1908, 0x1401 },	// GL_RGBA8
	{ TEXTURE_TYPE_RGTC1, 0x8C70, 0, 0 },		// LATC1
	{ TEXTURE_TYPE_SIGNED_RGTC1, 0x8C71, 0, 0 },	// SIGNED_LATC1
	{ TEXTURE_TYPE_RGTC2, 0x8C72, 0, 0 },		// LATC1
	{ TEXTURE_TYPE_SIGNED_RGTC2, 0x8C73, 0, 0 },	// SIGNED_LATC1
};

typedef struct {
	int type;
	const char *dx10_four_cc;
	int dx10_format;
} DDSTextureFormatSynonym;

#define NU_DDS_SYNONYMS 20

static DDSTextureFormatSynonym dds_synonym[NU_DDS_SYNONYMS] = {
	{ TEXTURE_TYPE_UNCOMPRESSED_RGBA8, "DX10", 27 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RGBA8, "DX10", 30 },
	{ TEXTURE_TYPE_UNCOMPRESSED_RG16, "DX10", 36 },
	{ TEXTURE_TYPE_UNCOMPRESSED_R16, "DX10", 57 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG16, "DX10", 38 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R16, "DX10", 59 }, 
	{ TEXTURE_TYPE_UNCOMPRESSED_RG8, "DX10", 50 },
	{ TEXTURE_TYPE_UNCOMPRESSED_R8, "DX10", 62 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG8, "DX10", 52 },
	{ TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R8, "DX10", 64 }, 
	{ TEXTURE_TYPE_DXT1, "DX10", 70 },
	{ TEXTURE_TYPE_DXT1, "DX10", 71 },
	{ TEXTURE_TYPE_DXT3, "DX10", 73 },
	{ TEXTURE_TYPE_DXT3, "DX10", 74 },
	{ TEXTURE_TYPE_DXT5, "DX10", 76 },
	{ TEXTURE_TYPE_DXT5, "DX10", 77 },
	{ TEXTURE_TYPE_BPTC, "DX10", 97 },
	{ TEXTURE_TYPE_BPTC_FLOAT, "DX10", 94 },
	{ TEXTURE_TYPE_RGTC1, "ATI1", 0 },
	{ TEXTURE_TYPE_RGTC2, "ATI2", 0 }
};

TextureInfo *match_texture_type(int type) {
	for (int i = 0; i < NU_ITEMS; i++)
		if (texture_info[i].type == type)
			return &texture_info[i];
	return NULL;
}

TextureInfo *match_texture_description(const char *s) {
	for (int i = 0; i < NU_ITEMS; i++)
		if (strcasecmp(texture_info[i].text1, s) == 0 || strcasecmp(texture_info[i].text2, s) == 0)
			return &texture_info[i];
	return NULL;
}

TextureInfo *match_ktx_id(int gl_internal_format, int gl_format, int gl_type) {
	for (int i = 0; i < NU_ITEMS; i++)
		if (texture_info[i].gl_internal_format != 0 && texture_info[i].gl_internal_format == gl_internal_format) {
			if (texture_info[i].gl_format == 0)
				return &texture_info[i];
			if (texture_info[i].gl_format == gl_format && texture_info[i].gl_type == gl_type)
				return &texture_info[i];
		}
	for (int i = 0; i < NU_OPEN_GL_SYNONYMS; i++)
		if (open_gl_synonym[i].gl_internal_format == gl_internal_format) {
			if (open_gl_synonym[i].gl_format == 0)
				return match_texture_type(open_gl_synonym[i].type);
			if (open_gl_synonym[i].gl_format == gl_format && open_gl_synonym[i].gl_type == gl_type)
				return match_texture_type(open_gl_synonym[i].type);
		}
	return NULL;
}

TextureInfo *match_dds_id(const char *four_cc, int dx10_format, uint32_t pixel_format_flags, int bitcount, uint32_t red_mask, uint32_t green_mask, uint32_t blue_mask, uint32_t alpha_mask) {
	for (int i = 0; i < NU_ITEMS; i++)
		if (strncmp(four_cc, "DX10", 4) == 0) {
			if (texture_info[i].dx10_format == dx10_format)
				return &texture_info[i];
		}
		else
			if (texture_info[i].dx_four_cc[0] != '\0' &&
			strncmp(texture_info[i].dx_four_cc, four_cc, 4) == 0)
				return &texture_info[i];
			else {
				if (pixel_format_flags & 0x40) {	// Uncompressed data
					if ((texture_info[i].type & TEXTURE_TYPE_UNCOMPRESSED_BIT) &&
					texture_info[i].bits_per_block == bitcount && texture_info[i].red_mask == red_mask &&
					texture_info[i].green_mask == green_mask && ((pixel_format_flags & 0x1) == 0 ||
					texture_info[i].alpha_mask == alpha_mask)) {
						return &texture_info[i];
					}
				}
			}
	for (int i = 0; i < NU_DDS_SYNONYMS; i++)
		if (strncmp(four_cc, "DX10", 4) == 0) {
			if (dds_synonym[i].dx10_format == dx10_format)
				return match_texture_type(dds_synonym[i].type);
		}
		else
		if (texture_info[i].dx_four_cc[0] != '\0' &&
		strncmp(texture_info[i].dx_four_cc, four_cc, 4) == 0)
			return match_texture_type(dds_synonym[i].type);
	return NULL;
}

// Return a description of the texture type.

const char *texture_type_text(int texture_type) {
	TextureInfo *info;
	info = match_texture_type(texture_type);
	if (info == NULL) {
		printf("Error -- invalid texture type.\n");
		exit(1);
	}
	return info->text1;
}

// Set the texture decoding function and comparison function based on the texture type.
// If image is NULL, use the texture's native format in the comparison function.
// If image if not NULL, and for example in half-float format, use a higher-precision comparison function
// if possible.

void set_texture_decoding_function(Texture *texture, Image *image) {
	TextureDecodingFunction decoding_func;
	TextureComparisonFunction comparison_func;
	if (texture->type >= TEXTURE_TYPE_RGBA_ASTC_4X4 && texture->type <= TEXTURE_TYPE_RGBA_ASTC_12X12) {
		decoding_func = draw_block_rgba_astc;
		comparison_func = compare_block_any_size_rgba;
		goto end;
	}
	else
	switch (texture->type) {
	case TEXTURE_TYPE_ETC1 :
		decoding_func = draw_block4x4_etc1;
		break;
	case TEXTURE_TYPE_ETC2_RGB8 :
	case TEXTURE_TYPE_ETC2_SRGB8 :
		decoding_func = draw_block4x4_etc2_rgb8;
		break;
	case TEXTURE_TYPE_ETC2_EAC :
	case TEXTURE_TYPE_ETC2_SRGB_EAC :
		decoding_func = draw_block4x4_etc2_eac;
		break;
	case TEXTURE_TYPE_ETC2_PUNCHTHROUGH :
	case TEXTURE_TYPE_ETC2_SRGB_PUNCHTHROUGH :
		decoding_func = draw_block4x4_etc2_punchthrough;
		break;
	case TEXTURE_TYPE_R11_EAC :
		decoding_func = draw_block4x4_r11_eac;
		break;
	case TEXTURE_TYPE_RG11_EAC :
		decoding_func = draw_block4x4_rg11_eac;
		break;
	case TEXTURE_TYPE_SIGNED_R11_EAC :
		decoding_func = draw_block4x4_signed_r11_eac;
		break;
	case TEXTURE_TYPE_SIGNED_RG11_EAC :
		decoding_func = draw_block4x4_signed_rg11_eac;
		break;
	case TEXTURE_TYPE_DXT1 :
		decoding_func = draw_block4x4_dxt1;
		break;
	case TEXTURE_TYPE_DXT3 :
		decoding_func = draw_block4x4_dxt3;
		break;
	case TEXTURE_TYPE_DXT5 :
		decoding_func = draw_block4x4_dxt5;
		break;
	case TEXTURE_TYPE_DXT1A :
		decoding_func = draw_block4x4_dxt1a;	
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RGB8 :
	case TEXTURE_TYPE_UNCOMPRESSED_RGBA8 :
	case TEXTURE_TYPE_UNCOMPRESSED_RG8 :
	case TEXTURE_TYPE_UNCOMPRESSED_R8 :
	case TEXTURE_TYPE_UNCOMPRESSED_RG16 :
	case TEXTURE_TYPE_UNCOMPRESSED_R16 : 
		decoding_func = draw_block4x4_uncompressed;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_ARGB8 :
		decoding_func = draw_block4x4_argb8;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RGB_HALF_FLOAT :
		decoding_func = draw_block4x4_uncompressed_rgb_half_float;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RGBA_HALF_FLOAT :
		decoding_func = draw_block4x4_uncompressed_rgba_half_float;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_R_HALF_FLOAT :
		decoding_func = draw_block4x4_uncompressed_r_half_float;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RG_HALF_FLOAT :
		decoding_func = draw_block4x4_uncompressed_rg_half_float;
		break;
	case TEXTURE_TYPE_BPTC :
		decoding_func = draw_block4x4_bptc;
		break;
	case TEXTURE_TYPE_BPTC_FLOAT :
		decoding_func = draw_block4x4_bptc_float;
		break;
	case TEXTURE_TYPE_BPTC_SIGNED_FLOAT :
		decoding_func = draw_block4x4_bptc_signed_float;
		break;
	case TEXTURE_TYPE_RGTC1 :
		decoding_func = draw_block4x4_rgtc1;
		break;
	case TEXTURE_TYPE_SIGNED_RGTC1 :
		decoding_func = draw_block4x4_signed_rgtc1;
		break;
	case TEXTURE_TYPE_RGTC2 :
		decoding_func = draw_block4x4_rgtc2;
		break;
	case TEXTURE_TYPE_SIGNED_RGTC2 :
		decoding_func = draw_block4x4_signed_rgtc2;
		break;
	default :
		printf("Error -- no decoding function defined for texture type.\n");
		exit(1);
	}
	switch (texture->type) {
	case TEXTURE_TYPE_UNCOMPRESSED_RGB8 :
	case TEXTURE_TYPE_ETC1 :
	case TEXTURE_TYPE_ETC2_RGB8 :
	case TEXTURE_TYPE_ETC2_SRGB8 :
	case TEXTURE_TYPE_DXT1 :
		comparison_func = compare_block_4x4_rgb;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RGBA8 :
	case TEXTURE_TYPE_UNCOMPRESSED_ARGB8 :
	case TEXTURE_TYPE_ETC2_PUNCHTHROUGH :
	case TEXTURE_TYPE_ETC2_SRGB_PUNCHTHROUGH :
	case TEXTURE_TYPE_ETC2_EAC :
	case TEXTURE_TYPE_ETC2_SRGB_EAC :
	case TEXTURE_TYPE_DXT3 :
	case TEXTURE_TYPE_DXT5 :
	case TEXTURE_TYPE_DXT1A :
	case TEXTURE_TYPE_BPTC :
		comparison_func = compare_block_4x4_rgba;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RG8 :
	case TEXTURE_TYPE_UNCOMPRESSED_R8 :
	case TEXTURE_TYPE_RGTC1 :
	case TEXTURE_TYPE_RGTC2 :
		comparison_func = compare_block_4x4_8_bit_components;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG8 :
	case TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R8 :
		comparison_func = compare_block_4x4_signed_8_bit_components;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_R16 :
	case TEXTURE_TYPE_R11_EAC :
		comparison_func = compare_block_4x4_r16;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RG16 :
	case TEXTURE_TYPE_RG11_EAC :
		comparison_func = compare_block_4x4_rg16;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R16 :
	case TEXTURE_TYPE_SIGNED_R11_EAC :
	case TEXTURE_TYPE_SIGNED_RGTC1 :
		comparison_func = compare_block_4x4_r16_signed;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG16 :
	case TEXTURE_TYPE_SIGNED_RG11_EAC :
	case TEXTURE_TYPE_SIGNED_RGTC2 :
		comparison_func = compare_block_4x4_rg16_signed;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RGB_HALF_FLOAT :
	case TEXTURE_TYPE_BPTC_FLOAT :
	case TEXTURE_TYPE_BPTC_SIGNED_FLOAT :
		if (option_hdr)
			comparison_func = compare_block_4x4_rgb_half_float_hdr;
		else
			comparison_func = compare_block_4x4_rgb_half_float;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RGBA_HALF_FLOAT :
		if (option_hdr)
			comparison_func = compare_block_4x4_rgba_half_float_hdr;
		else
			comparison_func = compare_block_4x4_rgba_half_float;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_R_HALF_FLOAT :
		comparison_func = compare_block_4x4_r_half_float;
		break;
	case TEXTURE_TYPE_UNCOMPRESSED_RG_HALF_FLOAT :
		comparison_func = compare_block_4x4_rg_half_float;
		break;
	default :
		printf("Error -- no block comparison function defined for texture type.\n");
		exit(1);
	}
end :
	if (image != NULL) {
		if (comparison_func == compare_block_4x4_rgb && image->is_half_float)
			comparison_func = compare_block_4x4_rgb8_with_half_float;
		if (comparison_func == compare_block_4x4_rgba && image->is_half_float)
			comparison_func = compare_block_4x4_rgba8_with_half_float;
		if (comparison_func == compare_block_4x4_8_bit_components && image->bits_per_component == 16)
			comparison_func = compare_block_4x4_8_bit_components_with_16_bit;
		if (comparison_func == compare_block_4x4_signed_8_bit_components && image->bits_per_component == 16)
			comparison_func = compare_block_4x4_signed_8_bit_components_with_16_bit;
	}
	texture->decoding_function = decoding_func;
	texture->comparison_function = comparison_func;
}

int get_number_of_texture_formats() {
	return NU_ITEMS;
}

const char *get_texture_format_index_text(int i, int j) {
	if (j == 0)
		return texture_info[i].text1;
	else
		return texture_info[i].text2;
}

int draw_block4x4_uncompressed(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	// Block is one pixel.
	image_buffer[0] = *(unsigned int *)&bitstring[0];
	return 1;
}

int draw_block4x4_argb8(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	uint32_t pixel = *(uint32_t *)&bitstring[0];
	image_buffer[0] = pack_rgba(pixel_get_a(pixel), pixel_get_r(pixel), pixel_get_g(pixel), pixel_get_b(pixel));
	return 1;
}

int draw_block4x4_uncompressed_rgb_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	// Block is one pixel.
	// The image is also in half-float format.
	*(uint64_t *)&image_buffer[0] = *(uint64_t *)&bitstring[0];
	return 1;
}

int draw_block4x4_uncompressed_rgba_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	// Block is one pixel.
	// The image is also in half-float format.
	*(uint64_t *)&image_buffer[0] = *(uint64_t *)&bitstring[0];
	return 1;
}

int draw_block4x4_uncompressed_r_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	*(uint64_t *)&image_buffer[0] = *(uint64_t *)&bitstring[0];
	return 1;
}

int draw_block4x4_uncompressed_rg_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	*(uint64_t *)&image_buffer[0] = *(uint64_t *)&bitstring[0];
	return 1;
}


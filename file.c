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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <png.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

// Load a .pkm texture.

void load_pkm_file(const char *filename, Texture *texture) {
	if (!option_quiet)
		printf("Reading .pkm file %s.\n", filename);
	FILE *f = fopen(filename, "rb");
	unsigned char header[16];
	fread(header, 1, 16, f);
	if (header[0] != 'P' || header[1] != 'K' || header[2] != 'M' || header[3] != ' ') {
		printf("Error -- couldn't find PKM signature.\n");
		exit(1);
	}

	int texture_type = ((int)header[6] << 8) | header[7];
	switch (texture_type) {
		case 0:
			texture->type = TEXTURE_TYPE_ETC1;
			texture->info = match_texture_type(TEXTURE_TYPE_ETC1);
			break;
		case 1:
			texture->type = TEXTURE_TYPE_ETC2_RGB8;
			texture->info = match_texture_type(TEXTURE_TYPE_ETC2_RGB8);
			break;
		case 3:
			texture->type = TEXTURE_TYPE_ETC2_EAC;
			texture->info = match_texture_type(TEXTURE_TYPE_ETC2_EAC);
			break;
		case 4:
			texture->type = TEXTURE_TYPE_ETC2_PUNCHTHROUGH;
			texture->info = match_texture_type(TEXTURE_TYPE_ETC2_PUNCHTHROUGH);
			break;
		case 5:
			texture->type = TEXTURE_TYPE_R11_EAC;
			texture->info = match_texture_type(TEXTURE_TYPE_R11_EAC);
			break;
		case 6:
			texture->type = TEXTURE_TYPE_RG11_EAC;
			texture->info = match_texture_type(TEXTURE_TYPE_RG11_EAC);
			break;
		case 7:
			texture->type = TEXTURE_TYPE_SIGNED_R11_EAC;
			texture->info = match_texture_type(TEXTURE_TYPE_SIGNED_R11_EAC);
			break;
		case 8:
			texture->type = TEXTURE_TYPE_SIGNED_RG11_EAC;
			texture->info = match_texture_type(TEXTURE_TYPE_SIGNED_RG11_EAC);
			break;
		default:
			printf("Error -- unsupported format PKM format.\n");
			exit(1);
	}

	int ext_width = ((int)header[8] << 8) | header[9];
	int ext_height = ((int)header[10] << 8) | header[11];
	int width = ((int)header[12] << 8) | header[13];
	int height = ((int)header[14] << 8) | header[15];
	texture->extended_width = ext_width;
	texture->extended_height = ext_height;
	texture->width = width;
	texture->height = height;

	texture->block_width = texture->info->block_width;
	texture->block_height = texture->info->block_height;
	texture->bits_per_block = texture->info->bits_per_block;
	int n = (ext_width / texture->block_width) * (ext_height / texture->block_height);
	texture->pixels = (unsigned int *)malloc(n * (texture->bits_per_block / 8));
	if (fread(texture->pixels, 1, n * (texture->bits_per_block / 8), f) < n * (texture->bits_per_block / 8)) {
		printf("Error reading file %s.\n", filename);
		exit (1);
	}
	fclose(f);
}

// Save a .pkm texture.

void save_pkm_file(Texture *texture, const char *filename) {
	if (!option_quiet)
		printf("Writing .pkm file %s with texture format %s.\n", filename, texture->info->text1);
	FILE *f = fopen(filename, "wb");
	if (f == NULL) {
		printf("Error opening output file.\n");
		exit(1);
	}
	fputc('P', f); fputc('K', f); fputc('M', f); fputc(' ', f);
	fputc('1', f); fputc('0', f);
	fputc(0, f); fputc(0, f);
	int pkm_texture_type;
	switch (texture->type) {
	case TEXTURE_TYPE_ETC1 :
		pkm_texture_type = 0;
		break;
	case TEXTURE_TYPE_ETC2_RGB8 :
		pkm_texture_type = 1;
		break;
	default :
		printf("Error -- writing of texture format to .pkm file not supported.\n");
		exit(1);
	}
	fputc((pkm_texture_type & 0xFF00) >> 8, f);
	fputc((pkm_texture_type & 0xFF), f);
	// Write size data in big-endian 16-bit words.
	fputc((texture->extended_width & 0xFF00) >> 8, f);		// Extended width.
	fputc((texture->extended_width & 0xFB), f);
	fputc((texture->extended_height & 0xFF00) >> 8, f);	// Extended height.
	fputc((texture->extended_height & 0xFB), f);
	fputc((texture->width & 0xFF00) >> 8, f);		// Original width.
	fputc((texture->width & 0xFF), f);
	fputc((texture->height & 0xFF00) >> 8, f);		// Original height.
	fputc((texture->height & 0xFF), f);
	int n = (texture->extended_height / texture->block_height) * (texture->extended_width / texture->block_width);
	fwrite(texture->pixels, 1, n * (texture->bits_per_block / 8), f);
	fclose(f);
}

// Load a .ktx texture. At most max_mipmaps are stored in the array of Textures texture. The number of
// mipmaps loaded is returned.

static unsigned char ktx_id[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };

int load_ktx_file(const char *filename, int max_mipmaps, Texture *texture) {
	if (!option_quiet)
		printf("Reading .ktx file %s.\n", filename);
	FILE *f = fopen(filename, "rb");
	int header[16];
	fread(header, 1, 64, f);
	if (memcmp(header, ktx_id, 12) != 0) {
		printf("Error -- couldn't find KTX signature.\n");
		exit(1);
	}
	int wrong_endian = 0;
	if (header[3] == 0x01020304) {
		// Wrong endian .ktx file.
		wrong_endian = 1;
		for (int i = 3; i < 16; i++) {
			unsigned char *b = (unsigned char *)&header[i];
			unsigned char temp = b[0];
			b[0] = b[3];
			b[3] = temp;
			temp = b[1];
			b[1] = b[2];
			b[2] = temp;
		}
	}
	int type;
	int glType = header[4];
	int glFormat = header[6];
	int glInternalFormat = header[7];
	int pixelDepth = header[11];
	TextureInfo *info = match_ktx_id(glInternalFormat, glFormat, glType);
	if (info == NULL) {
		printf("Error -- unsupported format in .ktx file (glInternalFormat = 0x%04X).\n", glInternalFormat);
		exit(1);
	}
	type = info->type;
	if (type == TEXTURE_TYPE_DXT1 && option_texture_format == TEXTURE_TYPE_DXT1A) {
		info = match_texture_type(TEXTURE_TYPE_DXT1A);
		type = TEXTURE_TYPE_DXT1A;
	}
	int bits_per_block = info->internal_bits_per_block;
	int block_width = info->block_width;
	int block_height = info->block_height;
	int ktx_block_size = info->bits_per_block;
	if (!option_quiet)
		printf("File is %s texture.\n", info->text1);
	int width = header[9];
	int height = header[10];
	int extended_width = ((width + block_width - 1) / block_width) * block_width;
	int extended_height = ((height + block_height - 1) / block_height) * block_height;
	int nu_mipmaps = header[14];
	if (nu_mipmaps > 1 && max_mipmaps == 1) {
		printf("Disregarding mipmaps beyond the first level.\n");
	}
 	if (header[15] > 0) {
		// Skip metadata.
		unsigned char *metadata = (unsigned char *)malloc(header[15]);
		if (fread(metadata, 1, header[15], f) < header[15]) {
			printf("Error reading metadata.\n");
			exit(1);
		}
		free(metadata);
	}
	for (int i = 0; i < max_mipmaps && i < nu_mipmaps; i++) {
		unsigned char image_size[4];
		int r = fread(image_size, 1, 4, f);
		if (wrong_endian) {
			unsigned char temp = image_size[0];
			image_size[0] = image_size[3];
			image_size[3] = temp;
			temp = image_size[1];
			image_size[1] = image_size[2];
			image_size[2] = temp;
		}
		int n = (extended_height / block_height) * (extended_width / block_width);
		if (type != TEXTURE_TYPE_UNCOMPRESSED_RGB8 && type != TEXTURE_TYPE_UNCOMPRESSED_RGB_HALF_FLOAT &&
		type != TEXTURE_TYPE_UNCOMPRESSED_R8 &&	type != TEXTURE_TYPE_UNCOMPRESSED_RG8 &&
		type != TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R8 && type != TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG8 &&
		type != TEXTURE_TYPE_UNCOMPRESSED_R16 && type != TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R16 &&
		type != TEXTURE_TYPE_UNCOMPRESSED_R_HALF_FLOAT &&
		*(int *)&image_size[0] != n * (ktx_block_size / 8)) {
			printf("Error -- image size field of mipmap level %d does not match (%d vs %d).\n",
				i, *(int *)&image_size[0], n * (ktx_block_size / 8));
			exit(1);
		}
		texture[i].info = info;
		texture[i].width = width;
		texture[i].height = height;
		texture[i].extended_width = extended_width;
		texture[i].extended_height = extended_height;
		texture[i].bits_per_block = bits_per_block;
		texture[i].type = type;
		texture[i].block_width = block_width;
		texture[i].block_height = block_height;
		texture[i].pixels = (unsigned int *)malloc(n * (bits_per_block / 8));
		if (bits_per_block != ktx_block_size) {
			// Have to read row by row due to row padding, and convert 24-bit pixels to to 32-bit pixels
			// or 48-bit pixels to 64-bit pixels.
			int bpp = ktx_block_size / 8;
			int row_size = (width * bpp + 3) & ~3;
			int row_size_no_padding = width * bpp;
			if (*(int *)&image_size[0] != height * row_size) {
				if (*(int *)&image_size[0] == height * row_size_no_padding) {
					// This file violates .ktx specification by have no 32-bit row alignment.
					// Load it anyway.
					printf("Warning: file %s violates KTX row alignment specification for "
						"uncompressed textures.\n", filename);
					row_size = row_size_no_padding;
				}
				else {
					printf("Error -- image size field of mipmap level %d does not match (%d vs %d).\n",
						i, *(int *)&image_size[0], height * row_size);
					printf("bpp = %d, ktx_block_size == %d\n", bpp, ktx_block_size);
					exit(1);
				}
			}
			unsigned char *row = (unsigned char *)alloca(row_size);
			for (int y = 0; y < height; y++) {
				if (fread(row, 1, row_size, f) < row_size) {
					printf("Error reading file %s.\n", filename);
					exit(1);
				}
				for (int x = 0; x < width; x++) {
					unsigned int pixel;
					if (bpp == 3) {
						pixel = pack_rgb_alpha_0xff(row[x * 3], row[x * 3 + 1],	row[x * 3 + 2]);
						texture[i].pixels[y * extended_width + x] = pixel;
					}
					else
					if (bpp == 6) {
						texture[i].pixels[(y * extended_width + x) * 2] = pack_half_float(
							*(unsigned short *)&row[x * 6],
							*(unsigned short *)&row[x * 6 + 2]);
						texture[i].pixels[(y * extended_width + x) * 2 + 1] = pack_half_float(
							*(unsigned short *)&row[x * 6 + 4], 0);
					}
					else
					if (bpp == 2) {
						if (bits_per_block == 64)
							*(uint64_t *)&texture[i].pixels[(y * extended_width + x) * 2] = *(unsigned short *)&row[x * 2];
						else
							// This might present a problem on big-endian systems.
							texture[i].pixels[y * extended_width + x] = *(unsigned short *)&row[x * 2];
					}
					else
					if (bpp == 1)
						texture[i].pixels[y * extended_width + x] = row[x];
					else
					if (bpp == 4 && bits_per_block == 64)
						*(uint64_t *)&texture[i].pixels[(y * extended_width + x) * 2] = pack_half_float(*(uint16_t *)&row[x * 4],
							*(uint16_t *)&row[x * 4 + 2]);
					else {
						printf("Error -- cannot handle combination of internal size and real size of texture data.\n");
							exit(1);
					}
				}
			}
		}
		else
			if (fread(texture[i].pixels, 1, n * (ktx_block_size / 8), f) < n * (ktx_block_size / 8)) {
				printf("Error reading file %s.\n", filename);
				exit (1);
			}
		// Divide by two for the next mipmap level, rounding down.
		width >>= 1;
		height >>= 1;
		extended_width = ((width + block_width - 1) / block_width) * block_width;
		extended_height = ((height + block_height - 1) / block_height) * block_height;
		// Read mipPadding. But not if we have already read everything specified.
		char buffer[4];
		if (i + 1 < max_mipmaps && i + 1 < nu_mipmaps)
			fread(buffer, 1, 3 - ((*(int *)&image_size[0] + 3) % 4), f);
	}
	fclose(f);
	// Return the number of stored textures.
	if (max_mipmaps < nu_mipmaps)
		return max_mipmaps;
	else
		return nu_mipmaps;
}

// Save a .ktx texture. texture is a pointer to an array of Texture structures.

static char ktx_orientation_key_down[24] = { 'K', 'T', 'X', 'o', 'r', 'i', 'e', 'n', 't', 'a', 't', 'i', 'o', 'n', 0,
	'S', '=', 'r', ',', 'T', '=', 'd', 0, 0 };	// Includes one byte of padding.
static char ktx_orientation_key_up[24] = { 'K', 'T', 'X', 'o', 'r', 'i', 'e', 'n', 't', 'a', 't', 'i', 'o', 'n', 0,
	'S', '=', 'r', ',', 'T', '=', 'u', 0, 0 };	// Includes one byte of padding.

void save_ktx_file(Texture *texture, int nu_mipmaps, const char *filename) {
	if (!option_quiet)
		printf("Writing .ktx file %s with texture format %s.\n", filename, texture->info->text1);
	FILE *f = fopen(filename, "wb");
	if (f == NULL) {
		printf("Error opening output file.\n");
		exit(1);
	}
	unsigned int header[16];
	memset(header, 0, 64);
	memcpy(header, ktx_id, 12);	// Set id.
	header[3] = 0x04030201;
	int glType = 0;
	int glTypeSize = 1;
	int glFormat = 0;
	glType = texture->info->gl_type;
	glFormat = texture->info->gl_format;
	int glInternalFormat = texture->info->gl_internal_format;
	header[4] = glType;			// glType
	header[5] = glTypeSize;			// glTypeSize
	header[6] = glFormat;			// glFormat
	header[7] = glInternalFormat;
	header[9] = texture[0].width;
	header[10] = texture[0].height;
	header[11] = 0;
	header[13] = 1;			// Number of faces.
	header[14] = nu_mipmaps;	// Mipmap levels.
	int data[1];
	if (option_orientation == 0) {
		header[15] = 0;
		fwrite(header, 1, 64, f);
	}
	else {
		header[15] = 28;	// Key value data bytes.
		fwrite(header, 1, 64, f);
		data[0] = 27;		// Key and value size.
		fwrite(data, 1, 4, f);
		if (option_orientation == ORIENTATION_DOWN)
			fwrite(ktx_orientation_key_down, 1, 24, f);
		else
			fwrite(ktx_orientation_key_up, 1, 24, f);
	}
	for (int i = 0; i < nu_mipmaps; i++) {
		int n = (texture[i].extended_height / texture[i].block_height) *
			(texture[i].extended_width / texture[i].block_width);
		if (!option_quiet)
			printf("Writing mipmap level %d of size %d x %d.\n", i, texture[i].width, texture[i].height);
		// Because of per row 32-bit alignment mandated by the KTX specification, we have to handle
		// special cases of unaligned uncompressed textures.
		if (texture[i].info->bits_per_block == 48) {
			// 48-bit pixel texture, texgenpack internal format is 64-bit.
			int bpp = 6;
			int row_size = (texture[i].width * bpp + 3) & ~3;
			data[0] = texture[i].height * row_size;		// Image size.
			fwrite(data, 1, 4, f);
			unsigned char *row = (unsigned char *)alloca(row_size);
			for (int y = 0; y < texture[i].height; y++) {
				for (int x = 0; x < texture[i].width; x++) {
					*(unsigned short *)&row[x * bpp] = pixel_get_r16(texture[i].pixels[
						(y * texture[i].extended_width + x) * 2]);
					*(unsigned short *)&row[x * bpp + 2] = pixel_get_g16(texture[i].pixels[
						(y * texture[i].extended_width + x) * 2]);
					*(unsigned short *)&row[x * bpp + 4] = pixel_get_r16(texture[i].pixels[
						(y * texture[i].extended_width + x) * 2 + 1]);
				}
				memset(&row[texture[i].width * bpp], 0, row_size - texture[i].width * bpp);
				fwrite(row, 1, row_size, f);
			}
		}
		else
		if (texture[i].info->bits_per_block == 24) {
			// 24-bit pixel texture, texgenpack internal format is 32-bit.
			int bpp = 3;
			int row_size = (texture[i].width * bpp + 3) & ~3;
			data[0] = texture[i].height * row_size;		// Image size.
			fwrite(data, 1, 4, f);
			unsigned char *row = (unsigned char *)alloca(row_size);
			for (int y = 0; y < texture[i].height; y++) {
				for (int x = 0; x < texture[i].width; x++) {
					*(unsigned char *)&row[x * bpp] = pixel_get_r(texture[i].pixels[
						y * texture[i].extended_width + x]);
					*(unsigned char *)&row[x * bpp + 1] = pixel_get_g(texture[i].pixels[
						y * texture[i].extended_width + x]);
					*(unsigned char *)&row[x * bpp + 2] = pixel_get_b(texture[i].pixels[
						y * texture[i].extended_width + x]);
				}
				memset(&row[texture[i].width * bpp], 0, row_size - texture[i].width * bpp);
				fwrite(row, 1, row_size, f);
			}
		}
		else
		if (texture[i].info->bits_per_block == 16) {
			// 16-bit pixel texture, texgenpack internal format is 32-bit or 64-bit (R half-float).
			int bpp = 2;
			int row_size = (texture[i].width * bpp + 3) & ~3;
			data[0] = texture[i].height * row_size;		// Image size.
			fwrite(data, 1, 4, f);
			unsigned char *row = (unsigned char *)alloca(row_size);
			for (int y = 0; y < texture[i].height; y++) {
				for (int x = 0; x < texture[i].width; x++) {
					if (texture[i].info->internal_bits_per_block == 64)
						*(uint16_t *)&row[x * bpp] = pixel_get_r16(texture[i].pixels[(
							y * texture[i].extended_width + x) * 2]);
					else
						*(uint16_t *)&row[x * bpp] = pixel_get_r16(texture[i].pixels[
							y * texture[i].extended_width + x]);
				}
				memset(&row[texture[i].width * bpp], 0, row_size - texture[i].width * bpp);
				fwrite(row, 1, row_size, f);
			}
		}
		else
		if (texture[i].info->bits_per_block == 8) {
			// 8-bit pixel texture, texgenpack internal format is 32-bit.
			int bpp = 1;
			int row_size = (texture[i].width * bpp + 3) & ~3;
			data[0] = texture[i].height * row_size;		// Image size.
			fwrite(data, 1, 4, f);
			unsigned char *row = (unsigned char *)alloca(row_size);
			for (int y = 0; y < texture[i].height; y++) {
				for (int x = 0; x < texture[i].width; x++) {
					*(uint8_t *)&row[x * bpp] = pixel_get_r(texture[i].pixels[
						y * texture[i].extended_width + x]);
				}
				memset(&row[texture[i].width * bpp], 0, row_size - texture[i].width * bpp);
				fwrite(row, 1, row_size, f);
			}
		}
		else {
			// Regular 32-bit aligned texture.
			data[0] = n * (texture[i].bits_per_block / 8);	// Image size.
			fwrite(data, 1, 4, f);
			fwrite(texture[i].pixels, 1, n * (texture[i].bits_per_block / 8), f);
		}
	}
	fclose(f);
}

// Load a .dds texture.

int load_dds_file(const char *filename, int max_mipmaps, Texture *texture) {
	if (!option_quiet)
		printf("Reading .dds file %s.\n", filename);
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		printf("Error opening file %s.\n", filename);
		exit(1);
	}
	char id[4];
	fread(id, 1, 4, f);
	if (id[0] != 'D' || id[1] != 'D' || id[2] != 'S' || id[3] != ' ') {
		printf("Error -- couldn't find DDS signature.\n");
		exit(1);
	}
	unsigned char header[124];
	fread(header, 1, 124, f);
	int width = *(unsigned int *)&header[12];
	int height = *(unsigned int *)&header[8];
	int pitch = *(unsigned int *)&header[16];
	int pixel_format_flags = *(unsigned int *)&header[76];
	int type;
	int block_width = 4;
	int block_height = 4;
	int internal_bits_per_block = 64;
	int dds_block_size = 64;
	int bitcount = *(unsigned int *)&header[84];
	unsigned int red_mask = *(unsigned int *)&header[88];
	unsigned int green_mask = *(unsigned int *)&header[92];
	unsigned int blue_mask = *(unsigned int *)&header[96];
	unsigned int alpha_mask = *(unsigned int *)&header[100];
	char four_cc[5];
	strncpy(four_cc, (char *)&header[80], 4);
	four_cc[4] = '\0';
	unsigned int dx10_format = 0;
	if (strncmp(four_cc, "DX10", 4) == 0) {
		unsigned char dx10_header[20];
		fread(dx10_header, 1, 20, f);
		dx10_format = *(unsigned int *)&dx10_header[0];
		unsigned int resource_dimension = *(unsigned int *)&dx10_header[4];
		if (resource_dimension != 3) {
			printf("Error -- only 2D textures supported for .dds files.\n");
			exit(1);
		}
	}
	TextureInfo *info = match_dds_id(four_cc, dx10_format, pixel_format_flags, bitcount, red_mask, green_mask, blue_mask, alpha_mask);
	if (info == NULL) {
		printf("Error -- unsupported format in .dds file (fourCC = %s, DX10 format = %d).\n", four_cc, dx10_format);
		exit(1);
	}
	type = info->type;
	if (type == TEXTURE_TYPE_DXT1 && option_texture_format == TEXTURE_TYPE_DXT1A) {
		info = match_texture_type(TEXTURE_TYPE_DXT1A);
		type = TEXTURE_TYPE_DXT1A;
	}
	internal_bits_per_block = info->internal_bits_per_block;
	block_width = info->block_width;
	block_height = info->block_height;
	dds_block_size = info->bits_per_block;
	if (!option_quiet)
		printf("File is %s texture.\n", info->text1);
	int extended_width = ((width + block_width - 1) / block_width) * block_width;
	int extended_height = ((height + block_height - 1) / block_height) * block_height;
	unsigned int flags = *(unsigned int *)&header[4];
	int nu_mipmaps = 1;
	if (flags & 0x20000) {
		nu_mipmaps = *(unsigned int *)&header[24];
		if (nu_mipmaps > 1 && max_mipmaps == 1) {
			if (!option_quiet)
				printf("Disregarding mipmaps beyond the first level.\n");
			nu_mipmaps = 1;
		}
	}
	for (int i = 0; i < max_mipmaps && i < nu_mipmaps; i++) {
		int n = (extended_height / block_width) * (extended_width / block_height);
		texture[i].info = info;
		texture[i].width = width;
		texture[i].height = height;
		texture[i].extended_width = extended_width;
		texture[i].extended_height = extended_height;
		texture[i].bits_per_block = info->bits_per_block;	// Real format bits_per_block
		texture[i].type = type;
		texture[i].block_width = block_width;
		texture[i].block_height = block_height;
		if (internal_bits_per_block != dds_block_size) {
			// This happens when we have 24-bit RGB data.
			// Convert to 32-bit.
			int bpp = dds_block_size / 8;
			int row_size = pitch;
			unsigned char *row = (unsigned char *)alloca(row_size);
			texture[i].pixels = (unsigned int *)malloc(n * (internal_bits_per_block / 8));
			for (int y = 0; y < height; y++) {
				if (fread(row, 1, row_size, f) < row_size) {
					printf("Error reading file %s.\n", filename);
					exit(1);
				}
				if (bpp == 3)
					for (int x = 0; x < width; x++)
						texture[i].pixels[y * extended_width + x] = pack_rgb_alpha_0xff(
							row[x * 3], row[x * 3 + 1], row[x * 3 + 2]);
				else
				if (bpp == 2)
					for (int x = 0; x < width; x++)
						if (internal_bits_per_block == 64)
							*(uint64_t *)&texture[i].pixels[(y * extended_width + x) * 2] =
								*(unsigned short *)&row[x * 2];
						else
							texture[i].pixels[y * extended_width + x] = pack_r(row[x * 2])  |
								pack_g(row[x * 2 + 1]);
				else
				if (bpp == 1)
					for (int x = 0; x < width; x++)
						texture[i].pixels[y * extended_width + x] = pack_r(row[x]);
				else
				if (bpp == 4 && internal_bits_per_block == 64)
					for (int x = 0; x < width; x++)
						*(uint64_t *)&texture[i].pixels[(y * extended_width + x) * 2] =
							pack_half_float(*(uint16_t *)&row[x * 4],
							*(uint16_t *)&row[x * 4 + 2]);
				else {
					printf("Error -- cannot handle combination of internal size and real size "
						"of texture data.\n");
					exit(1);
				}
			}
		}
		else {
			texture[i].pixels = (unsigned int *)malloc(n * (internal_bits_per_block / 8));
			int r = fread(texture[i].pixels, 1, n * (internal_bits_per_block / 8), f);
			if (r < n * (internal_bits_per_block / 8)) {
				printf("Error reading file %s.\n", filename);
				printf("%d bytes read vs. %d requested.\n", r, n * (internal_bits_per_block / 8));
				exit(1);
			}
		}
		// Divide by two for the next mipmap level, rounding down.
		width >>= 1;
		height >>= 1;
		extended_width = ((width + block_width - 1) / block_width) * block_width;
		extended_height = ((height + block_height - 1) / block_height) * block_height;
	}
	fclose(f);
	// Return the number of stored textures.
	if (max_mipmaps < nu_mipmaps)
		return max_mipmaps;
	else
		return nu_mipmaps;
}

// Save a .dds file.

void save_dds_file(Texture *texture, int nu_mipmaps, const char *filename) {
	if (!option_quiet)
		printf("Writing .dds file %s with texture format %s.\n", filename, texture->info->text1);
	int n = (texture->extended_height / texture->block_height) * (texture->extended_width / texture->block_width);
	FILE *f = fopen(filename, "wb");
	if (f == NULL) {
		printf("Error opening output file.\n");
		exit(1);
	}
	fputc('D', f); fputc('D', f); fputc('S', f); fputc(' ', f);
	unsigned char header[124];
	unsigned char dx10_header[20];
	memset(header, 0, 124);
	memset(dx10_header, 0, 20);
	*(unsigned int *)&header[0] = 124;
	unsigned int flags = 0x1007;
	if (nu_mipmaps > 1)
		flags |= 0x20000;
	if (texture->type & TEXTURE_TYPE_UNCOMPRESSED_BIT)
		flags |= 0x8;	// Pitch specified.
	else
		flags |= 0x80000;	// Linear size specified.
	*(unsigned int *)&header[4] = flags;	// Flags
	*(unsigned int *)&header[8] = texture->height;
	*(unsigned int *)&header[12] = texture->width;
	*(unsigned int *)&header[16] = n * (texture->bits_per_block / 8); // Linear size for compressed textures.
	*(unsigned int *)&header[24] = nu_mipmaps;	// Mipmap count.
	*(unsigned int *)&header[72] = 32;
	*(unsigned int *)&header[76] = 0x4;	// Pixel format flags (fourCC present).
	int write_dx10_header = 0;
	if (strncmp(texture->info->dx_four_cc, "DX10", 4) == 0) {
		write_dx10_header = 1;
		*(unsigned int *)&dx10_header[0] = texture->info->dx10_format;
		*(unsigned int *)&dx10_header[4] = 3;	// Resource dimensions = 2D.
		*(unsigned int *)&dx10_header[12] = 1;	// Array size.
	}
	if (strlen(texture->info->dx_four_cc) == 0) {
		uint32_t pixel_format_flags = 0x40;	// Uncompressed RGB data present.
		if (texture->info->alpha_bits > 0)
			pixel_format_flags |= 0x01;
		*(unsigned int *)&header[84] = texture->info->bits_per_block;	// bit count
		*(unsigned int *)&header[88] = texture->info->red_mask;
		*(unsigned int *)&header[92] = texture->info->green_mask;
		*(unsigned int *)&header[96] = texture->info->blue_mask;
		*(unsigned int *)&header[100] = texture->info->alpha_mask;
		*(unsigned int *)&header[76] = pixel_format_flags;
	}
	else {
		// In case of DXTn or DX10 fourCC, write it.
		strncpy((char *)&header[80], texture->info->dx_four_cc, 4);
		// Some readers don't like the absence of other data for uncompressed data with a DX10 header.
		if (texture->type & TEXTURE_TYPE_UNCOMPRESSED_BIT) {
			uint32_t pixel_format_flags = 0x44;	// fourCC + Uncompressed RGB data present.
			if (texture->info->alpha_bits > 0)
				pixel_format_flags |= 0x01;
			*(unsigned int *)&header[84] = texture->info->bits_per_block;	// bit count
			*(unsigned int *)&header[88] = texture->info->red_mask;
			*(unsigned int *)&header[92] = texture->info->green_mask;
			*(unsigned int *)&header[96] = texture->info->blue_mask;
			*(unsigned int *)&header[100] = texture->info->alpha_mask;
			*(unsigned int *)&header[76] = pixel_format_flags;
		}
	}
	unsigned int caps = 0x1000;
	if (nu_mipmaps > 1)
		caps |= 0x400008;
	*(unsigned int *)&header[104] = caps;	// Caps.
	int pitch = texture->extended_width * (texture->bits_per_block / 8);
	if (texture->type & TEXTURE_TYPE_UNCOMPRESSED_BIT)
		*(unsigned int *)&header[16] = pitch;
	fwrite(header, 1, 124, f);
	if (write_dx10_header)
		fwrite(dx10_header, 1, 20, f);
	for (int i = 0; i < nu_mipmaps; i++) {
		int n = (texture[i].extended_height / texture[i].block_height) *
			(texture[i].extended_width / texture[i].block_width);
		if (!option_quiet)
			printf("Writing mipmap level %d of size %d x %d.\n", i, texture[i].width, texture[i].height);
		if (texture[i].bits_per_block == texture[i].info->internal_bits_per_block)
			fwrite(texture[i].pixels, 1, n * (texture[i].bits_per_block / 8), f);
		else {
			int bpp = texture[i].bits_per_block / 8;
			if (bpp == 2 && texture[i].info->internal_bits_per_block == 32) {
				unsigned char *row = (unsigned char *)alloca(pitch);
				for (int y = 0; y < texture[i].height; y++) {
					for (int x = 0; x < texture[i].width; x++) {
						*(unsigned short *)&row[x * bpp] = pixel_get_r16(texture[i].pixels[
							y * texture[i].extended_width + x]);
					}
					memset(&row[texture[i].width * bpp], 0, pitch - texture[i].width * bpp);
					fwrite(row, 1, pitch, f);
				}
			}
			else
			if (bpp == 3 && texture[i].info->internal_bits_per_block == 32) {
				unsigned char *row = (unsigned char *)alloca(pitch);
				for (int y = 0; y < texture[i].height; y++) {
					for (int x = 0; x < texture[i].width; x++) {
						*(unsigned char *)&row[x * bpp] = pixel_get_r(texture[i].pixels[
							y * texture[i].extended_width + x]);
						*(unsigned char *)&row[x * bpp + 1] = pixel_get_g(texture[i].pixels[
							y * texture[i].extended_width + x]);
						*(unsigned char *)&row[x * bpp + 2] = pixel_get_b(texture[i].pixels[
							y * texture[i].extended_width + x]);
					}
					memset(&row[texture[i].width * bpp], 0, pitch - texture[i].width * bpp);
					fwrite(row, 1, pitch, f);
				}
			}
			else
			if (bpp == 1 && texture[i].info->internal_bits_per_block == 32) {
				unsigned char *row = (unsigned char *)alloca(pitch);
				for (int y = 0; y < texture[i].height; y++) {
					for (int x = 0; x < texture[i].width; x++) {
						*(unsigned char *)&row[x * bpp] = pixel_get_r(texture[i].pixels[
							y * texture[i].extended_width + x]);
					}
					memset(&row[texture[i].width * bpp], 0, pitch - texture[i].width * bpp);
					fwrite(row, 1, pitch, f);
				}
			}
			else
			if (bpp == 6 && texture[i].info->internal_bits_per_block == 64) {
				unsigned char *row= (unsigned char *)alloca(pitch);
				for (int y = 0; y < texture[i].height; y++) {
					for (int x = 0; x < texture[i].width; x++) {
						*(unsigned short *)&row[x * bpp] = pixel_get_r16(texture[i].pixels[
							(y * texture[i].extended_width + x) * 2]);
						*(unsigned short *)&row[x * bpp + 2] = pixel_get_g16(texture[i].pixels[
								(y * texture[i].extended_width + x) * 2]);
						*(unsigned short *)&row[x * bpp + 4] = pixel_get_r16(texture[i].pixels[
							(y * texture[i].extended_width + x) * 2 + 1]);
					}
					memset(&row[texture[i].width * bpp], 0, pitch - texture[i].width * bpp);
					fwrite(row, 1, pitch, f);
				}
			}
			else
			if (bpp == 2 && texture[i].info->internal_bits_per_block == 64) {
				unsigned char *row = (unsigned char *)alloca(pitch);
				for (int y = 0; y < texture[i].height; y++) {
					for (int x = 0; x < texture[i].width; x++) {
						*(unsigned short *)&row[x * bpp] = pixel_get_r16(texture[i].pixels[
							(y * texture[i].extended_width + x) * 2]);
					}
					memset(&row[texture[i].width * bpp], 0, pitch - texture[i].width * bpp);
					fwrite(row, 1, pitch, f);
				}
			}
			else {
				printf("Error -- unsupported pixel size when writing .dds file.\n"
					"bits_per_block = %d, internal_bits_per_block = %d.\n",
					texture[i].bits_per_block, texture[i].info->internal_bits_per_block);
				exit(1);
			}
		}
	}
	fclose(f);
}

// Load an .astc file.

void load_astc_file(const char *filename, Texture *texture) {
	if (!option_quiet)
		printf("Reading .astc file %s.\n", filename);
	FILE *f = fopen(filename, "rb");
	unsigned char header[16];
	fread(header, 1, 16, f);
	if (header[0] != 0x13 || header[1] != 0xAB || header[2] != 0xA1 || header[3] != 0x5c) {
		printf("Error -- couldn't find ASTC signature.\n");
		exit(1);
	}
	int blockdim_x = header[4];
	int blockdim_y = header[5];
	int blockdim_z = header[6];
	if (blockdim_z != 1) {
		printf("Error -- 3D blocksize not supported.\n");
		exit(1);
	}
	int i = match_astc_block_size(blockdim_x, blockdim_y);
	if (i == - 1) {
		printf("Error -- unrecognized block size in .astc file.\n");
		exit(1);
	}
	texture->block_width = blockdim_x;
	texture->block_height = blockdim_y;
	int width = header[7] + (int)header[8] * 256 + (int)header[9] * 65536;
	int height = header[10] + (int)header[11] * 256 + (int)header[12] * 65536;
	int zsize = header[13] + (int)header[14] * 256 + (int)header[15] * 65536;
	if (zsize != 1) {
		printf("Error -- 3D textures not supported.\n");
		exit(1);
	}
	int xblocks = (width + blockdim_x - 1) / blockdim_x;
	int yblocks = (height + blockdim_y - 1) / blockdim_y;
	int zblocks = (zsize + blockdim_z - 1) / blockdim_z;
	int n = xblocks * yblocks * zblocks;
	texture->pixels = (unsigned int *)malloc(n * 16);
	texture->bits_per_block = 128;
	if (fread(texture->pixels, 1, n * 16, f) < n * 16) {
		printf("Error reading file %s.\n", filename);
		exit (1);
	}
	fclose(f);
	texture->extended_width = xblocks * blockdim_x;
	texture->extended_height = yblocks * blockdim_y;
	texture->width = width;
	texture->height = height;
	texture->type = TEXTURE_TYPE_RGBA_ASTC_4X4 + i;
	texture->info = match_texture_type(texture->type);
}

// Save an .astc file.

void save_astc_file(Texture *texture, const char *filename) {
	if (!option_quiet)
		printf("Writing .astc file %s.\n", filename);
	FILE *f = fopen(filename, "wb");
	if (f == NULL) {
		printf("Error opening output file.\n");
		exit(1);
	}
	unsigned char header[16];
	header[0] = 0x13; header[1] = 0xAB; header[2] = 0xA1; header[3] = 0x5C;
	header[4] = texture->block_width;
	header[5] = texture->block_height;
	header[6] = 1;
	header[7] = texture->width & 0xFF;
	header[8] = (texture->width & 0xFF00) >> 8;
	header[9] = (texture->width & 0xFF0000) >> 16;
	header[10] = texture->height & 0xFF;
	header[11] = (texture->height & 0xFF00) >> 8;
	header[12] = (texture->height & 0xFF0000) >> 16;
	int zsize = 1;
	header[13] = zsize & 0xFF;
	header[14] = (zsize & 0xFF00) >> 8;
	header[15] = (zsize & 0xFF0000) >> 16;
	fwrite(header, 1, 16, f);
	int n = (texture->extended_height / texture->block_height) * (texture->extended_width / texture->block_width);
	fwrite(texture->pixels, 1, n * (texture->bits_per_block / 8), f);
	fclose(f);
}

// Load a .ppm file.

void load_ppm_file(const char *filename, Image *image) {
	if (!option_quiet)
		printf("Reading .ppm file %s.\n", filename);
	FILE *f = fopen(filename, "rb");
	fclose(f);
}

// Load a .png file.

void load_png_file(const char *filename, Image *image) {
	int png_width, png_height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_bytep *row_pointers;

	png_byte header[8];    // 8 is the maximum size that can be checked

        FILE *fp = fopen(filename, "rb");
	if (!fp) {
		printf("Error - file %s could not be opened for reading.\n", filename);
		exit(1);
	}
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8)) {
		printf("Error - file %s is not recognized as a PNG file.\n", filename);
		exit(1);
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		printf("png_create_read_struct failed\n");
		exit(1);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		printf("png_create_info_struct failed\n");
		exit(1);
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		printf("Error during init_io.");
		exit(1);
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_width = png_get_image_width(png_ptr, info_ptr);
	png_height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

        /* read file */
	if (setjmp(png_jmpbuf(png_ptr))) {
		printf("Error during read_image.\n");
		exit(1);
        }

	row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * png_height);
	for (int y = 0; y < png_height; y++)
		row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));

	png_read_image(png_ptr, row_pointers);

	fclose(fp);

	if (!option_quiet) {
		printf("Loading .png image with size (%d x %d), bit depth %d", png_width, png_height, bit_depth);
		if (color_type == PNG_COLOR_TYPE_RGBA)
			printf(", with alpha.\n");
		else
			printf(".\n");
	}
	if (color_type != PNG_COLOR_TYPE_GRAY && color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA) {
		printf("Error - unrecognized color format.\n");
		exit(1);
	}
	if (bit_depth != 8) {
		printf("Error - expected bit depth of 8 in PNG file.\n");
		exit(1);
	}

	image->width = png_width;
	image->height = png_height;
	// In the current version, extending the image to a 4x4 block boundary is no longer necessary.
	image->extended_width = png_width;
	image->extended_height = png_height;
	image->pixels = (unsigned int *)malloc(image->extended_width * image->extended_height * 4);
	image->nu_components = 3;
	image->bits_per_component = 8;
	image->srgb = 0;
	image->is_half_float = 0;
	image->is_signed = 0;
        // The internal image format after reading a PNG file is always 32 bits per pixel.
	if (color_type == PNG_COLOR_TYPE_GRAY) {
		image->alpha_bits = 0;
                image->nu_components = 1;
		for (int y = 0; y < image->height; y++)
			for (int x = 0; x < image->width; x++) {
                                unsigned int pixel = (unsigned int)*(row_pointers[y] + x) +
                                    0xFF000000;
				*(image->pixels + y * image->extended_width + x) = pixel;
			}
        }
 	else if (color_type == PNG_COLOR_TYPE_RGB) {
		image->alpha_bits = 0;
		for (int y = 0; y < image->height; y++)
			for (int x = 0; x < image->width; x++) {
				memcpy(image->pixels + y * image->extended_width + x, row_pointers[y] + x * 3, 3);
				// Set the alpha byte to 0xFF.
				*((unsigned char *)&image->pixels[y * image->extended_width + x] + 3) = 0xFF;
			}
	}
	else {
		image->alpha_bits = 8;
		image->nu_components = 4;
		for (int y = 0; y < image->height; y++)
		        memcpy(image->pixels + y * image->extended_width, row_pointers[y], image->width * 4);
	}
	if (image->alpha_bits >= 8) {
		check_1bit_alpha(image);
		if (image->alpha_bits == 1)
			if (!option_quiet)
				printf("1-bit alpha detected.\n");
	}
	pad_image_borders(image);
	for (int y = 0; y < png_height; y++)
		free(row_pointers[y]);
	free(row_pointers);
}

// Save a .png file.

void save_png_file(Image *image, const char *filename) {
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;

	if (image->is_half_float) {
		convert_image_from_half_float(image, 0, 1.0, 1.0);
	}
	else
	if (image->bits_per_component != 8) {
		printf("Error -- cannot write PNG file with non 8-bit components.\n");
		exit(1);
	}

	if (!option_quiet)
		printf("Writing .png file %s.\n", filename);
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		printf("Error - file %s could not be opened for writing.\n", filename);
		exit(1);
	}
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		printf("Error using libpng.\n");
		exit(1);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		printf("Error using libpng.\n");
		exit(1);
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem writing the file. */
		printf("Error writing png file %s.\n", filename);
		exit(1);
	}
	png_init_io(png_ptr, fp);
	int t;
        if (image->nu_components == 1)
		t = PNG_COLOR_TYPE_GRAY;
	else if (image->alpha_bits > 0)
		t = PNG_COLOR_TYPE_RGBA;
	else
		t = PNG_COLOR_TYPE_RGB;
	png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 8, t,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	if (t == PNG_COLOR_TYPE_GRAY || image->alpha_bits == 0)
		// We have RGB (or one component) data in 32-bit pixels with the last byte
                // or bytes unused.
		png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

 	png_byte **row_pointers = (png_byte **)alloca(image->height * sizeof(png_byte *));
	for (int y = 0; y < image->height; y++)
		row_pointers[y] = (png_byte *)(image->pixels + y * image->extended_width);

	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}


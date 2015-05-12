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
#include <stdbool.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

// Draw a 4x4 pixel block using the DXT1 texture compression data in bitstring.

int draw_block4x4_dxt1(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	unsigned int colors = *(unsigned int *)&bitstring[0];
#else
	unsigned int colors = ((unsigned int)bitstring[0] << 24) | ((unsigned int)bitstring[1] << 16) |
		((unsigned int)bitstring[2] << 8) | bitstring[3];
#endif
	// Decode the two 5-6-5 RGB colors.
	int color_r[4], color_g[4], color_b[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	if ((colors & 0xFFFF) > ((colors & 0xFFFF0000) >> 16)) {
		color_r[2] = (2 * color_r[0] + color_r[1]) / 3;
		color_g[2] = (2 * color_g[0] + color_g[1]) / 3;
		color_b[2] = (2 * color_b[0] + color_b[1]) / 3;
		color_r[3] = (color_r[0] + 2 * color_r[1]) / 3;
		color_g[3] = (color_g[0] + 2 * color_g[1]) / 3;
		color_b[3] = (color_b[0] + 2 * color_b[1]) / 3;
	}
	else {
		color_r[2] = (color_r[0] + color_r[1]) / 2;
		color_g[2] = (color_g[0] + color_g[1]) / 2;
		color_b[2] = (color_b[0] + color_b[1]) / 2;
		color_r[3] = color_g[3] = color_b[3] = 0;
	}
	unsigned int pixels = *(unsigned int *)&bitstring[4];
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		image_buffer[i] = pack_rgb_alpha_0xff(color_r[pixel], color_g[pixel], color_b[pixel]);
	}
	return 1;
}

// Draw a 4x4 pixel block using the DXT1A texture compression data in bitstring.

int draw_block4x4_dxt1a(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	unsigned int colors = *(unsigned int *)&bitstring[0];
#else
	unsigned int colors = ((unsigned int)bitstring[0] << 24) | ((unsigned int)bitstring[1] << 16) |
		((unsigned int)bitstring[2] << 8) | bitstring[3];
#endif
	bool opaque = ((colors & 0xFFFF) > ((colors & 0xFFFF0000) >> 16));
	if (opaque && (flags & MODES_ALLOWED_NON_OPAQUE_ONLY))
		return 0;
	if (!opaque && (flags & MODES_ALLOWED_OPAQUE_ONLY))
		return 0;
	// Decode the two 5-6-5 RGB colors.
	int color_r[4], color_g[4], color_b[4], color_a[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	color_a[0] = color_a[1] = color_a[2] = color_a[3] = 0xFF;
	if (opaque) {
		color_r[2] = (2 * color_r[0] + color_r[1]) / 3;
		color_g[2] = (2 * color_g[0] + color_g[1]) / 3;
		color_b[2] = (2 * color_b[0] + color_b[1]) / 3;
		color_r[3] = (color_r[0] + 2 * color_r[1]) / 3;
		color_g[3] = (color_g[0] + 2 * color_g[1]) / 3;
		color_b[3] = (color_b[0] + 2 * color_b[1]) / 3;
	}
	else {
		color_r[2] = (color_r[0] + color_r[1]) / 2;
		color_g[2] = (color_g[0] + color_g[1]) / 2;
		color_b[2] = (color_b[0] + color_b[1]) / 2;
		color_r[3] = color_g[3] = color_b[3] = color_a[3] = 0;
	}
	unsigned int pixels = *(unsigned int *)&bitstring[4];
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		image_buffer[i] = pack_rgba(color_r[pixel], color_g[pixel], color_b[pixel], color_a[pixel]);
	}
	return 1;
}

// Draw a 4x4 pixel block using the DXT3 texture compression data in bitstring.

int draw_block4x4_dxt3(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	unsigned int colors = *(unsigned int *)&bitstring[8];
#else
	unsigned int colors = ((unsigned int)bitstring[8] << 24) | ((unsigned int)bitstring[9] << 16) |
		((unsigned int)bitstring[10] << 8) | bitstring[11];
#endif
	if ((colors & 0xFFFF) <= ((colors & 0xFFFF0000) >> 16) && (flags & ENCODE_BIT))
		// GeForce 6 and 7 series produce wrong result in this case.
		return 0;
	int color_r[4], color_g[4], color_b[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	color_r[2] = (2 * color_r[0] + color_r[1]) / 3;
	color_g[2] = (2 * color_g[0] + color_g[1]) / 3;
	color_b[2] = (2 * color_b[0] + color_b[1]) / 3;
	color_r[3] = (color_r[0] + 2 * color_r[1]) / 3;
	color_g[3] = (color_g[0] + 2 * color_g[1]) / 3;
	color_b[3] = (color_b[0] + 2 * color_b[1]) / 3;
	unsigned int pixels = *(unsigned int *)&bitstring[12];
	uint64_t alpha_pixels = *(uint64_t *)&bitstring[0];
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		int alpha = ((alpha_pixels >> (i * 4)) & 0xF) * 255 / 15;
		image_buffer[i] = pack_rgba(color_r[pixel], color_g[pixel], color_b[pixel], alpha);
	}
	return 1;
}

// Draw a 4x4 pixel block using the DXT5 texture compression data in bitstring.

int draw_block4x4_dxt5(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	int alpha0 = bitstring[0];
	int alpha1 = bitstring[1];
	if (alpha0 > alpha1 && (flags & MODES_ALLOWED_OPAQUE_ONLY))
		return 0;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)
	unsigned int colors = *(unsigned int *)&bitstring[8];
#else
	unsigned int colors = ((unsigned int)bitstring[8] << 24) | ((unsigned int)bitstring[9] << 16) |
		((unsigned int)bitstring[10] << 8) | bitstring[11];
#endif
	if ((colors & 0xFFFF) <= ((colors & 0xFFFF0000) >> 16) && (flags & ENCODE_BIT))
		// GeForce 6 and 7 series produce wrong result in this case.
		return 0;
	int color_r[4], color_g[4], color_b[4];
	color_b[0] = (colors & 0x0000001F) << 3;
	color_g[0] = (colors & 0x000007E0) >> (5 - 2);
	color_r[0] = (colors & 0x0000F800) >> (11 - 3);
	color_b[1] = (colors & 0x001F0000) >> (16 - 3);
	color_g[1] = (colors & 0x07E00000) >> (21 - 2);
	color_r[1] = (colors & 0xF8000000) >> (27 - 3);
	color_r[2] = (2 * color_r[0] + color_r[1]) / 3;
	color_g[2] = (2 * color_g[0] + color_g[1]) / 3;
	color_b[2] = (2 * color_b[0] + color_b[1]) / 3;
	color_r[3] = (color_r[0] + 2 * color_r[1]) / 3;
	color_g[3] = (color_g[0] + 2 * color_g[1]) / 3;
	color_b[3] = (color_b[0] + 2 * color_b[1]) / 3;
	unsigned int pixels = *(unsigned int *)&bitstring[12];
	uint64_t alpha_bits = (unsigned int)bitstring[2] | ((unsigned int)bitstring[3] << 8) |
		((uint64_t)*(unsigned int *)&bitstring[4] << 16);
	for (int i = 0; i < 16; i++) {
		int pixel = (pixels >> (i * 2)) & 0x3;
		int code = (alpha_bits >> (i * 3)) & 0x7;
		int alpha;
		if (alpha0 > alpha1)
			switch (code) {
			case 0 : alpha = alpha0; break;
			case 1 : alpha = alpha1; break;
			case 2 : alpha = (6 * alpha0 + 1 * alpha1) / 7; break;
			case 3 : alpha = (5 * alpha0 + 2 * alpha1) / 7; break;
			case 4 : alpha = (4 * alpha0 + 3 * alpha1) / 7; break;
			case 5 : alpha = (3 * alpha0 + 4 * alpha1) / 7; break;
			case 6 : alpha = (2 * alpha0 + 5 * alpha1) / 7; break;
			case 7 : alpha = (1 * alpha0 + 6 * alpha1) / 7; break;
			}
		else
			switch (code) {
			case 0 : alpha = alpha0; break;
			case 1 : alpha = alpha1; break;
			case 2 : alpha = (4 * alpha0 + 1 * alpha1) / 5; break;
			case 3 : alpha = (3 * alpha0 + 2 * alpha1) / 5; break;
			case 4 : alpha = (2 * alpha0 + 3 * alpha1) / 5; break;
			case 5 : alpha = (1 * alpha0 + 4 * alpha1) / 5; break;
			case 6 : alpha = 0; break;
			case 7 : alpha = 0xFF; break;
			}
		image_buffer[i] = pack_rgba(color_r[pixel], color_g[pixel], color_b[pixel], alpha);
	}
	return 1;
}

// Manual optimization function for the alpha components of DXT3.

void optimize_block_alpha_dxt3(unsigned char *bitstring, unsigned char *alpha_values) {
	uint64_t alpha_pixels = 0;
	for (int i = 0; i < 16; i++) {
		// Scale and round the alpha value to the values allowed by DXT3.
		unsigned int alpha_pixel = ((unsigned int)alpha_values[i] * 15 + 8) / 255;
		alpha_pixels |= (uint64_t)alpha_pixel << (i * 4);
	}
	*(uint64_t *)&bitstring[0] = alpha_pixels;
}



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
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

// Define an array to speed up clamping of values to the ranges 0 to 255.

static unsigned char clamp_table[255 + 256 + 256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
	65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
	81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
	97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
	113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
	129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
	145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
	161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
	177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
	193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
	209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
	225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
	241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

static int complement3bitshifted_table[8] = {
	0, 8, 16, 24, -32, -24, -16, -8
};

static int modifier_table[8][4] = {
	{ 2, 8, -2, -8 },
	{ 5, 17, -5, -17 },
	{ 9, 29, -9, -29 },
	{ 13, 42, -13, -42 },
	{ 18, 60, -18, -60 },
	{ 24, 80, -24, -80 },
	{ 33, 106, -33, -106 },
	{ 47, 183, -47, -183 }
};

static unsigned char clamp(int x) {
	return clamp_table[x + 255];
}

static int clamp_slow(int x) {
	if (x < 0)
		return 0;
	if (x > 255)
		return 255;
	return x;
}

static int clamp2047(int x) {
	if (x < 0)
		return 0;
	if (x > 2047)
		return 2047;
	return x;
}

static int clamp1023_signed(int x) {
	if (x < - 1023)
		return - 1023;
	if (x > 1023)
		return 1023;
	return x;
}

// This function calculates the 3-bit complement value in the range -4 to 3 of a three bit
// representation. The result is arithmetically shifted 3 places to the left before returning.

static int complement3bitshifted(int x) {
	return complement3bitshifted_table[x];
}

static int complement3bitshifted_slow(int x) {
	if (x & 4)
		return ((x & 3) - 4) << 3;	// Note: shift is arithmetic.
	return x << 3;
}

static int complement3bit(int x) {
	if (x & 4)
		return ((x & 3) - 4);
	return x;
}

// Define some macros to speed up ETC1 decoding.

#define do_flipbit0_pixel0to7(val) \
	{ \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = modifier_table[table_codeword1][pixel_index]; \
	r = clamp(base_color_subblock1_R + modifier); \
	g = clamp(base_color_subblock1_G + modifier); \
	b = clamp(base_color_subblock1_B + modifier); \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = pack_rgb_alpha_0xff(r, g, b); \
	}

#define do_flipbit0_pixel8to15(val) \
	{ \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = modifier_table[table_codeword2][pixel_index]; \
	r = clamp(base_color_subblock2_R + modifier); \
	g = clamp(base_color_subblock2_G + modifier); \
	b = clamp(base_color_subblock2_B + modifier); \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = pack_rgb_alpha_0xff(r, g, b); \
	}

#define do_flipbit1_pixelbit1notset(val) \
	 { \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = modifier_table[table_codeword1][pixel_index]; \
	r = clamp(base_color_subblock1_R + modifier); \
	g = clamp(base_color_subblock1_G + modifier); \
	b = clamp(base_color_subblock1_B + modifier); \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = pack_rgb_alpha_0xff(r, g, b); \
	}

#define do_flipbit1_pixelbit1set(val) \
	 { \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = modifier_table[table_codeword2][pixel_index]; \
	r = clamp(base_color_subblock2_R + modifier); \
	g = clamp(base_color_subblock2_G + modifier); \
	b = clamp(base_color_subblock2_B + modifier); \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = pack_rgb_alpha_0xff(r, g, b); \
	}

// Draw a 4x4 pixel block using the ETC1 texture compression data in bitstring. Return 1 is it is a valid block, 0 if
// not (overflow or underflow occurred in differential mode).

int draw_block4x4_etc1(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	int differential_mode = bitstring[3] & 2;
	if (differential_mode) {
		if ((flags & ETC_MODE_ALLOWED_DIFFERENTIAL) == 0)
			return 0;
	}
	else
		if ((flags & ETC_MODE_ALLOWED_INDIVIDUAL) == 0)
			return 0;
	int flipbit = bitstring[3] & 1;
	int base_color_subblock1_R;
	int base_color_subblock1_G;
	int base_color_subblock1_B;
	int base_color_subblock2_R;
	int base_color_subblock2_G;
	int base_color_subblock2_B;
	if (differential_mode) {
		base_color_subblock1_R = (bitstring[0] & 0xF8);
		base_color_subblock1_R |= ((base_color_subblock1_R & 224) >> 5);
		base_color_subblock1_G = (bitstring[1] & 0xF8);
		base_color_subblock1_G |= (base_color_subblock1_G & 224) >> 5;
		base_color_subblock1_B = (bitstring[2] & 0xF8);
		base_color_subblock1_B |= (base_color_subblock1_B & 224) >> 5;
		base_color_subblock2_R = (bitstring[0] & 0xF8);				// 5 highest order bits.
		base_color_subblock2_R += complement3bitshifted(bitstring[0] & 7);	// Add difference.
		if (base_color_subblock2_R & 0xFF07)					// Check for overflow.
			return 0;
		base_color_subblock2_R |= (base_color_subblock2_R & 224) >> 5;		// Replicate.
		base_color_subblock2_G = (bitstring[1] & 0xF8);
		base_color_subblock2_G += complement3bitshifted(bitstring[1] & 7);
		if (base_color_subblock2_G & 0xFF07)
			return 0;
		base_color_subblock2_G |= (base_color_subblock2_G & 224) >> 5;
		base_color_subblock2_B = (bitstring[2] & 0xF8);
		base_color_subblock2_B += complement3bitshifted(bitstring[2] & 7);
		if (base_color_subblock2_B & 0xFF07)
			return 0;
		base_color_subblock2_B |= (base_color_subblock2_B & 224) >> 5;
	}
	else {
		base_color_subblock1_R = (bitstring[0] & 0xF0);
		base_color_subblock1_R |= base_color_subblock1_R >> 4;
		base_color_subblock1_G = (bitstring[1] & 0xF0);
		base_color_subblock1_G |= base_color_subblock1_G >> 4;
		base_color_subblock1_B = (bitstring[2] & 0xF0);
		base_color_subblock1_B |= base_color_subblock1_B >> 4;
		base_color_subblock2_R = (bitstring[0] & 0x0F);
		base_color_subblock2_R |= base_color_subblock2_R << 4;
		base_color_subblock2_G = (bitstring[1] & 0x0F);
		base_color_subblock2_G |= base_color_subblock2_G << 4;
		base_color_subblock2_B = (bitstring[2] & 0x0F);
		base_color_subblock2_B |= base_color_subblock2_B << 4;
	}
	int table_codeword1 = (bitstring[3] & 224) >> 5;
	int table_codeword2 = (bitstring[3] & 28) >> 2;
	unsigned int pixel_index_word = ((unsigned int)bitstring[4] << 24) | ((unsigned int)bitstring[5] << 16) |
		((unsigned int)bitstring[6] << 8) | bitstring[7];
#if 1
	if (flipbit == 0) {
		do_flipbit0_pixel0to7(0);
		do_flipbit0_pixel0to7(1);
		do_flipbit0_pixel0to7(2);
		do_flipbit0_pixel0to7(3);
		do_flipbit0_pixel0to7(4);
		do_flipbit0_pixel0to7(5);
		do_flipbit0_pixel0to7(6);
		do_flipbit0_pixel0to7(7);
		do_flipbit0_pixel8to15(8);
		do_flipbit0_pixel8to15(9);
		do_flipbit0_pixel8to15(10);
		do_flipbit0_pixel8to15(11);
		do_flipbit0_pixel8to15(12);
		do_flipbit0_pixel8to15(13);
		do_flipbit0_pixel8to15(14);
		do_flipbit0_pixel8to15(15);
		return 1;
	}
	else {
		do_flipbit1_pixelbit1notset(0);
		do_flipbit1_pixelbit1notset(1);
		do_flipbit1_pixelbit1set(2);
		do_flipbit1_pixelbit1set(3);
		do_flipbit1_pixelbit1notset(4);
		do_flipbit1_pixelbit1notset(5);
		do_flipbit1_pixelbit1set(6);
		do_flipbit1_pixelbit1set(7);
		do_flipbit1_pixelbit1notset(8);
		do_flipbit1_pixelbit1notset(9);
		do_flipbit1_pixelbit1set(10);
		do_flipbit1_pixelbit1set(11);
		do_flipbit1_pixelbit1notset(12);
		do_flipbit1_pixelbit1notset(13);
		do_flipbit1_pixelbit1set(14);
		do_flipbit1_pixelbit1set(15);
		return 1;
	}
#else
	// Slow unoptimized version.
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)		// Least significant bit.
			| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r, g, b;
		if (flipbit == 0) {
			// Two 2x4 blocks side-by-side.
			if (i < 8) {
				// Subblock 1.
				int modifier = modifier_table[table_codeword1][pixel_index];
				r = clamp(base_color_subblock1_R + modifier);
				g = clamp(base_color_subblock1_G + modifier);
				b = clamp(base_color_subblock1_B + modifier);
			}
			else {
				// Subblock 2.
				int modifier = modifier_table[table_codeword2][pixel_index];
				r = clamp(base_color_subblock2_R + modifier);
				g = clamp(base_color_subblock2_G + modifier);
				b = clamp(base_color_subblock2_B + modifier);
			}
		}
		else {
			// Two 4x2 blocks on top of each other.
			if ((i & 2) == 0) {
				// Subblock 1.
				int modifier = modifier_table[table_codeword1][pixel_index];
				r = clamp(base_color_subblock1_R + modifier);
				g = clamp(base_color_subblock1_G + modifier);
				b = clamp(base_color_subblock1_B + modifier);
			}
			else {
				// Subblock 2.
				int modifier = modifier_table[table_codeword2][pixel_index];
				r = clamp(base_color_subblock2_R + modifier);
				g = clamp(base_color_subblock2_G + modifier);
				b = clamp(base_color_subblock2_B + modifier);
			}
		}
		image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = pack_rgb_alpha_0xff(r, g, b);
	}
	return 1;
#endif
}

static int etc2_distance_table[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };

static void draw_block4x4_rgb8_etc2_T_or_H_mode(const unsigned char *bitstring, unsigned int *image_buffer, int mode) {
	int base_color1_R, base_color1_G, base_color1_B;
	int base_color2_R, base_color2_G, base_color2_B;
	int paint_color_R[4], paint_color_G[4], paint_color_B[4];
	int distance;
	if (mode == 0) {
		// T mode.
		base_color1_R = ((bitstring[0] & 0x18) >> 1) | (bitstring[0] & 0x3);
		base_color1_R |= base_color1_R << 4;
		base_color1_G = bitstring[1] & 0xF0;
		base_color1_G |= base_color1_G >> 4;
		base_color1_B = bitstring[1] & 0x0F;
		base_color1_B |= base_color1_B << 4;
		base_color2_R = bitstring[2] & 0xF0;
		base_color2_R |= base_color2_R >> 4;
		base_color2_G = bitstring[2] & 0x0F;
		base_color2_G |= base_color2_G << 4;
		base_color2_B = bitstring[3] & 0xF0;
		base_color2_B |= base_color2_B >> 4;
		// index = (da << 1) | db
		distance = etc2_distance_table[((bitstring[3] & 0x0C) >> 1) | (bitstring[3] & 0x1)];
		paint_color_R[0] = base_color1_R;
		paint_color_G[0] = base_color1_G;
		paint_color_B[0] = base_color1_B;
		paint_color_R[2] = base_color2_R;
		paint_color_G[2] = base_color2_G;
		paint_color_B[2] = base_color2_B;
		paint_color_R[1] = clamp(base_color2_R + distance);
		paint_color_G[1] = clamp(base_color2_G + distance);
		paint_color_B[1] = clamp(base_color2_B + distance);
		paint_color_R[3] = clamp(base_color2_R - distance);
		paint_color_G[3] = clamp(base_color2_G - distance);
		paint_color_B[3] = clamp(base_color2_B - distance);
	}
	else {
		// H mode.
		base_color1_R = (bitstring[0] & 0x78) >> 3;
		base_color1_R |= base_color1_R << 4;
		base_color1_G = ((bitstring[0] & 0x07) << 1) | ((bitstring[1] & 0x10) >> 4);
		base_color1_G |= base_color1_G << 4;
		base_color1_B = (bitstring[1] & 0x08) | ((bitstring[1] & 0x03) << 1) | ((bitstring[2] & 0x80) >> 7);
		base_color1_B |= base_color1_B << 4;
		base_color2_R = (bitstring[2] & 0x78) >> 3;
		base_color2_R |= base_color2_R << 4;
		base_color2_G = ((bitstring[2] & 0x07) << 1) | ((bitstring[3] & 0x80) >> 7);
		base_color2_G |= base_color2_G << 4;
		base_color2_B = (bitstring[3] & 0x78) >> 3;
		base_color2_B |= base_color2_B << 4;
		// da is most significant bit, db is middle bit, least significant bit is
		// (base_color1 value >= base_color2 value).
		int base_color1_value = (base_color1_R << 16) + (base_color1_G << 8) + base_color1_B;
		int base_color2_value = (base_color2_R << 16) + (base_color2_G << 8) + base_color2_B;
		int bit;
		if (base_color1_value >= base_color2_value)
			bit = 1;
		else
			bit = 0;
		distance = etc2_distance_table[(bitstring[3] & 0x04) | ((bitstring[3] & 0x01) << 1) | bit];
		paint_color_R[0] = clamp(base_color1_R + distance);
		paint_color_G[0] = clamp(base_color1_G + distance);
		paint_color_B[0] = clamp(base_color1_B + distance);
		paint_color_R[1] = clamp(base_color1_R - distance);
		paint_color_G[1] = clamp(base_color1_G - distance);
		paint_color_B[1] = clamp(base_color1_B - distance);
		paint_color_R[2] = clamp(base_color2_R + distance);
		paint_color_G[2] = clamp(base_color2_G + distance);
		paint_color_B[2] = clamp(base_color2_B + distance);
		paint_color_R[3] = clamp(base_color2_R - distance);
		paint_color_G[3] = clamp(base_color2_G - distance);
		paint_color_B[3] = clamp(base_color2_B - distance);
	}
	unsigned int pixel_index_word = ((unsigned int)bitstring[4] << 24) | ((unsigned int)bitstring[5] << 16) |
		((unsigned int)bitstring[6] << 8) | bitstring[7];
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
			| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r = paint_color_R[pixel_index];
		int g = paint_color_G[pixel_index];
		int b = paint_color_B[pixel_index];
		image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = pack_rgb_alpha_0xff(r, g, b);
	}
}

static void draw_block4x4_rgb8_etc2_planar_mode(const unsigned char *bitstring, unsigned int *image_buffer) {
	// Each color O, H and V is in 6-7-6 format.
	int RO = (bitstring[0] & 0x7E) >> 1;
	int GO = ((bitstring[0] & 0x1) << 6) | ((bitstring[1] & 0x7E) >> 1);
	int BO = ((bitstring[1] & 0x1) << 5) | (bitstring[2] & 0x18) | ((bitstring[2] & 0x03) << 1) |
		((bitstring[3] & 0x80) >> 7);
	int RH = ((bitstring[3] & 0x7C) >> 1) | (bitstring[3] & 0x1);
	int GH = (bitstring[4] & 0xFE) >> 1;
	int BH = ((bitstring[4] & 0x1) << 5) | ((bitstring[5] & 0xF8) >> 3);
	int RV = ((bitstring[5] & 0x7) << 3) | ((bitstring[6] & 0xE0) >> 5);
	int GV = ((bitstring[6] & 0x1F) << 2) | ((bitstring[7] & 0xC0) >> 6);
	int BV = bitstring[7] & 0x3F;
	RO = (RO << 2) | ((RO & 0x30) >> 4);	// Replicate bits.
	GO = (GO << 1) | ((GO & 0x40) >> 6);
	BO = (BO << 2) | ((BO & 0x30) >> 4);
	RH = (RH << 2) | ((RH & 0x30) >> 4);
	GH = (GH << 1) | ((GH & 0x40) >> 6);
	BH = (BH << 2) | ((BH & 0x30) >> 4);
	RV = (RV << 2) | ((RV & 0x30) >> 4);
	GV = (GV << 1) | ((GV & 0x40) >> 6);
	BV = (BV << 2) | ((BV & 0x30) >> 4);
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++) {
			int r = clamp((x * (RH - RO) + y * (RV - RO) + 4 * RO + 2) >> 2);
			int g = clamp((x * (GH - GO) + y * (GV - GO) + 4 * GO + 2) >> 2);
			int b = clamp((x * (BH - BO) + y * (BV - BO) + 4 * BO + 2) >> 2);
			image_buffer[y * 4 + x] = pack_rgb_alpha_0xff(r, g, b);
		}
}

// Draw a 4x4 pixel block using the ETC2 texture compression data in bitstring (format COMPRESSED_RGB8_ETC2).

int draw_block4x4_etc2_rgb8(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	// Figure out the mode.
	if ((bitstring[3] & 2) == 0) {
		// Individual mode.
		return draw_block4x4_etc1(bitstring, image_buffer, flags);
	}
	if ((flags & ((~ETC_MODE_ALLOWED_INDIVIDUAL) ^ ENCODE_BIT)) == 0)
		return 0;
#if 0
	int R = (bitstring[0] & 0xF8) >> 3;;
	R += complement3bit(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8) >> 3;
	G += complement3bit(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8) >> 3;
	B += complement3bit(bitstring[2] & 7);
	if (R < 0 || R > 31) {
		// T mode.
		if ((flags & ETC2_MODE_ALLOWED_T) == 0)
			return 0;
		draw_block4x4_rgb8_etc2_T_or_H_mode(bitstring, image_buffer, 0);
		return 1;
	}
	else
	if (G < 0 || G > 31) {
		// H mode.
		if ((flags & ETC2_MODE_ALLOWED_H) == 0)
			return 0;
		draw_block4x4_rgb8_etc2_T_or_H_mode(bitstring, image_buffer, 1);
		return 1;
	}
	else
	if (B < 0 || B > 31) {
		// Planar mode.
		if ((flags & ETC2_MODE_ALLOWED_PLANAR) == 0)
			return 0;
		draw_block4x4_rgb8_etc2_planar_mode(bitstring, image_buffer);
		return 1;
	}
	else {
		// Differential mode.
		return draw_block4x4_etc1(bitstring, image_buffer, flags);
	}

#else
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	if (R & 0xFF07) {
		// T mode.
		if ((flags & ETC2_MODE_ALLOWED_T) == 0)
			return 0;
		draw_block4x4_rgb8_etc2_T_or_H_mode(bitstring, image_buffer, 0);
		return 1;
	}
	else
	if (G & 0xFF07) {
		// H mode.
		if ((flags & ETC2_MODE_ALLOWED_H) == 0)
			return 0;
		draw_block4x4_rgb8_etc2_T_or_H_mode(bitstring, image_buffer, 1);
		return 1;
	}
	else
	if (B & 0xFF07) {
		// Planar mode.
		if ((flags & ETC2_MODE_ALLOWED_PLANAR) == 0)
			return 0;
		draw_block4x4_rgb8_etc2_planar_mode(bitstring, image_buffer);
		return 1;
	}
	else {
		// Differential mode.
		return draw_block4x4_etc1(bitstring, image_buffer, flags);
	}
#endif
}

// Determine the mode of the ETC1 RGB8 block.
// Return one of the following values:
// 0	Individual mode
// 1	Differential mode

int block4x4_etc1_get_mode(const unsigned char *bitstring) {
	// Figure out the mode.
	if ((bitstring[3] & 2) == 0)
		// Individual mode.
		return 0;
	else
		return 1;
}

// Determine the mode of the ETC2 RGB8 block.
// Return one of the following values:
// 0	Individual mode
// 1	Differential mode
// 2	T mode
// 3	H mode
// 4	Planar mode	

int block4x4_etc2_rgb8_get_mode(const unsigned char *bitstring) {
	// Figure out the mode.
	if ((bitstring[3] & 2) == 0)
		// Individual mode.
		return 0;
#if 0
	int R = (bitstring[0] & 0xF8) >> 3;;
	R += complement3bit(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8) >> 3;
	G += complement3bit(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8) >> 3;
	B += complement3bit(bitstring[2] & 7);
	if (R < 0 || R > 31)
		// T mode.
		return 2;
	else
	if (G < 0 || G > 31)
		// H mode.
		return 3;
	else
	if (B < 0 || B > 31)
		// Planar mode.
		return 4;
	else
		// Differential mode.
		return 1;
#else
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	if (R & 0xFF07)
		// T mode.
		return 2;
	else
	if (G & 0xFF07)
		// H mode.
		return 3;
	else
	if (B & 0xFF07)
		// Planar mode.
		return 4;
	else
		// Differential mode.
		return 1;
#endif
}


static char eac_modifier_table[16][8] = {
	{ -3, -6, -9, -15, 2, 5, 8, 14 },
	{ -3, -7, -10, -13, 2, 6, 9, 12 },
	{ -2, -5, -8, -13, 1, 4, 7, 12 },
	{ -2, -4, -6, -13, 1, 3, 5, 12 },
	{ -3, -6, -8, -12, 2, 5, 7, 11 },
	{ -3, -7, -9, -11, 2, 6, 8, 10 },
	{ -4, -7, -8, -11, 3, 6, 7, 10 },
	{ -3, -5, -8, -11, 2, 4, 7, 10 },
	{ -2, -6, -8, -10, 1, 5, 7, 9 },
	{ -2, -5, -8, -10, 1, 4, 7, 9 },
	{ -2, -4, -8, -10, 1, 3, 7, 9 },
	{ -2, -5, -7, -10, 1, 4, 6, 9 },
	{ -3, -4, -7, -10, 2, 3, 6, 9 },
	{ -1, -2, -3, -10, 0, 1, 2, 9 },
	{ -4, -6, -8, -9, 3, 5, 7, 8 },
	{ -3, -5, -7, -9, 2, 4, 6, 8 }
};

#if 0
// This table is currently not used.
static short int modifier_times_multiplier_table[30][16] = {
	{ 0, -15, -30, -45, -60, -75, -90, -105, -120, -135, -150, -165, -180, -195, -210, -225 },
	{ 0, -14, -28, -42, -56, -70, -84, -98, -112, -126, -140, -154, -168, -182, -196, -210 },
	{ 0, -13, -26, -39, -52, -65, -78, -91, -104, -117, -130, -143, -156, -169, -182, -195 },
	{ 0, -12, -24, -36, -48, -60, -72, -84, -96, -108, -120, -132, -144, -156, -168, -180 },
	{ 0, -11, -22, -33, -44, -55, -66, -77, -88, -99, -110, -121, -132, -143, -154, -165 },
	{ 0, -10, -20, -30, -40, -50, -60, -70, -80, -90, -100, -110, -120, -130, -140, -150 },
	{ 0, -9, -18, -27, -36, -45, -54, -63, -72, -81, -90, -99, -108, -117, -126, -135 },
	{ 0, -8, -16, -24, -32, -40, -48, -56, -64, -72, -80, -88, -96, -104, -112, -120 },
	{ 0, -7, -14, -21, -28, -35, -42, -49, -56, -63, -70, -77, -84, -91, -98, -105 },
	{ 0, -6, -12, -18, -24, -30, -36, -42, -48, -54, -60, -66, -72, -78, -84, -90 },
	{ 0, -5, -10, -15, -20, -25, -30, -35, -40, -45, -50, -55, -60, -65, -70, -75 },
	{ 0, -4, -8, -12, -16, -20, -24, -28, -32, -36, -40, -44, -48, -52, -56, -60 },
	{ 0, -3, -6, -9, -12, -15, -18, -21, -24, -27, -30, -33, -36, -39, -42, -45 },
	{ 0, -2, -4, -6, -8, -10, -12, -14, -16, -18, -20, -22, -24, -26, -28, -30 },
	{ 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 },
	{ 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45 },
	{ 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75 },
	{ 0, 6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90 },
	{ 0, 7, 14, 21, 28, 35, 42, 49, 56, 63, 70, 77, 84, 91, 98, 105 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 },
	{ 0, 9, 18, 27, 36, 45, 54, 63, 72, 81, 90, 99, 108, 117, 126, 135 },
	{ 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150 },
	{ 0, 11, 22, 33, 44, 55, 66, 77, 88, 99, 110, 121, 132, 143, 154, 165 },
	{ 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180 },
	{ 0, 13, 26, 39, 52, 65, 78, 91, 104, 117, 130, 143, 156, 169, 182, 195 },
	{ 0, 14, 28, 42, 56, 70, 84, 98, 112, 126, 140, 154, 168, 182, 196, 210 },
};

static int modifier_times_multiplier_lookup(int modifier, int multiplier) {
	return modifier_times_multiplier_table[modifier + 15][multiplier];
}
#endif

static int modifier_times_multiplier(int modifier, int multiplier) {
	return modifier * multiplier;
}

#define do_eac_pixel(val) \
	{ \
	int i  = val; \
	int modifier = modifier_table[(pixels >> (45 - i * 3)) & 7]; \
	*((unsigned char *)&image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] + alpha_byte_offset) = \
		clamp(base_codeword + modifier_times_multiplier(modifier, multiplier)); \
	}

// Draw a 4x4 pixel block using 128-bit ETC2 EAC compression data.
// This might slow on 32-bit architectures.

int draw_block4x4_etc2_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	int r = draw_block4x4_etc2_rgb8(&bitstring[8], image_buffer, flags);
	if (r == 0)
		return 0;
	// Decode the alpha part.
	int base_codeword = bitstring[0];
	char *modifier_table = eac_modifier_table[(bitstring[1] & 0x0F)];
	int multiplier = (bitstring[1] & 0xF0) >> 4;
	if (multiplier == 0 && (flags & ENCODE_BIT))
		// Not allowed in encoding. Decoder should handle it.
		return 0;
	uint64_t pixels = ((uint64_t)bitstring[2] << 40) | ((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24)
		| ((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
#if 1
	do_eac_pixel(0);
	do_eac_pixel(1);
	do_eac_pixel(2);
	do_eac_pixel(3);
	do_eac_pixel(4);
	do_eac_pixel(5);
	do_eac_pixel(6);
	do_eac_pixel(7);
	do_eac_pixel(8);
	do_eac_pixel(9);
	do_eac_pixel(10);
	do_eac_pixel(11);
	do_eac_pixel(12);
	do_eac_pixel(13);
	do_eac_pixel(14);
	do_eac_pixel(15);
#else
	for (int i = 0; i < 16; i++) {
		int modifier = modifier_table[(pixels >> (45 - i * 3)) & 7];
		*((unsigned char *)&image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] + alpha_byte_offset) =
			clamp(base_codeword + modifier_times_multiplier(modifier, multiplier));
	}
#endif
	return 1;
}

int block4x4_etc2_eac_get_mode(const unsigned char *bitstring) {
	return block4x4_etc2_rgb8_get_mode(&bitstring[8]);
}

static int punchthrough_modifier_table[8][4] = {
	{ 0, 8, 0, -8 },
	{ 0, 17, 0, -17 },
	{ 0, 29, 0, -29 },
	{ 0, 42, 0, -42 },
	{ 0, 60, 0, -60 },
	{ 0, 80, 0, -80 },
	{ 0, 106, 0, -106 },
	{ 0, 183, 0, -183 }
};

static unsigned int punchthrough_mask_table[4] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF };

#define do_flipbit0_pixel0to7_punchthrough(val) \
	{ \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	unsigned int a = 0xFF000000; \
	int r, g, b; \
	int modifier = punchthrough_modifier_table[table_codeword1][pixel_index]; \
	r = clamp(base_color_subblock1_R + modifier); \
	g = clamp(base_color_subblock1_G + modifier); \
	b = clamp(base_color_subblock1_B + modifier); \
	unsigned int mask = punchthrough_mask_table[pixel_index]; \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (pack_rgb_alpha_0xff(r, g, b)) & mask; \
	}

#define do_flipbit0_pixel8to15_punchthrough(val) \
	{ \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = punchthrough_modifier_table[table_codeword2][pixel_index]; \
	r = clamp(base_color_subblock2_R + modifier); \
	g = clamp(base_color_subblock2_G + modifier); \
	b = clamp(base_color_subblock2_B + modifier); \
	unsigned int mask = punchthrough_mask_table[pixel_index]; \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (pack_rgb_alpha_0xff(r, g, b)) & mask; \
	}

#define do_flipbit1_pixelbit1notset_punchthrough(val) \
	 { \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = punchthrough_modifier_table[table_codeword1][pixel_index]; \
	r = clamp(base_color_subblock1_R + modifier); \
	g = clamp(base_color_subblock1_G + modifier); \
	b = clamp(base_color_subblock1_B + modifier); \
	unsigned int mask = punchthrough_mask_table[pixel_index]; \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (pack_rgb_alpha_0xff(r, g, b)) & mask; \
	}

#define do_flipbit1_pixelbit1set_punchthrough(val) \
	 { \
	int i = val; \
	int pixel_index = ((pixel_index_word & (1 << i)) >> i) \
		| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1)); \
	int r, g, b; \
	int modifier = punchthrough_modifier_table[table_codeword2][pixel_index]; \
	r = clamp(base_color_subblock2_R + modifier); \
	g = clamp(base_color_subblock2_G + modifier); \
	b = clamp(base_color_subblock2_B + modifier); \
	unsigned int mask = punchthrough_mask_table[pixel_index]; \
	image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (pack_rgb_alpha_0xff(r, g, b)) & mask; \
	}

void draw_block4x4_etc2_punchthrough_differential(const unsigned char *bitstring, unsigned int *image_buffer) {
	int flipbit = bitstring[3] & 1;
	int base_color_subblock1_R;
	int base_color_subblock1_G;
	int base_color_subblock1_B;
	int base_color_subblock2_R;
	int base_color_subblock2_G;
	int base_color_subblock2_B;
	base_color_subblock1_R = (bitstring[0] & 0xF8);
	base_color_subblock1_R |= ((base_color_subblock1_R & 224) >> 5);
	base_color_subblock1_G = (bitstring[1] & 0xF8);
	base_color_subblock1_G |= (base_color_subblock1_G & 224) >> 5;
	base_color_subblock1_B = (bitstring[2] & 0xF8);
	base_color_subblock1_B |= (base_color_subblock1_B & 224) >> 5;
	base_color_subblock2_R = (bitstring[0] & 0xF8);				// 5 highest order bits.
	base_color_subblock2_R += complement3bitshifted(bitstring[0] & 7);	// Add difference.
	base_color_subblock2_R |= (base_color_subblock2_R & 224) >> 5;		// Replicate.
	base_color_subblock2_G = (bitstring[1] & 0xF8);
	base_color_subblock2_G += complement3bitshifted(bitstring[1] & 7);
	base_color_subblock2_G |= (base_color_subblock2_G & 224) >> 5;
	base_color_subblock2_B = (bitstring[2] & 0xF8);
	base_color_subblock2_B += complement3bitshifted(bitstring[2] & 7);
	base_color_subblock2_B |= (base_color_subblock2_B & 224) >> 5;
	int table_codeword1 = (bitstring[3] & 224) >> 5;
	int table_codeword2 = (bitstring[3] & 28) >> 2;
	unsigned int pixel_index_word = ((unsigned int)bitstring[4] << 24) | ((unsigned int)bitstring[5] << 16) |
		((unsigned int)bitstring[6] << 8) | bitstring[7];
	if (flipbit == 0) {
		do_flipbit0_pixel0to7_punchthrough(0);
		do_flipbit0_pixel0to7_punchthrough(1);
		do_flipbit0_pixel0to7_punchthrough(2);
		do_flipbit0_pixel0to7_punchthrough(3);
		do_flipbit0_pixel0to7_punchthrough(4);
		do_flipbit0_pixel0to7_punchthrough(5);
		do_flipbit0_pixel0to7_punchthrough(6);
		do_flipbit0_pixel0to7_punchthrough(7);
		do_flipbit0_pixel8to15_punchthrough(8);
		do_flipbit0_pixel8to15_punchthrough(9);
		do_flipbit0_pixel8to15_punchthrough(10);
		do_flipbit0_pixel8to15_punchthrough(11);
		do_flipbit0_pixel8to15_punchthrough(12);
		do_flipbit0_pixel8to15_punchthrough(13);
		do_flipbit0_pixel8to15_punchthrough(14);
		do_flipbit0_pixel8to15_punchthrough(15);
		return;
	}
	else {
		do_flipbit1_pixelbit1notset_punchthrough(0);
		do_flipbit1_pixelbit1notset_punchthrough(1);
		do_flipbit1_pixelbit1set_punchthrough(2);
		do_flipbit1_pixelbit1set_punchthrough(3);
		do_flipbit1_pixelbit1notset_punchthrough(4);
		do_flipbit1_pixelbit1notset_punchthrough(5);
		do_flipbit1_pixelbit1set_punchthrough(6);
		do_flipbit1_pixelbit1set_punchthrough(7);
		do_flipbit1_pixelbit1notset_punchthrough(8);
		do_flipbit1_pixelbit1notset_punchthrough(9);
		do_flipbit1_pixelbit1set_punchthrough(10);
		do_flipbit1_pixelbit1set_punchthrough(11);
		do_flipbit1_pixelbit1notset_punchthrough(12);
		do_flipbit1_pixelbit1notset_punchthrough(13);
		do_flipbit1_pixelbit1set_punchthrough(14);
		do_flipbit1_pixelbit1set_punchthrough(15);
		return;
	}
}

void draw_block4x4_etc2_punchthrough_T_or_H_mode(const unsigned char *bitstring, unsigned int *image_buffer, int mode) {
	int base_color1_R, base_color1_G, base_color1_B;
	int base_color2_R, base_color2_G, base_color2_B;
	int paint_color_R[4], paint_color_G[4], paint_color_B[4];
	int distance;
	if (mode == 0) {
		// T mode.
		base_color1_R = ((bitstring[0] & 0x18) >> 1) | (bitstring[0] & 0x3);
		base_color1_R |= base_color1_R << 4;
		base_color1_G = bitstring[1] & 0xF0;
		base_color1_G |= base_color1_G >> 4;
		base_color1_B = bitstring[1] & 0x0F;
		base_color1_B |= base_color1_B << 4;
		base_color2_R = bitstring[2] & 0xF0;
		base_color2_R |= base_color2_R >> 4;
		base_color2_G = bitstring[2] & 0x0F;
		base_color2_G |= base_color2_G << 4;
		base_color2_B = bitstring[3] & 0xF0;
		base_color2_B |= base_color2_B >> 4;
		// index = (da << 1) | db
		distance = etc2_distance_table[((bitstring[3] & 0x0C) >> 1) | (bitstring[3] & 0x1)];
		paint_color_R[0] = base_color1_R;
		paint_color_G[0] = base_color1_G;
		paint_color_B[0] = base_color1_B;
		paint_color_R[2] = base_color2_R;
		paint_color_G[2] = base_color2_G;
		paint_color_B[2] = base_color2_B;
		paint_color_R[1] = clamp(base_color2_R + distance);
		paint_color_G[1] = clamp(base_color2_G + distance);
		paint_color_B[1] = clamp(base_color2_B + distance);
		paint_color_R[3] = clamp(base_color2_R - distance);
		paint_color_G[3] = clamp(base_color2_G - distance);
		paint_color_B[3] = clamp(base_color2_B - distance);
	}
	else {
		// H mode.
		base_color1_R = (bitstring[0] & 0x78) >> 3;
		base_color1_R |= base_color1_R << 4;
		base_color1_G = ((bitstring[0] & 0x07) << 1) | ((bitstring[1] & 0x10) >> 4);
		base_color1_G |= base_color1_G << 4;
		base_color1_B = (bitstring[1] & 0x08) | ((bitstring[1] & 0x03) << 1) | ((bitstring[2] & 0x80) >> 7);
		base_color1_B |= base_color1_B << 4;
		base_color2_R = (bitstring[2] & 0x78) >> 3;
		base_color2_R |= base_color2_R << 4;
		base_color2_G = ((bitstring[2] & 0x07) << 1) | ((bitstring[3] & 0x80) >> 7);
		base_color2_G |= base_color2_G << 4;
		base_color2_B = (bitstring[3] & 0x78) >> 3;
		base_color2_B |= base_color2_B << 4;
		// da is most significant bit, db is middle bit, least significant bit is
		// (base_color1 value >= base_color2 value).
		int base_color1_value = (base_color1_R << 16) + (base_color1_G << 8) + base_color1_B;
		int base_color2_value = (base_color2_R << 16) + (base_color2_G << 8) + base_color2_B;
		int bit;
		if (base_color1_value >= base_color2_value)
			bit = 1;
		else
			bit = 0;
		distance = etc2_distance_table[(bitstring[3] & 0x04) | ((bitstring[3] & 0x01) << 1) | bit];
		paint_color_R[0] = clamp(base_color1_R + distance);
		paint_color_G[0] = clamp(base_color1_G + distance);
		paint_color_B[0] = clamp(base_color1_B + distance);
		paint_color_R[1] = clamp(base_color1_R - distance);
		paint_color_G[1] = clamp(base_color1_G - distance);
		paint_color_B[1] = clamp(base_color1_B - distance);
		paint_color_R[2] = clamp(base_color2_R + distance);
		paint_color_G[2] = clamp(base_color2_G + distance);
		paint_color_B[2] = clamp(base_color2_B + distance);
		paint_color_R[3] = clamp(base_color2_R - distance);
		paint_color_G[3] = clamp(base_color2_G - distance);
		paint_color_B[3] = clamp(base_color2_B - distance);
	}
	unsigned int pixel_index_word = ((unsigned int)bitstring[4] << 24) | ((unsigned int)bitstring[5] << 16) |
		((unsigned int)bitstring[6] << 8) | bitstring[7];
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
			| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r = paint_color_R[pixel_index];
		int g = paint_color_G[pixel_index];
		int b = paint_color_B[pixel_index];
		unsigned int mask = punchthrough_mask_table[pixel_index];
		image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (pack_rgb_alpha_0xff(r, g, b)) & mask;
	}
}

int block4x4_etc2_punchthrough_get_mode(const unsigned char *bitstring) {
	// Figure out the mode.
	int opaque = bitstring[3] & 2;
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	if (R & 0xFF07)
		// T mode.
		return 2;
	else if (G & 0xFF07)
		// H mode.
		return 3;
	else if (B & 0xFF07)
		// Planar mode.
		return 4;
	else
		// Differential mode.
		return 1;
}

// Draw a 4x4 pixel block using 64-bit ETC2 PUNCHTHROUGH ALPHA compression data.

int draw_block4x4_etc2_punchthrough(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	int opaque = bitstring[3] & 2;
	if (opaque && (flags & MODES_ALLOWED_NON_OPAQUE_ONLY))
		return 0;
	if (!opaque && (flags & MODES_ALLOWED_OPAQUE_ONLY))
		return 0;
	if (R & 0xFF07) {
		// T mode.
		if ((flags & ETC2_MODE_ALLOWED_T) == 0)
			return 0;
		if (opaque) {
			draw_block4x4_rgb8_etc2_T_or_H_mode(bitstring, image_buffer, 0);
			return 1;
		}
		// T mode with punchthrough alpha.
		draw_block4x4_etc2_punchthrough_T_or_H_mode(bitstring, image_buffer, 0);
		return 1;
	}
	else
	if (G & 0xFF07) {
		// H mode.
		if ((flags & ETC2_MODE_ALLOWED_H) == 0)
			return 0;
		if (opaque) {
			draw_block4x4_rgb8_etc2_T_or_H_mode(bitstring, image_buffer, 1);
			return 1;
		}
		// H mode with punchthrough alpha.
		draw_block4x4_etc2_punchthrough_T_or_H_mode(bitstring, image_buffer, 1);
		return 1;
	}
	else
	if (B & 0xFF07) {
		// Planar mode.
		if ((flags & ETC2_MODE_ALLOWED_PLANAR) == 0)
			return 0;
		// Opaque always set.
		if (flags & MODES_ALLOWED_NON_OPAQUE_ONLY)
			return 0;
		draw_block4x4_rgb8_etc2_planar_mode(bitstring, image_buffer);
		return 1;
	}
	else {
		// Differential mode.
		if (opaque)
			return draw_block4x4_etc1(bitstring, image_buffer, flags);
		// Differential mode with punchthrough alpha.
		if ((flags & ETC_MODE_ALLOWED_DIFFERENTIAL) == 0)
			return 0;
		draw_block4x4_etc2_punchthrough_differential(bitstring, image_buffer);
		return 1;
	}
}

// Decode an 11-bit integer and store it in the first 16-bits in memory of each pixel in image_buffer if offset is zero,
// in the last 16 bits in memory if offset is 1.

void decode_block4x4_11bits(uint64_t qword, unsigned int *image_buffer, int offset) {
	int base_codeword_times_8_plus_4 = ((qword & 0xFF00000000000000) >> (56 - 3)) | 0x4;
	int modifier_index = (qword & 0x000F000000000000) >> 48;
	char *modifier_table = eac_modifier_table[modifier_index];
	int multiplier_times_8 = (qword & 0x00F0000000000000) >> (52 - 3);
	if (multiplier_times_8 == 0)
		for (int i = 0; i < 16; i++) {
			int pixel_index = (qword & (0x0000E00000000000 >> (i * 3))) >> (45 - i * 3);
			int modifier = modifier_table[pixel_index];
			unsigned int value = clamp2047(base_codeword_times_8_plus_4 + modifier);
			*((unsigned short *)&image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] + offset) =
				(value << 5) | (value >> 6);	// Replicate bits to 16-bit.
		}
	else
		for (int i = 0; i < 16; i++) {
			int pixel_index = (qword & (0x0000E00000000000 >> (i * 3))) >> (45 - i * 3);
			int modifier = modifier_table[pixel_index];
			unsigned int value = clamp2047(base_codeword_times_8_plus_4 + modifier * multiplier_times_8);
			*((unsigned short *)&image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] + offset) =
				(value << 5) | (value >> 6);	// Replicate bits to 16-bit.
		}
}

// Draw a 4x4 pixel block using 64-bit ETC2 R11_EAC compression data.

int draw_block4x4_r11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int offset) {
	memset(image_buffer, 0, 64);
	uint64_t qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) | ((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	decode_block4x4_11bits(qword, image_buffer, 0);
	return 1;
}

// Draw a 4x4 pixel block using 128-bit ETC2 RG11_EAC compression data.

int draw_block4x4_rg11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	uint64_t red_qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) | ((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	decode_block4x4_11bits(red_qword, image_buffer, 0);
	uint64_t green_qword = ((uint64_t)bitstring[8] << 56) | ((uint64_t)bitstring[9] << 48) | ((uint64_t)bitstring[10] << 40) |
		((uint64_t)bitstring[11] << 32) | ((uint64_t)bitstring[12] << 24) |
		((uint64_t)bitstring[13] << 16) | ((uint64_t)bitstring[14] << 8) | bitstring[15];
	decode_block4x4_11bits(green_qword, image_buffer, 1);
	return 1;
}

static unsigned int replicate_11bits_signed_to_16bits(int value) {
	if (value >= 0)
		return (value << 5) | (value >> 5);
	value = - value;
	value = (value << 5) | (value >> 5);
	return - value;
}

// Decode an 11-bit signed integer and store it in the first 16-bits in memory of each pixel in image_buffer if offset is
// zero, in the last 16 bits in memory if offset is 1.

int decode_block4x4_11bits_signed(uint64_t qword, unsigned int *image_buffer, int offset, int flags) {
	int base_codeword = (char)((qword & 0xFF00000000000000) >> 56);		// Signed 8 bits.
	if (base_codeword == - 128 /* && (flags & ENCODE_BIT) */)
		// Not allowed in encoding. Decoder should handle it but we don't do that yet.
		return 0;
	int base_codeword_times_8 = base_codeword << 3;				// Arithmetic shift.
	int modifier_index = (qword & 0x000F000000000000) >> 48;
	char *modifier_table = eac_modifier_table[modifier_index];
	int multiplier_times_8 = (qword & 0x00F0000000000000) >> (52 - 3);
	if (multiplier_times_8 == 0)
		for (int i = 0; i < 16; i++) {
			int pixel_index = (qword & (0x0000E00000000000 >> (i * 3))) >> (45 - i * 3);
			int modifier = modifier_table[pixel_index];
			int value = clamp1023_signed(base_codeword_times_8 + modifier);
			unsigned int bits = replicate_11bits_signed_to_16bits(value);
			*((unsigned short *)&image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] + offset) = bits;
		}
	else
		for (int i = 0; i < 16; i++) {
			int pixel_index = (qword & (0x0000E00000000000 >> (i * 3))) >> (45 - i * 3);
			int modifier = modifier_table[pixel_index];
			int value = clamp1023_signed(base_codeword_times_8 + modifier * multiplier_times_8);
			unsigned int bits = replicate_11bits_signed_to_16bits(value);
			*((unsigned short *)&image_buffer[(i & 3) * 4 + ((i & 12) >> 2)] + offset) = bits;
		}
	return 1;
}

// Draw a 4x4 pixel block using 64-bit ETC2 SIGNED_R11_EAC compression data.

int draw_block4x4_signed_r11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	memset(image_buffer, 0, 64);
	uint64_t qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) | ((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	return decode_block4x4_11bits_signed(qword, image_buffer, 0, flags);
}

// Draw a 4x4 pixel block using 128-bit ETC2 SIGNED_RG11_EAC compression data.

int draw_block4x4_signed_rg11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	uint64_t red_qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) | ((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	int r = decode_block4x4_11bits_signed(red_qword, image_buffer, 0, flags);
	if (r == 0)
		return 0;
	uint64_t green_qword = ((uint64_t)bitstring[8] << 56) | ((uint64_t)bitstring[9] << 48) | ((uint64_t)bitstring[10] << 40) |
		((uint64_t)bitstring[11] << 32) | ((uint64_t)bitstring[12] << 24) |
		((uint64_t)bitstring[13] << 16) | ((uint64_t)bitstring[14] << 8) | bitstring[15];
	decode_block4x4_11bits_signed(green_qword, image_buffer, 1, flags);
	return 1;
}

// Manual optimization function for the alpha components of ETC2 PUNCHTHROUGH mode.

void optimize_block_alpha_etc2_punchthrough(unsigned char *bitstring, unsigned char *alpha_values) {
	if (bitstring[3] & 2)
		// Opaque mode. Don't bother optimizing this bitstring.
		return;
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	if (!(R & 0xFF07) && !(G & 0xFF07) && (B & 0xFF07))
		// Planar mode. Always opaque.
		return;
	// We have differential, T or H mode with pixels that can be non-opaque.
	unsigned int pixel_index_word = ((unsigned int)bitstring[4] << 24) | ((unsigned int)bitstring[5] << 16) |
		((unsigned int)bitstring[6] << 8) | bitstring[7];
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
			| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int row_major_index = (i & 3) * 4 + ((i & 12) >> 2);
		if (alpha_values[row_major_index] < 128 && pixel_index != 2)
			// Force pixel index 2 (transparent, alpha is 0).
			pixel_index = 2;
		if (alpha_values[row_major_index] >= 128 && pixel_index == 2)
			// Force pixel index 1 (opaque). pixel index 0 or 3 would also be ok.
			pixel_index = 1;
		// Set the pixel index. First clear the two pixel index bits.
		pixel_index_word &= ~((1 << i) | (0x10000 << i));
		// Set the two bits.
		pixel_index_word |= ((pixel_index & 1) << i) | ((pixel_index & 2) << (16 + i - 1));
	}
	bitstring[4] = pixel_index_word >> 24;	// Only the lower 8 bits are assigned.
	bitstring[5] = pixel_index_word >> 16;
	bitstring[6] = pixel_index_word >> 8;
	bitstring[7] = pixel_index_word;
}

void optimize_block_alpha_etc2_eac(unsigned char *bitstring, unsigned char *alpha_values, int flags) {
	// Optimize the alpha values if they are either 0x00 or 0xFF.
	if ((flags & MODES_ALLOWED_PUNCHTHROUGH_ONLY) == 0)
		return;
	int base_codeword = 225;
	// Select modifier table entry 0: { -3, -6, -9, -15, 2, 5, 8, 14 }
	int modifier_table = 0;
	// Select multiplier 15, so that:
	// pixel value 3 = 225 + 15 * - 15 = 0
	// pixel value 4 = 225 + 2 * 15 = 255
	int multiplier = 15;
	bitstring[0] = base_codeword;
	bitstring[1] = modifier_table | (multiplier << 4);
	uint64_t pixels = 0;
	for (int i = 0; i < 16; i++) {
		int j = (i & 3) * 4 + ((i & 12) >> 2);
		uint64_t pixel_index;
		if (alpha_values[j] == 0)
			pixel_index = 0x3;
		else
			pixel_index = 0x4;
		pixels |= pixel_index << (45 - i * 3);
	}
	bitstring[2] = (uint8_t)(pixels >> 40);
	bitstring[3] = (uint8_t)(pixels >> 32);
	bitstring[4] = (uint8_t)(pixels >> 24);
	bitstring[5] = (uint8_t)(pixels >> 16);
	bitstring[6] = (uint8_t)(pixels >> 8);
	bitstring[7] = (uint8_t)pixels;
}

// set_mode functions: Try to modify the bitstring so that it conforms to a single mode if a single
// mode is defined in flags.

static void etc2_set_mode_THP(unsigned char *bitstring, int flags) {
	if ((flags & ETC2_MODE_ALLOWED_ALL) == ETC2_MODE_ALLOWED_T) {
		// bitstring[0] bits 0, 1, 3, 4 are used.
		// Bits 2, 5, 6, 7 can be modified.
		// Modify bits 2, 5, 6, 7 so that R < 0 or R > 31.
		int R_bits_5_to_7_clear = (bitstring[0] & 0x18) >> 3;
		int R_compl_bit_2_clear = complement3bit(bitstring[0] & 0x3);
		if (R_bits_5_to_7_clear + 0x1C + R_compl_bit_2_clear > 31) {
			// Set bits 5, 6, 7 and clear bit 2.
			bitstring[0] &= ~0x04;
			bitstring[0] |= 0xE0;
		}
		else {
			int R_compl_bit_2_set = complement3bit((bitstring[0] & 0x3) | 0x4);
			if (R_bits_5_to_7_clear + R_compl_bit_2_set < 0) {
				// Clear bits 5, 6, 7 and set bit 2.
				bitstring[0] &= ~0xE0;
				bitstring[0] |= 0x04;
			}
			else
				printf("set_mode: Can't modify ETC2_PUNCHTHROUGH mode to mode T.\n");
		}
//		printf("set_mode: bitstring[0] = 0x%02X\n", bitstring[0]);
	}
	else if ((flags & ETC2_MODE_ALLOWED_ALL) == ETC2_MODE_ALLOWED_H) {
		int G_bits_5_to_7_clear = (bitstring[1] & 0x18) >> 3;
		int G_compl_bit_2_clear = complement3bit(bitstring[1] & 0x3);
		if (G_bits_5_to_7_clear + 0x1C + G_compl_bit_2_clear > 31) {
			// Set bits 5, 6, 7 and clear bit 2.
			bitstring[1] &= ~0x04;
			bitstring[1] |= 0xE0;
		}
		else {
			int G_compl_bit_2_set = complement3bit((bitstring[1] & 0x3) | 0x4);
			if (G_bits_5_to_7_clear + G_compl_bit_2_set < 0) {
				// Clear bits 5, 6, 7 and set bit 2.
				bitstring[1] &= ~0xE0;
				bitstring[1] |= 0x04;
			}
			else
				printf("set_mode: Can't modify ETC2_PUNCHTHROUGH mode to mode H.\n");
		}
	}
	else if ((flags & ETC2_MODE_ALLOWED_ALL) == ETC2_MODE_ALLOWED_PLANAR) {
		int B_bits_5_to_7_clear = (bitstring[2] & 0x18) >> 3;
		int B_compl_bit_2_clear = complement3bit(bitstring[2] & 0x3);
		if (B_bits_5_to_7_clear + 0x1C + B_compl_bit_2_clear > 31) {
			// Set bits 5, 6, 7 and clear bit 2.
			bitstring[2] &= ~0x04;
			bitstring[2] |= 0xE0;
		}
		else {
			int B_compl_bit_2_set = complement3bit((bitstring[2] & 0x3) | 0x4);
			if (B_bits_5_to_7_clear + B_compl_bit_2_set < 0) {
				// Clear bits 5, 6, 7 and set bit 2.
				bitstring[2] &= ~0xE0;
				bitstring[2] |= 0x04;
			}
			else
				printf("set_mode: Can't modify ETC2_PUNCHTHROUGH mode to mode P.\n");
		}
//		printf("set_mode: bitstring[2] = 0x%02X\n", bitstring[2]);
	}
}

void block4x4_etc1_set_mode(unsigned char *bitstring, int flags) {
	if ((flags & ETC2_MODE_ALLOWED_ALL) == ETC_MODE_ALLOWED_INDIVIDUAL)
		bitstring[3] &= ~0x2;
	else
		bitstring[3] |= 0x2;
}

void block4x4_etc2_rgb8_set_mode(unsigned char *bitstring, int flags) {
	if ((flags & ETC2_MODE_ALLOWED_ALL) == ETC_MODE_ALLOWED_INDIVIDUAL)
		bitstring[3] &= ~0x2;
	else {
		bitstring[3] |= 0x2;
		etc2_set_mode_THP(bitstring, flags);
	}
}

void block4x4_etc2_eac_set_mode(unsigned char *bitstring, int flags) {
	block4x4_etc2_rgb8_set_mode(&bitstring[8], flags);
}

void block4x4_etc2_punchthrough_set_mode(unsigned char *bitstring, int flags) {
	if (flags & MODES_ALLOWED_NON_OPAQUE_ONLY)
		bitstring[3] &= ~0x2;
	if (flags & MODES_ALLOWED_OPAQUE_ONLY)
		bitstring[3] |= 0x2;
	etc2_set_mode_THP(bitstring, flags);
}


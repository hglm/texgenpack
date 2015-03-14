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

#include <stdint.h>

// Define some short functions for pixel packing/unpacking. The compiler will take care of optimization by
// inlining and removing unused functions.

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)

static unsigned int pack_rgba(int r, int g, int b, int a) {
	return (unsigned int)r | ((unsigned int)g << 8) | ((unsigned int)b << 16) |
		((unsigned int)a << 24);
}

static unsigned int pack_rgb_alpha_0xff(int r, int g, int b) {
	return pack_rgba(r, g, b, 0xFF);
}

static unsigned int pack_r(int r) {
	return (unsigned int)r;
}

static unsigned int pack_g(int g) {
	return (unsigned int)g << 8;
}

static unsigned int pack_b(int b) {
	return (unsigned int)b << 16;
}

static unsigned int pack_a(int a) {
	return (unsigned int)a << 24;
}

static unsigned int pack_half_float(int r16, int g16) {
	return (unsigned int)r16 | ((unsigned int)g16 << 16);
}

static uint64_t pack_r16(unsigned int r16) {
	return r16;
}

static uint64_t pack_g16(unsigned int g16) {
	return g16 << 16;
}

static uint64_t pack_b16(unsigned int b16) {
	return (uint64_t)b16 << 32;
}

static uint64_t pack_a16(unsigned int a16) {
	return (uint64_t)a16 << 48;
}

static uint64_t pack_rgb16(uint16_t r16, uint16_t g16, uint16_t b16) {
	return (uint64_t)r16 | ((uint64_t)g16 << 16) | ((uint64_t)b16 << 32);
}

static uint64_t pack_rgba16(uint16_t r16, uint16_t g16, uint16_t b16, uint16_t a16) {
	return (uint64_t)r16 | ((uint64_t)g16 << 16) | ((uint64_t)b16 << 32) | ((uint64_t)a16 << 48);
}

static int pixel_get_r(unsigned int pixel) {
	return pixel & 0xFF;
}

static int pixel_get_g(unsigned int pixel) {
	return (pixel & 0xFF00) >> 8;
}

static int pixel_get_b(unsigned int pixel) {
	return (pixel & 0xFF0000) >> 16;
}

static int pixel_get_a(unsigned int pixel) {
	return (pixel & 0xFF000000) >> 24;
}

static int pixel_get_signed_r8(unsigned int pixel) {
	return (char)(pixel & 0xFF);
}

static int pixel_get_signed_g8(unsigned int pixel) {
	return (char)((pixel & 0xFF00) >> 8);
}

static unsigned int pixel_get_r16(unsigned int pixel) {
	return pixel & 0x0000FFFF;
}

static unsigned int pixel_get_g16(unsigned int pixel) {
	return (pixel & 0xFFFF0000) >> 16;
}

static int pixel_get_signed_r16(unsigned int pixel) {
	return (short)(pixel & 0xFFFF);
}

static int pixel_get_signed_g16(unsigned int pixel) {
	return (short)((pixel & 0xFFFF0000) >> 16);
}

static uint64_t pixel64_get_r16(uint64_t pixel) {
	return pixel & 0xFFFF;
}

static uint64_t pixel64_get_g16(uint64_t pixel) {
	return (pixel & 0xFFFF0000) >> 16;
}

static uint64_t pixel64_get_b16(uint64_t pixel) {
	return (pixel & 0xFFFF00000000) >> 32;
}

static uint64_t pixel64_get_a16(uint64_t pixel) {
	return (pixel & 0xFFFF000000000000) >> 48;
}


#define alpha_byte_offset 3

#else

static unsigned int pack_rgba(int r, int g, int b, int a) {
	return a | ((unsigned int)b << 8) | ((unsigned int)g << 16) | ((unsigned int )r << 24);
}

static unsigned int pack_rgb_alpha_0xff(int r, int g, int b) {
	return pack_rgba(r, g, b, 0xFF);
}

static unsigned int pack_r(int r) {
	return (unsigned int)r << 24;
}

static unsigned int pack_g(int g) {
	return (unsigned int)g << 16;
}

static unsigned int pack_b(int b) {
	return (unsigned int)b << 8;
}

static unsigned int pack_a(int a) {
	return a;
}

static unsigned int pack_half_float(int r16, int g16) {
	return (unsigned int)g16 | ((unsigned int)r16 << 16);
}

static int pixel_get_r(unsigned int pixel) {
	return (pixel & 0xFF000000) >> 24;
}

static int pixel_get_g(unsigned int pixel) {
	return (pixel & 0xFF0000) >> 16;
}

static int pixel_get_b(unsigned int pixel) {
	return (pixel & 0xFF00) >> 8;
}

static int pixel_get_a(unsigned int pixel) {
	return pixel & 0xFF;
}

static unsigned int pixel_get_r16(unsigned int pixel) {
	return ((pixel & 0xFF000000) >> 24) | ((pixel & 0x00FF0000) >> 8);
}

static unsigned int pixel_get_g16(unsigned int pixel) {
	return ((pixel & 0x0000FF00) >> 8) | ((pixel & 0x000000FF) << 8);
}

static int pixel_get_signed_r16(unsigned int pixel) {
	return (short)(((pixel & 0xFF000000) >> 24) | ((pixel & 0x00FF0000) >> 8));
}

static int pixel_get_signed_g16(unsigned int pixel) {
	return (short)(((pixel & 0x0000FF00) >> 8) | ((pixel & 0x000000FF) << 8));
}

#define alpha_byte_offset 0

#endif

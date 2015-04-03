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

// A note about texgenpack's implementation of RGTC1 and RGTC2:
// Unsigned RGTC's internal format is 8 bits per component [0, 255].
// Signed RGTC's internal format is 16 bits per component [-32768, 32768]. This is because signed RGTC
// maps [-127, 127] to [0, 1], leaving -128 unused.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

static int draw_block4x4_rgtc1_shared(const unsigned char *bitstring, unsigned int *image_buffer, int offset, int flags) {
	// LSBFirst byte order only.
	uint64_t bits = (*(uint64_t *)&bitstring[0]) >> 16;
	int lum0 = bitstring[0];
	int lum1 = bitstring[1];
	for (int i = 0; i < 16; i++) {
		int control_code = bits & 0x7;
		uint8_t output;
		if (lum0 > lum1)
			switch (control_code) {
			case 0 : output = lum0; break;
			case 1 : output = lum1; break;
			case 2 : output = (6 * lum0 + lum1) / 7; break;
			case 3 : output = (5 * lum0 + 2 * lum1) / 7; break;
			case 4 : output = (4 * lum0 + 3 * lum1) / 7; break;
			case 5 : output = (3 * lum0 + 4 * lum1) / 7; break;
			case 6 : output = (2 * lum0 + 5 * lum1) / 7; break;
			case 7 : output = (lum0 + 6 * lum1) / 7; break;
			}
		else
			switch (control_code) {
			case 0 : output = lum0; break;
			case 1 : output = lum1; break;
			case 2 : output = (4 * lum0 + lum1) / 5; break;
			case 3 : output = (3 * lum0 + 2 * lum1) / 5; break;
			case 4 : output = (2 * lum0 + 3 * lum1) / 5; break;
			case 5 : output = (lum0 + 4 * lum1) / 5; break;
			case 6 : output = 0; break;
			case 7 : output = 0xFF; break;
			}
		*((uint8_t *)&image_buffer[i] + offset) = output;
		bits >>= 3;
	}
	return 1;
}

int draw_block4x4_rgtc1(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	memset(image_buffer, 0, 64);
	return draw_block4x4_rgtc1_shared(bitstring, image_buffer, 0, flags);
}

int draw_block4x4_signed_rgtc1_shared(const unsigned char *bitstring, unsigned int *image_buffer,
int offset, int flags) {
	// LSBFirst byte order only.
	uint64_t bits = (*(uint64_t *)&bitstring[0]) >> 16;
	int lum0 = (char)bitstring[0];
	int lum1 = (char)bitstring[1];
	if (lum0 == - 127 && lum1 == - 128)
		// Not allowed.
		return 0;
	if (lum0 == - 128)
		lum0 = - 127;
	if (lum1 == - 128)
		lum1 = - 127;
	// Note: values are mapped to a red value of -127 to 127.
	for (int i = 0; i < 16; i++) {
		int control_code = bits & 0x7;
		int32_t result;
		if (lum0 > lum1)
			switch (control_code) {
			case 0 : result = lum0; break;
			case 1 : result = lum1; break;
			case 2 : result = (6 * lum0 + lum1) / 7; break;
			case 3 : result = (5 * lum0 + 2 * lum1) / 7; break;
			case 4 : result = (4 * lum0 + 3 * lum1) / 7; break;
			case 5 : result = (3 * lum0 + 4 * lum1) / 7; break;
			case 6 : result = (2 * lum0 + 5 * lum1) / 7; break;
			case 7 : result = (lum0 + 6 * lum1) / 7; break;
			}
		else
			switch (control_code) {
			case 0 : result = lum0; break;
			case 1 : result = lum1; break;
			case 2 : result = (4 * lum0 + lum1) / 5; break;
			case 3 : result = (3 * lum0 + 2 * lum1) / 5; break;
			case 4 : result = (2 * lum0 + 3 * lum1) / 5; break;
			case 5 : result = (lum0 + 4 * lum1) / 5; break;
			case 6 : result = - 127; break;
			case 7 : result = 127; break;
			}
		// Map from [-127, 127] to [-32768, 32767].
		*((int16_t *)&image_buffer[i] + offset) = (int16_t)((result + 127) * 65535 / 254 - 32768);
		bits >>= 3;
	}
	return 1;
}

int draw_block4x4_signed_rgtc1(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	memset(image_buffer, 0, 64);
	return draw_block4x4_signed_rgtc1_shared(bitstring, image_buffer, 0, flags);
}

// Note: the second component in stored in the green byte, not the alpha byte.

int draw_block4x4_rgtc2(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	memset(image_buffer, 0, 64);
	int r = draw_block4x4_rgtc1_shared(bitstring, image_buffer, 0, flags);
	if (r == 0)
		return 0;
	return draw_block4x4_rgtc1_shared(&bitstring[8], image_buffer, 1, flags);
}

int draw_block4x4_signed_rgtc2(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	memset(image_buffer, 0, 64);
	int r = draw_block4x4_signed_rgtc1_shared(bitstring, image_buffer, 0, flags);
	if (r == 0)
		return 0;
	return draw_block4x4_signed_rgtc1_shared(&bitstring[8], image_buffer, 1, flags);
}


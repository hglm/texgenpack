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
#include <stdint.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

// Functions for BPTC/BC7/BC6H decompression.

static char table_P2[64 * 16] = {
    0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
    0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,
    0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,
    0,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1,
    0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1,
    0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,
    0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,
    0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,
    0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
    0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
    0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,1,
    0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,
    0,1,1,1,0,0,1,1,0,0,0,1,0,0,0,0,
    0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,
    0,0,0,0,1,0,0,0,1,1,0,0,1,1,1,0,
    0,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,
    0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,
    0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0,
    0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,
    0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,
    0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,
    0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0,
    0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
    0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0,
    0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0,
    0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
    0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
    0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,
    0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0,
    0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,
    0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,
    0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
    0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,
    0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0,
    0,0,0,1,0,0,1,1,1,1,0,0,1,0,0,0,
    0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,
    0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,0,1,1,1,1,0,0,1,1,0,0,0,0,1,1,
    0,1,1,0,0,1,1,0,1,0,0,1,1,0,0,1,
    0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,
    0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,0,
    0,0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,
    0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,
    0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0,
    0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1,
    0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,1,
    0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,0,
    0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0,
    0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,1,
    0,1,1,0,0,0,1,1,0,0,1,1,1,0,0,1,
    0,1,1,1,1,1,1,0,1,0,0,0,0,0,0,1,
    0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1,
    0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1,
    0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,
    0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,0,
    0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1
};

char table_P3[64 * 16] = {
    0,0,1,1,0,0,1,1,0,2,2,1,2,2,2,2,
    0,0,0,1,0,0,1,1,2,2,1,1,2,2,2,1,
    0,0,0,0,2,0,0,1,2,2,1,1,2,2,1,1,
    0,2,2,2,0,0,2,2,0,0,1,1,0,1,1,1,
    0,0,0,0,0,0,0,0,1,1,2,2,1,1,2,2,
    0,0,1,1,0,0,1,1,0,0,2,2,0,0,2,2,
    0,0,2,2,0,0,2,2,1,1,1,1,1,1,1,1,
    0,0,1,1,0,0,1,1,2,2,1,1,2,2,1,1,
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,
    0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,
    0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,
    0,1,1,2,0,1,1,2,0,1,1,2,0,1,1,2,
    0,1,2,2,0,1,2,2,0,1,2,2,0,1,2,2,
    0,0,1,1,0,1,1,2,1,1,2,2,1,2,2,2,
    0,0,1,1,2,0,0,1,2,2,0,0,2,2,2,0,
    0,0,0,1,0,0,1,1,0,1,1,2,1,1,2,2,
    0,1,1,1,0,0,1,1,2,0,0,1,2,2,0,0,
    0,0,0,0,1,1,2,2,1,1,2,2,1,1,2,2,
    0,0,2,2,0,0,2,2,0,0,2,2,1,1,1,1,
    0,1,1,1,0,1,1,1,0,2,2,2,0,2,2,2,
    0,0,0,1,0,0,0,1,2,2,2,1,2,2,2,1,
    0,0,0,0,0,0,1,1,0,1,2,2,0,1,2,2,
    0,0,0,0,1,1,0,0,2,2,1,0,2,2,1,0,
    0,1,2,2,0,1,2,2,0,0,1,1,0,0,0,0,
    0,0,1,2,0,0,1,2,1,1,2,2,2,2,2,2,
    0,1,1,0,1,2,2,1,1,2,2,1,0,1,1,0,
    0,0,0,0,0,1,1,0,1,2,2,1,1,2,2,1,
    0,0,2,2,1,1,0,2,1,1,0,2,0,0,2,2,
    0,1,1,0,0,1,1,0,2,0,0,2,2,2,2,2,
    0,0,1,1,0,1,2,2,0,1,2,2,0,0,1,1,
    0,0,0,0,2,0,0,0,2,2,1,1,2,2,2,1,
    0,0,0,0,0,0,0,2,1,1,2,2,1,2,2,2,
    0,2,2,2,0,0,2,2,0,0,1,2,0,0,1,1,
    0,0,1,1,0,0,1,2,0,0,2,2,0,2,2,2,
    0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,
    0,0,0,0,1,1,1,1,2,2,2,2,0,0,0,0,
    0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0,
    0,1,2,0,2,0,1,2,1,2,0,1,0,1,2,0,
    0,0,1,1,2,2,0,0,1,1,2,2,0,0,1,1,
    0,0,1,1,1,1,2,2,2,2,0,0,0,0,1,1,
    0,1,0,1,0,1,0,1,2,2,2,2,2,2,2,2,
    0,0,0,0,0,0,0,0,2,1,2,1,2,1,2,1,
    0,0,2,2,1,1,2,2,0,0,2,2,1,1,2,2,
    0,0,2,2,0,0,1,1,0,0,2,2,0,0,1,1,
    0,2,2,0,1,2,2,1,0,2,2,0,1,2,2,1,
    0,1,0,1,2,2,2,2,2,2,2,2,0,1,0,1,
    0,0,0,0,2,1,2,1,2,1,2,1,2,1,2,1,
    0,1,0,1,0,1,0,1,0,1,0,1,2,2,2,2,
    0,2,2,2,0,1,1,1,0,2,2,2,0,1,1,1,
    0,0,0,2,1,1,1,2,0,0,0,2,1,1,1,2,
    0,0,0,0,2,1,1,2,2,1,1,2,2,1,1,2,
    0,2,2,2,0,1,1,1,0,1,1,1,0,2,2,2,
    0,0,0,2,1,1,1,2,1,1,1,2,0,0,0,2,
    0,1,1,0,0,1,1,0,0,1,1,0,2,2,2,2,
    0,0,0,0,0,0,0,0,2,1,1,2,2,1,1,2,
    0,1,1,0,0,1,1,0,2,2,2,2,2,2,2,2,
    0,0,2,2,0,0,1,1,0,0,1,1,0,0,2,2,
    0,0,2,2,1,1,2,2,1,1,2,2,0,0,2,2,
    0,0,0,0,0,0,0,0,0,0,0,0,2,1,1,2,
    0,0,0,2,0,0,0,1,0,0,0,2,0,0,0,1,
    0,2,2,2,1,2,2,2,0,2,2,2,1,2,2,2,
    0,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
    0,1,1,1,2,0,1,1,2,2,0,1,2,2,2,0,
};

typedef struct {
	uint64_t data0;
	uint64_t data1;
	int index;
} Block;

static uint32_t block_extract_bits(Block *block, int nu_bits) {
	uint32_t value = 0;
	for (int i = 0; i < nu_bits; i++) {
		if (block->index < 64) {
			int shift = block->index - i;
			if (shift < 0)
				value |= (block->data0 & ((uint64_t)1 << block->index)) << (- shift);
			else
				value |= (block->data0 & ((uint64_t)1 << block->index)) >> shift;
		}
		else {
			int shift = ((block->index - 64) - i);
			if (shift < 0)
				value |= (block->data1 & ((uint64_t)1 << (block->index - 64))) << (- shift);
			else
				value |= (block->data1 & ((uint64_t)1 << (block->index - 64))) >> shift;
		}
		block->index++;
	}
//	if (block->index > 128)
//		printf("Block overflow (%d)\n", block->index);
	return value;
}

static uint32_t get_bits_uint64(uint64_t data, int bit0, int bit1) {
	return (data & (((uint64_t)1 << (bit1 + 1)) - 1)) >> bit0;
}

static char color_precision_table[8] = { 4, 6, 5, 7, 5, 7, 7, 5 };

// Note: precision includes P-bits!
static char color_precision_plus_pbit_table[8] = { 5, 7, 5, 8, 5, 7, 8, 6 };

static char color_component_precision(int mode) {
	return color_precision_table[mode];
}

static char color_component_precision_plus_pbit(int mode) {
	return color_precision_plus_pbit_table[mode];
}

static char alpha_precision_table[8] = { 0, 0, 0, 0, 6, 8, 7, 5 };

// Note: precision include P-bits!
static char alpha_precision_plus_pbit_table[8] = { 0, 0, 0, 0, 6, 8, 8, 6 };

static char alpha_component_precision(int mode) {
	return alpha_precision_table[mode];
}

static char alpha_component_precision_plus_pbit(int mode) {
	return alpha_precision_plus_pbit_table[mode];
}

// #subsets = { 3, 2, 3, 2, 1, 1, 1, 2 };
// partition bits = { 4, 6, 6, 6, 0, 0, 0, 6 };
// rotation bits = { 0, 0, 0, 0, 2, 2, 0, 0 };
// mode 4 has one index selection bit.
//
//	    #subsets  color   alpha part. index before color	index after color	index after alpha
// Mode 0	3	4	0		1 + 4 = 5	5 + 6 * 3 * 4 = 77	77
// Mode 1	Handled elsewhere.
// Mode 2	3	5	0		3 + 6 = 9	9 + 6 * 3 * 5 = 99	99
// Mode 3	2	7	0		4 + 6 = 10	10 + 4 * 3 * 7 = 94	94
// Mode 4	1	5	6		5 + 2 + 1 = 8	8 + 2 * 3 * 5 = 38	37 + 2 * 6 = 50
// Mode 5	1	7	8		6 + 2 = 8	8 + 2 * 3 * 7 = 50	50 + 2 * 8 = 66
// Mode	6	1	7	7		7		7 + 2 * 3 * 7 = 49	49 + 2 * 7 = 63
// Mode 7	2	5	5		8 + 6 = 14	14 + 4 * 3 * 5 = 74	74 + 4 * 5 = 94

static char components_in_qword0_table[8] = { 2, -1, 1, 1, 3, 3, 3, 2 };

static void extract_endpoints(int mode, int nu_subsets, Block *block, uint8_t *endpoint_array) {
#if 1
	// Optimized version avoiding the use of block_extract_bits()
	int components_in_qword0 = components_in_qword0_table[mode];
	uint64_t data = block->data0 >> block->index;
	uint8_t precision = color_component_precision(mode);
	uint8_t mask = (1 << precision) - 1;
	int total_bits_per_component = nu_subsets * 2 * precision;
	for (int i = 0; i < components_in_qword0; i++)	// For each color component.
		for (int j = 0; j < nu_subsets; j++)	// For each subset.
			for (int k = 0; k < 2; k++) {	// For each endpoint.
				endpoint_array[j * 8 + k * 4 + i] = data & mask;
				data >>= precision;
			}
	block->index += components_in_qword0 * total_bits_per_component;
	if (components_in_qword0 < 3) {
		// Handle the color component that crosses the boundary between data0 and data1
		data = block->data0 >> block->index;
		data |= block->data1 << (64 - block->index);
		int i = components_in_qword0;
		for (int j = 0; j < nu_subsets; j++)	// For each subset.
			for (int k = 0; k < 2; k++) {	// For each endpoint.
				endpoint_array[j * 8 + k * 4 + i] = data & mask;
				data >>= precision;
			}
		block->index += total_bits_per_component;
	}
	if (components_in_qword0 < 2) {
		// Handle the color component that is wholly in data1.
		data = block->data1 >> (block->index - 64);
		int i = 2;
		for (int j = 0; j < nu_subsets; j++)	// For each subset.
			for (int k = 0; k < 2; k++) {	// For each endpoint.
				endpoint_array[j * 8 + k * 4 + i] = data & mask;
				data >>= precision;
			}
		block->index += total_bits_per_component;
	}
	// Alpha component.
	if (alpha_component_precision(mode) > 0) {
		// For mode 7, the alpha data is wholly in data1.
		// For modes 4 and 6, the alpha data is wholly in data0.
		// For mode 5, the alpha data is in data0 and data1.
		if (mode == 7)
			data = block->data1 >> (block->index - 64);
		else if (mode == 5)
			data = (block->data0 >> block->index) | ((block->data1 & 0x3) << 14);
		else
			data = block->data0 >> block->index;
		uint8_t alpha_precision = alpha_component_precision(mode);
		uint8_t mask = (1 << alpha_precision) - 1;
		for (int j = 0; j < nu_subsets; j++)
			for (int k = 0; k < 2; k++) {	// For each endpoint.
				endpoint_array[j * 8 + k * 4 + 3] = data & mask;
				data >>= alpha_precision;
			}
		block->index += nu_subsets * 2 * alpha_precision;
	}
#else
	// Color components.
	for (int i = 0; i < 3; i++)	// For each color component.
		for (int j = 0; j < nu_subsets; j++)	// For each subset.
			for (int k = 0; k < 2; k++)	// For each endpoint.
				endpoint_array[j * 8 + k * 4 + i] =
					block_extract_bits(block, color_component_precision(mode));
	// Alpha component.
	if (alpha_component_precision(mode) > 0) {
		for (int j = 0; j < nu_subsets; j++)
			for (int k = 0; k < 2; k++)	// For each endpoint.
				endpoint_array[j * 8 + k * 4 + 3] =
					block_extract_bits(block, alpha_component_precision(mode));
	}
#endif
}

static char mode_has_p_bits[8] = { 1, 1, 0, 1, 0, 0, 1, 1 };

void fully_decode_endpoints(uint8_t *endpoint_array, int nu_subsets, int mode, Block *block) {
	if (mode_has_p_bits[mode]) {
		// Mode 1 handled elsewhere.

		// Extract end-point pbits.
		// Take advantage of the fact that they don't cross the 64-bit word boundary
		// in any mode.
		uint32_t bits;
		if (block->index < 64)
			bits = block->data0 >> block->index;
		else
			bits = block->data1 >> (block->index - 64);
		for (int i = 0; i < nu_subsets * 2; i++) {
			endpoint_array[i * 4 + 0] <<= 1;
 			endpoint_array[i * 4 + 1] <<= 1;
			endpoint_array[i * 4 + 2] <<= 1;
			endpoint_array[i * 4 + 3] <<= 1;
			endpoint_array[i * 4 + 0] |= (bits & 1);
			endpoint_array[i * 4 + 1] |= (bits & 1);
			endpoint_array[i * 4 + 2] |= (bits & 1);
			endpoint_array[i * 4 + 3] |= (bits & 1);
			bits >>= 1;
		}
		block->index += nu_subsets * 2;
	}

	int color_prec = color_component_precision_plus_pbit(mode);
	int alpha_prec = alpha_component_precision_plus_pbit(mode);
	for (int i = 0; i < nu_subsets * 2; i++) {
		// Color_component_precision & alpha_component_precision includes pbit
		// left shift endpoint components so that their MSB lies in bit 7
		endpoint_array[i * 4 + 0] <<= (8 - color_prec);
		endpoint_array[i * 4 + 1] <<= (8 - color_prec);
		endpoint_array[i * 4 + 2] <<= (8 - color_prec);
		endpoint_array[i * 4 + 3] <<= (8 - alpha_prec);

	        // Replicate each component's MSB into the LSBs revealed by the left-shift operation above.
		endpoint_array[i * 4 + 0] |= (endpoint_array[i * 4 + 0] >> color_prec);
		endpoint_array[i * 4 + 1] |= (endpoint_array[i * 4 + 1] >> color_prec);
		endpoint_array[i * 4 + 2] |= (endpoint_array[i * 4 + 2] >> color_prec);
		endpoint_array[i * 4 + 3] |= (endpoint_array[i * 4 + 3] >> alpha_prec);
	}
        
	if (mode <= 3) {
		for (int i = 0; i < nu_subsets * 2; i++)
			endpoint_array[i * 4 + 3] = 0xFF;
 	}
}

uint16_t aWeight2[4] = { 0, 21, 43, 64 };
uint16_t aWeight3[8] = { 0, 9, 18, 27, 37, 46, 55, 64 };
uint16_t aWeight4[16] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };

static uint8_t interpolate(uint8_t e0, uint8_t e1, uint8_t index, uint8_t indexprecision) {
	if (indexprecision == 2)
		return (uint8_t) (((64 - aWeight2[index]) * (uint16_t)e0 + aWeight2[index] * (uint16_t)e1 + 32) >> 6);
	else
	if (indexprecision == 3)
		return (uint8_t) (((64 - aWeight3[index]) * (uint16_t)e0 + aWeight3[index] * (uint16_t)e1 + 32) >> 6);
	else // indexprecision == 4
		return (uint8_t) (((64 - aWeight4[index]) * (uint16_t)e0 + aWeight4[index] * (uint16_t)e1 + 32) >> 6);
}

static char bptc_color_index_bitcount[8] = { 3, 3, 2, 2, 2, 2, 4, 2 };

static int get_color_index_bitcount(int mode, int index_selection_bit) {
	// If the index selection bit is set for mode 4, return 3, otherwise 2.
	return bptc_color_index_bitcount[mode] + index_selection_bit;
}

static char bptc_alpha_index_bitcount[8] = { 3, 3, 2, 2, 3, 2, 4, 2};

static int get_alpha_index_bitcount(int mode, int index_selection_bit) {
	// If the index selection bit is set for mode 4, return 2, otherwise 3.
	return bptc_alpha_index_bitcount[mode] - index_selection_bit;
}

static int extract_mode(Block *block) {
	for (int i = 0; i < 8; i++)
		if (block->data0 & ((uint64_t)1 << i)) {
			block->index = i + 1;
			return i;
		}
	// Illegal.
	return - 1;
}

static char bptc_NS[8] = { 3, 2, 3, 2, 1, 1, 1, 2 };

static int get_nu_subsets(int mode) {
	return bptc_NS[mode];
}

static char PB[8] = { 4, 6, 6, 6, 0, 0, 0, 6 };

static int extract_partition_set_id(Block *block, int mode) {
	return block_extract_bits(block, PB[mode]);
}


static int get_partition_index(int nu_subsets, int partition_set_id, int i) {
	if (nu_subsets == 1)
		return 0;
	if (nu_subsets == 2)
		return table_P2[partition_set_id * 16 + i];
	return table_P3[partition_set_id * 16 + i];
}

static char RB[8] = { 0, 0, 0, 0, 2, 2, 0, 0 };

static int extract_rot_bits(Block *block, int mode) {
	return block_extract_bits(block, RB[mode]);
}

static char anchor_index_second_subset[64] = {
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15, 2, 8, 2, 2, 8, 8,15,
     2, 8, 2, 2, 8, 8, 2, 2,
    15,15, 6, 8, 2, 8,15,15,
     2, 8, 2, 2, 2,15,15, 6,
     6, 2, 6, 8,15,15, 2, 2,
    15,15,15,15,15, 2, 2,15
};

static char anchor_index_second_subset_of_three[64] = {
     3, 3,15,15, 8, 3,15,15,
     8, 8, 6, 6, 6, 5, 3, 3,
     3, 3, 8,15, 3, 3, 6,10,
     5, 8, 8, 6, 8, 5,15,15,
     8,15, 3, 5, 6,10, 8,15,
    15, 3,15, 5,15,15,15,15,
     3,15, 5, 5, 5, 8, 5,10,
     5,10, 8,13,15,12, 3, 3
};

static char anchor_index_third_subset[64] = {
    15, 8, 8, 3,15,15, 3, 8,
    15,15,15,15,15,15,15, 8,
    15, 8,15, 3,15, 8,15, 8,
     3,15, 6,10,15,15,10, 8,
    15, 3,15,10,10, 8, 9,10,
     6,15, 8,15, 3, 6, 6, 8,
    15, 3,15,15,15,15,15,15,
    15,15,15,15, 3,15,15, 8
};

static int get_anchor_index(int partition_set_id, int partition, int nu_subsets) {
	if (partition == 0)
		return 0;
	if (nu_subsets == 2)
		return anchor_index_second_subset[partition_set_id];
	if (partition == 1)
		return anchor_index_second_subset_of_three[partition_set_id];
	return anchor_index_third_subset[partition_set_id];
}

static char IB[8] = { 3, 3, 2, 2, 2, 2, 4, 2 };
static char IB2[8] = { 0, 0, 0, 0, 3, 2, 0, 0 };

static char mode_has_partition_bits[8] = { 1, 1, 1, 1, 0, 0, 0, 1 };

static int draw_bptc_mode_1(Block *block, unsigned int *image_buffer);

int block4x4_bptc_get_mode(const unsigned char *bitstring) {
	Block block;
	block.data0 = *(uint64_t *)&bitstring[0];
	block.data1 = *(uint64_t *)&bitstring[8];
	block.index = 0;
	int mode = extract_mode(&block);
	return mode;
}

void block4x4_bptc_set_mode(unsigned char *bitstring, int flags) {
	// Mode 0 starts with 1
	// Mode 1 starts with 01
	// ...
	// Mode 7 starts with 00000001
	int mode_flags = flags & BPTC_MODE_ALLOWED_ALL;
	int bit = 0x1;
	for (int i = 0; i < 8; i++) {
		if (mode_flags == (1 << i)) {
			bitstring[0] &= ~(bit - 1);
			bitstring[0] |= bit;
			return;
		}
		bit <<= 1;
	}
}

static uint64_t clear_bits_uint64(uint64_t data, int bit0, int bit1) {
	uint64_t mask = ~(((uint64_t)1 << (bit1 + 1)) - 1);
	mask |= ((uint64_t)1 << bit0) - 1;
	return data & mask;
}

static uint64_t set_bits_uint64(uint64_t data, int bit0, int bit1, uint64_t val) {
	uint64_t d = clear_bits_uint64(data, bit0, bit1);
	d |= val << bit0;
	return d;
}

void bptc_set_block_colors(unsigned char *bitstring, int flags, unsigned int *colors) {
	if ((flags & TWO_COLORS) == 0)
		return;
	uint64_t data0 = *(uint64_t *)&bitstring[0];
	uint64_t data1 = *(uint64_t *)&bitstring[8];
	if ((flags & BPTC_MODE_ALLOWED_ALL) == (1 << 3)) {
		// Mode 3, 7 color bits.
		// Color bits at index: 10
		// Color bits end before index: 10 + 4 * 3 * 7 = 94
		int r0 = pixel_get_r(colors[0]);
		int g0 = pixel_get_g(colors[0]);
		int b0 = pixel_get_b(colors[0]);
		int r1 = pixel_get_r(colors[1]);
		int g1 = pixel_get_g(colors[1]);
		int b1 = pixel_get_b(colors[1]);
		data0 = set_bits_uint64(data0, 10, 16, r0 >> 1);
		data0 = set_bits_uint64(data0, 17, 23, r0 >> 1);
		data0 = set_bits_uint64(data0, 24, 30, r1 >> 1);
		data0 = set_bits_uint64(data0, 31, 37, r1 >> 1);
		data0 = set_bits_uint64(data0, 38, 44, g0 >> 1);
		data0 = set_bits_uint64(data0, 45, 51, g0 >> 1);
		data0 = set_bits_uint64(data0, 52, 58, g1 >> 1);
		data0 = set_bits_uint64(data0, 59, 63, (g1 >> 1) & 0x1F);
		data1 = set_bits_uint64(data1, 0, 1, ((g1 >> 1) & 0x60) >> 5);
		data1 = set_bits_uint64(data1, 2, 8, b0 >> 1);
		data1 = set_bits_uint64(data1, 9, 15, b0 >> 1);
		data1 = set_bits_uint64(data1, 16, 22, b1 >> 1);
		data1 = set_bits_uint64(data1, 23, 29, b1 >> 1);
		*(uint64_t *)&bitstring[0] = data0;
		*(uint64_t *)&bitstring[8] = data1;
//		printf("bptc_set_block_colors: Colors set for mode 3.\n");
	}
	else if ((flags & BPTC_MODE_ALLOWED_ALL) == (1 << 5)) {
		// Mode 5, 7 color bits, 8 alpha bits.
		// Color bits at index: 6 + 2 = 8
		// Alpha bits at index: 8 + 2 * 3 * 7 = 50
		// Alpha bits end before index: 50 + 2 * 8 = 66
		int r0 = pixel_get_r(colors[0]);
		int g0 = pixel_get_g(colors[0]);
		int b0 = pixel_get_b(colors[0]);
		int r1 = pixel_get_r(colors[1]);
		int g1 = pixel_get_g(colors[1]);
		int b1 = pixel_get_b(colors[1]);
		data0 = set_bits_uint64(data0, 8, 14, r0 >> 1);
		data0 = set_bits_uint64(data0, 15, 21, r1 >> 1);
		data0 = set_bits_uint64(data0, 22, 28, g0 >> 1);
		data0 = set_bits_uint64(data0, 29, 35, g0 >> 1);
		data0 = set_bits_uint64(data0, 36, 42, b0 >> 1);
		data0 = set_bits_uint64(data0, 43, 49, b1 >> 1);
		if (flags & (MODES_ALLOWED_PUNCHTHROUGH_ONLY)) {
			data0 = set_bits_uint64(data0, 50, 57, 0x00);
			data0 = set_bits_uint64(data0, 58, 63, 0x3F);
			data1 = set_bits_uint64(data1, 0, 1, 0x3);
		}
		*(uint64_t *)&bitstring[0] = data0;
		*(uint64_t *)&bitstring[8] = data1;
//		printf("bptc_set_block_colors: Colors set for mode 5.\n");
	}
	else if ((flags & BPTC_MODE_ALLOWED_ALL) == (1 << 6)) {
		// Mode 5, 7 color bits, 7 alpha bits.
		// Color bits at index 7.
		// Alpha bits at index: 7 + 2 * 3 * 7 = 49
		// Alpha bits end before index: 49 + 2 * 7 = 63
		int r0 = pixel_get_r(colors[0]);
		int g0 = pixel_get_g(colors[0]);
		int b0 = pixel_get_b(colors[0]);
		int r1 = pixel_get_r(colors[1]);
		int g1 = pixel_get_g(colors[1]);
		int b1 = pixel_get_b(colors[1]);
		data0 = set_bits_uint64(data0, 7, 13, r0 >> 1);
		data0 = set_bits_uint64(data0, 14, 20, r1 >> 1);
		data0 = set_bits_uint64(data0, 21, 27, g0 >> 1);
		data0 = set_bits_uint64(data0, 28, 34, g1 >> 1);
		data0 = set_bits_uint64(data0, 35, 41, b0 >> 1);
		data0 = set_bits_uint64(data0, 42, 48, b1 >> 1);
		if (flags & (MODES_ALLOWED_PUNCHTHROUGH_ONLY)) {
			data0 = set_bits_uint64(data0, 49, 55, 0x00);
			data0 = set_bits_uint64(data0, 56, 62, 0x7F);
		}
		*(uint64_t *)&bitstring[0] = data0;
//		printf("bptc_set_block_colors: Colors set for mode 6.\n");
	}
}

// Draw a 4x4 pixel block using the BPTC/BC7 texture compression data in bitstring.

int draw_block4x4_bptc(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	Block block;
	block.data0 = *(uint64_t *)&bitstring[0];
	block.data1 = *(uint64_t *)&bitstring[8];
	block.index = 0;
	int mode = extract_mode(&block);
	if (mode == - 1)
		return 0;
	// Allow compression tied to specific modes (according to flags).
	if (!(flags & ((int)1 << mode)))
		return 0;
	if (mode >= 4 && (flags & MODES_ALLOWED_OPAQUE_ONLY))
		return 0;
	if (mode < 4 && (flags & MODES_ALLOWED_NON_OPAQUE_ONLY))
		return 0;
	if (mode == 1)
		return draw_bptc_mode_1(&block, image_buffer);

	int nu_subsets = 1;
	int partition_set_id = 0;
	if (mode_has_partition_bits[mode]) {
		nu_subsets = get_nu_subsets(mode);
		partition_set_id = extract_partition_set_id(&block, mode);
	}
	int rotation = extract_rot_bits(&block, mode);
	int index_selection_bit = 0;
	if (mode == 4)
		index_selection_bit = block_extract_bits(&block, 1);

	int alpha_index_bitcount = get_alpha_index_bitcount(mode, index_selection_bit);
	int color_index_bitcount = get_color_index_bitcount(mode, index_selection_bit);

	uint8_t endpoint_array[3 * 2 * 4];	// Max. 3 subsets.
	extract_endpoints(mode, nu_subsets, &block, endpoint_array);
	fully_decode_endpoints(endpoint_array, nu_subsets, mode, &block);

	uint8_t subset_index[16];
	for (int i = 0; i < 16; i++)
		// subset_index[i] is a number from 0 to 2, or 0 to 1, or 0 depending on the number of subsets.
		subset_index[i] = get_partition_index(nu_subsets, partition_set_id, i);
	uint8_t anchor_index[4];	// Only need max. 3 elements.
	for (int i = 0; i < nu_subsets; i++)
		anchor_index[i] = get_anchor_index(partition_set_id, i, nu_subsets);
	uint8_t color_index[16];
	uint8_t alpha_index[16];
	// Extract primary index bits.
	uint64_t data1;
	if (block.index >= 64) {
		// Because the index bits are all in the second 64-bit word, there is no need to use
		// block_extract_bits().
		// This implies the mode is not 4.
		data1 = block.data1 >> (block.index - 64);
		uint8_t mask1 = (1 << IB[mode]) - 1;
		uint8_t mask2 = (1 << (IB[mode] - 1)) - 1;
		for (int i = 0; i < 16; i++)
			if (i == anchor_index[subset_index[i]]) {
				// Highest bit is zero.
				color_index[i] = data1 & mask2;
				data1 >>= IB[mode] - 1;
				alpha_index[i] = color_index[i];
			}
			else {
				color_index[i] = data1 & mask1;
				data1 >>= IB[mode];
				alpha_index[i] = color_index[i];
			}
	}
	else {	// Implies mode 4.
		// Because the bits cross the 64-bit word boundary, we have to be careful.
		// Block index is 50 at this point.
		uint64_t data = block.data0 >> 50;
		data |= block.data1 << 14;
		for (int i = 0; i < 16; i++)
			if (i == anchor_index[subset_index[i]]) {
				// Highest bit is zero.
				if (index_selection_bit) {	// Implies mode == 4.
					alpha_index[i] = data & 0x1;
					data >>= 1;
				}
				else {
					color_index[i] = data & 0x1;
					data >>= 1;
				}
			}
			else {
				if (index_selection_bit) {	// Implies mode == 4.
					alpha_index[i] = data & 0x3;
					data >>= 2;
				}
				else {
					color_index[i] = data & 0x3;
					data >>= 2;
				}
			}
		// Block index is 81 at this point.
		data1 = block.data1 >> (81 - 64);
	}
	// Extract secondary index bits.
	if (IB2[mode] > 0) {
		uint8_t mask1 = (1 << IB2[mode]) - 1;
		uint8_t mask2 = (1 << (IB2[mode] - 1)) - 1;
		for (int i = 0; i < 16; i++)
			if (i == anchor_index[subset_index[i]]) {
				// Highest bit is zero.
				if (index_selection_bit) {
					color_index[i] = data1 & 0x3;
					data1 >>= 2;
				}
				else {
//					alpha_index[i] = block_extract_bits(&block, IB2[mode] - 1);
					alpha_index[i] = data1 & mask2;
					data1 >>= IB2[mode] - 1;
				}
			}
			else {
				if (index_selection_bit) {
					color_index[i] = data1 & 0x7;
					data1 >>= 3;
				}
				else {
//					alpha_index[i] = block_extract_bits(&block, IB2[mode]);
					alpha_index[i] = data1 & mask1;
					data1 >>= IB2[mode];
				}
			}
	}

	for (int i = 0; i < 16; i++) {
		uint8_t endpoint_start[4];
		uint8_t endpoint_end[4];
		for (int j = 0; j < 4; j++) {
			 endpoint_start[j] = endpoint_array[2 * subset_index[i] * 4 + j];
			 endpoint_end[j] = endpoint_array[(2 * subset_index[i] + 1) * 4 + j];
		}

		uint32_t output = 0;
		output = pack_r(interpolate(endpoint_start[0], endpoint_end[0], color_index[i], color_index_bitcount));
		output |= pack_g(interpolate(endpoint_start[1], endpoint_end[1], color_index[i], color_index_bitcount));
		output |= pack_b(interpolate(endpoint_start[2], endpoint_end[2], color_index[i], color_index_bitcount));
		output |= pack_a(interpolate(endpoint_start[3], endpoint_end[3], alpha_index[i], alpha_index_bitcount));
   
		if (rotation > 0) {
			if (rotation == 1)
				output = pack_rgba(pixel_get_a(output), pixel_get_g(output), pixel_get_b(output),
					pixel_get_r(output));
			else
			if (rotation == 2)
				output = pack_rgba(pixel_get_r(output), pixel_get_a(output), pixel_get_b(output),
					pixel_get_g(output));
			else // rotation == 3
				output = pack_rgba(pixel_get_r(output), pixel_get_g(output), pixel_get_a(output),
					pixel_get_b(output));
		}
		image_buffer[i] = output;
	}
	return 1;
}

static uint32_t get_reversed_bits_uint64(uint64_t data, int bit0, int bit1) {
	// Assumes bit0 > bit1.
	// Reverse the bits.
	uint32_t val = 0;
	for (int i = 0; i <= bit0 - bit1; i++) {
		int shift_right = bit0 - 2 * i;
		if (shift_right >= 0)
			val |= (data & ((uint64_t)1 << (bit0 - i))) >> shift_right;
		else
			val |= (data & ((uint64_t)1 << (bit0 - i))) << (- shift_right);
	}
	return val;
}

// Optimized version of BPTC decode for mode 1, the most common mode.

static int draw_bptc_mode_1(Block *block, unsigned int *image_buffer) {
	uint64_t data0 = block->data0;
	uint64_t data1 = block->data1;
	int partition_set_id = get_bits_uint64(data0, 2, 7);
	uint8_t endpoint[2 * 2 * 3];	// 2 subsets.
	endpoint[0] = get_bits_uint64(data0, 8, 13);	// red, subset 0, endpoint 0
	endpoint[3] = get_bits_uint64(data0, 14, 19);	// red, subset 0, endpoint 1
	endpoint[6] = get_bits_uint64(data0, 20, 25);	// red, subset 1, endpoint 0
	endpoint[9] = get_bits_uint64(data0, 26, 31);	// red, subset 1, endpoint 1
	endpoint[1] = get_bits_uint64(data0, 32, 37);	// green, subset 0, endpoint 0
	endpoint[4] = get_bits_uint64(data0, 38, 43);	// green, subset 0, endpoint 1
	endpoint[7] = get_bits_uint64(data0, 44, 49);	// green, subset 1, endpoint 0
	endpoint[10] = get_bits_uint64(data0, 50, 55);	// green, subset 1, endpoint 1
	endpoint[2] = get_bits_uint64(data0, 56, 61);	// blue, subset 0, endpoint 0
	endpoint[5] = get_bits_uint64(data0, 62, 63)	// blue, subset 0, endpoint 1
		| (get_bits_uint64(data1, 0, 3) << 2);
	endpoint[8] = get_bits_uint64(data1, 4, 9);	// blue, subset 1, endpoint 0
	endpoint[11] = get_bits_uint64(data1, 10, 15);	// blue, subset 1, endpoint 1
	// Decode endpoints.
	for (int i = 0; i < 2 * 2; i++) {
		//component-wise left-shift
		endpoint[i * 3 + 0] <<= 2;
		endpoint[i * 3 + 1] <<= 2;
		endpoint[i * 3 + 2] <<= 2;
       	}
	// P-bit is shared.
	uint8_t pbit_zero = get_bits_uint64(data1, 16, 16) << 1;
	uint8_t pbit_one = get_bits_uint64(data1, 17, 17) << 1;
        // RGB only pbits for mode 1, one for each subset.
	for (int j = 0; j < 3; j++) {
		endpoint[0 * 3 + j] |= pbit_zero;
		endpoint[1 * 3 + j] |= pbit_zero;
		endpoint[2 * 3 + j] |= pbit_one;
		endpoint[3 * 3 + j] |= pbit_one;
	}
	for (int i = 0; i < 2 * 2; i++) {
	        // Replicate each component's MSB into the LSB.
		endpoint[i * 3 + 0] |= endpoint[i * 3 + 0] >> 7;
		endpoint[i * 3 + 1] |= endpoint[i * 3 + 1] >> 7;
		endpoint[i * 3 + 2] |= endpoint[i * 3 + 2] >> 7;
	}
 
	uint8_t subset_index[16];
	for (int i = 0; i < 16; i++)
		// subset_index[i] is a number from 0 to 1.
		subset_index[i] = table_P2[partition_set_id * 16 + i];
	uint8_t anchor_index[2];
	anchor_index[0] = 0;
	anchor_index[1] = anchor_index_second_subset[partition_set_id];
	uint8_t color_index[16];
	// Extract primary index bits.
	data1 >>= 18;
	for (int i = 0; i < 16; i++)
		if (i == anchor_index[subset_index[i]]) {
			// Highest bit is zero.
			color_index[i] = data1 & 3; // Get two bits.
			data1 >>= 2;
		}
		else {
			color_index[i] = data1 & 7;	// Get three bits.
			data1 >>= 3;
		}
	for (int i = 0; i < 16; i++) {
		uint8_t endpoint_start[3];
		uint8_t endpoint_end[3];
		for (int j = 0; j < 3; j++) {
			 endpoint_start[j] = endpoint[2 * subset_index[i] * 3 + j];
			 endpoint_end[j] = endpoint[(2 * subset_index[i] + 1) * 3 + j];
		}
		uint32_t output;
		output = pack_r(interpolate(endpoint_start[0], endpoint_end[0], color_index[i], 3));
		output |= pack_g(interpolate(endpoint_start[1], endpoint_end[1], color_index[i], 3));
		output |= pack_b(interpolate(endpoint_start[2], endpoint_end[2], color_index[i], 3));
		output |= pack_a(0xFF);
		image_buffer[i] = output;
	}
	return 1;
}


// BPTC float (BC6H) decoding.

static char map_mode_table[32] = {
	0, 1, 2, 10, -1, -1, 3, 11, -1, -1, 4, 12, -1, -1, 5, 13, -1, -1, 6, -1, -1, -1, 7, -1, -1, -1, 8,
	-1, -1, -1, 9, -1 };

static int extract_bptc_float_mode(Block *block) {
	int mode = block_extract_bits(block, 2);
	if (mode < 2)
		return mode;
	return map_mode_table[mode | (block_extract_bits(block, 3) << 2)];
}

static int bptc_float_get_partition_index(int nu_subsets, int partition_set_id, int i) {
	if (nu_subsets == 1)
		return 0;
	// nu_subset == 2
	return table_P2[partition_set_id * 16 + i];
}

static char bptc_float_EPB[14] = {
	10, 7, 11, 11, 11, 9, 8, 8, 8, 6, 10, 11, 12, 16 };

static uint32_t unquantize(uint16_t x, int mode) {
	int32_t unq;
	if (mode == 13)
		unq = x;
	else if (x == 0)
		unq = 0;
	else if (x == (((int32_t)1 << bptc_float_EPB[mode]) - 1))
		unq = 0xFFFF;
	else
		unq = (((int32_t)x << 15) + 0x4000) >> (bptc_float_EPB[mode] - 1);
	return unq;
}

static int32_t unquantize_signed(int16_t x, int mode) {
	int s = 0;
	int32_t unq;
	if (bptc_float_EPB[mode] >= 16)
		unq = x;
	else {
		if (x < 0) {
			s = 1;
			x = -x;
		}
		if (x == 0)
			unq = 0;
		else
		if (x >= (((int32_t)1 << (bptc_float_EPB[mode] - 1)) - 1))
			unq = 0x7FFF;
		else
			unq = (((int32_t)x << 15) + 0x4000) >> (bptc_float_EPB[mode] - 1);
		if (s)
			unq = -unq;
	}
	return unq;
}

static int sign_extend(int value, int source_nu_bits, int target_nu_bits) {
	uint32_t sign_bit = value & (1 << (source_nu_bits - 1));
	if (!sign_bit)
		return value;
	uint32_t sign_extend_bits = 0xFFFFFFFF ^ ((1 << source_nu_bits) - 1);
	sign_extend_bits &= ((uint64_t)1 << target_nu_bits) - 1;
	return value | sign_extend_bits;
}

static int32_t interpolate_float(int32_t e0, int32_t e1, int16_t index, uint8_t indexprecision) {
	if (indexprecision == 2)
		return (((64 - aWeight2[index]) * (int32_t)e0 + aWeight2[index] * (int32_t)e1 + 32) >> 6);
	else
	if (indexprecision == 3)
		return (((64 - aWeight3[index]) * (int32_t)e0 + aWeight3[index] * (int32_t)e1 + 32) >> 6);
	else // indexprecision == 4
		return (((64 - aWeight4[index]) * (int32_t)e0 + aWeight4[index] * (int32_t)e1 + 32) >> 6);
}

int block4x4_bptc_float_get_mode(const unsigned char *bitstring) {
	Block block;
	block.data0 = *(uint64_t *)&bitstring[0];
	block.data1 = *(uint64_t *)&bitstring[8];
	block.index = 0;
	uint32_t mode = extract_bptc_float_mode(&block);
	return mode;
}

static uint8_t bptc_float_set_mode_table[14] = {
	0, 1, 2, 6, 10, 14, 18, 22, 26, 30, 3, 7, 11, 15
};

void block4x4_bptc_float_set_mode(unsigned char *bitstring, int flags) {
	int mode_flags = flags & BPTC_FLOAT_MODE_ALLOWED_ALL;
	if (mode_flags & 0x3) {
		// Set mode 0 or 1.
		bitstring[0] = (bitstring[0] & 0xFC) | ((mode_flags & 0x2) >> 1);
		return;
	}
	uint8_t byte0 = bitstring[0];
	byte0 &= 0xE0;
	for (int i = 2; i < 14; i++)
		if (flags & (1 << i)) {
			byte0 |= bptc_float_set_mode_table[i];
			bitstring[0] = byte0;
			return;
		}
}

int draw_block4x4_bptc_float_shared(const unsigned char *bitstring, unsigned int *image_buffer, int signed_flag, int flags) {
	Block block;
	block.data0 = *(uint64_t *)&bitstring[0];
	block.data1 = *(uint64_t *)&bitstring[8];
	block.index = 0;
	uint32_t mode = extract_bptc_float_mode(&block);
	if (mode == - 1)
		return 0;
	// Allow compression tied to specific modes (according to flags).
	if (!(flags & ((int)1 << mode)))
		return 0;
	int32_t r[4], g[4], b[4];
	int partition_set_id = 0;
	int delta_bits_r, delta_bits_g, delta_bits_b;
	uint64_t data0 = block.data0;
	uint64_t data1 = block.data1;
	switch (mode) {
	case 0 :
		// m[1:0],g2[4],b2[4],b3[4],r0[9:0],g0[9:0],b0[9:0],r1[4:0],g3[4],g2[3:0],
		// g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
		g[2] = get_bits_uint64(data0, 2, 2) << 4;
		b[2] = get_bits_uint64(data0, 3, 3) << 4;
		b[3] = get_bits_uint64(data0, 4, 4) << 4;
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 39);
		g[3] = get_bits_uint64(data0, 40, 40) << 4;
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 49);
		b[3] |= get_bits_uint64(data0, 50, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 59);
		b[3] |= get_bits_uint64(data0, 60, 60) << 1;
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 5);
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 11);
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = delta_bits_b = 5;
		break;
	case 1 :
		// m[1:0],g2[5],g3[4],g3[5],r0[6:0],b3[0],b3[1],b2[4],g0[6:0],b2[5],b3[2],
		// g2[4],b0[6:0],b3[3],b3[5],b3[4],r1[5:0],g2[3:0],g1[5:0],g3[3:0],b1[5:0],
		// b2[3:0],r2[5:0],r3[5:0]
		g[2] = get_bits_uint64(data0, 2, 2) << 5;
		g[3] = get_bits_uint64(data0, 3, 3) << 4;
		g[3] |= get_bits_uint64(data0, 4, 4) << 5;
		r[0] = get_bits_uint64(data0, 5, 11);
		b[3] = get_bits_uint64(data0, 12, 12);
		b[3] |= get_bits_uint64(data0, 13, 13) << 1;
		b[2] = get_bits_uint64(data0, 14, 14) << 4;
		g[0] = get_bits_uint64(data0, 15, 21);
		b[2] |= get_bits_uint64(data0, 22, 22) << 5;
		b[3] |= get_bits_uint64(data0, 23, 23) << 2;
		g[2] |= get_bits_uint64(data0, 24, 24) << 4;
		b[0] = get_bits_uint64(data0, 25, 31);
		b[3] |= get_bits_uint64(data0, 32, 32) << 3;
		b[3] |= get_bits_uint64(data0, 33, 33) << 5;
		b[3] |= get_bits_uint64(data0, 34, 34) << 4;
		r[1] = get_bits_uint64(data0, 35, 40);
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 60);
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 6);
		r[3] = get_bits_uint64(data1, 7, 12);
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = delta_bits_b = 6;
		break;
	case 2 :
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[4:0],r0[10],g2[3:0],g1[3:0],g0[10],
		// b3[0],g3[3:0],b1[3:0],b0[10],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 39);
		r[0] |= get_bits_uint64(data0, 40, 40) << 10;
		g[2] = get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 48);
		g[0] |= get_bits_uint64(data0, 49, 49) << 10;
		b[3] = get_bits_uint64(data0, 50, 50);
		g[3] = get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 58);
		b[0] |= get_bits_uint64(data0, 59, 59) << 10;
		b[3] |= get_bits_uint64(data0, 60, 60) << 1;
		b[2] = get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 5);
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 11);
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = 5;
		delta_bits_g = delta_bits_b = 4;
		break;
	case 3 :	// Original mode 6.
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10],g3[4],g2[3:0],g1[4:0],
		// g0[10],g3[3:0],b1[3:0],b0[10],b3[1],b2[3:0],r2[3:0],b3[0],b3[2],r3[3:0],
		// g2[4],b3[3]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 38);
		r[0] |= get_bits_uint64(data0, 39, 39) << 10;
		g[3] = get_bits_uint64(data0, 40, 40) << 4;
		g[2] = get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 49);
		g[0] |= get_bits_uint64(data0, 50, 50) << 10;
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 58);
		b[0] |= get_bits_uint64(data0, 59, 59) << 10;
		b[3] = get_bits_uint64(data0, 60, 60) << 1;
		b[2] = get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 4);
		b[3] |= get_bits_uint64(data1, 5, 5);
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 10);
		g[2] |= get_bits_uint64(data1, 11, 11) << 4;
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_b = 4;
		delta_bits_g = 5;
		break;
	case 4 :	// Original mode 10.
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10],b2[4],g2[3:0],g1[3:0],
		// g0[10],b3[0],g3[3:0],b1[4:0],b0[10],b2[3:0],r2[3:0],b3[1],b3[2],r3[3:0],
		// b3[4],b3[3]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 38);
		r[0] |= get_bits_uint64(data0, 39, 39) << 10;
		b[2] = get_bits_uint64(data0, 40, 40) << 4;
		g[2] = get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 48);
		g[0] |= get_bits_uint64(data0, 49, 49) << 10;
		b[3] = get_bits_uint64(data0, 50, 50);
		g[3] = get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 59);
		b[0] |= get_bits_uint64(data0, 60, 60) << 10;
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 4);
		b[3] |= get_bits_uint64(data1, 5, 5) << 1;
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 10);
		b[3] |= get_bits_uint64(data1, 11, 11) << 4;
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = 4;
		delta_bits_b = 5;
		break;
	case 5 :	// Original mode 14
		// m[4:0],r0[8:0],b2[4],g0[8:0],g2[4],b0[8:0],b3[4],r1[4:0],g3[4],g2[3:0],
		// g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],b3[2],r3[4:0],b3[3]
		r[0] = get_bits_uint64(data0, 5, 13);
		b[2] = get_bits_uint64(data0, 14, 14) << 4;
		g[0] = get_bits_uint64(data0, 15, 23);
		g[2] = get_bits_uint64(data0, 24, 24) << 4;
		b[0] = get_bits_uint64(data0, 25, 33);
		b[3] = get_bits_uint64(data0, 34, 34) << 4;
		r[1] = get_bits_uint64(data0, 35, 39);
		g[3] = get_bits_uint64(data0, 40, 40) << 4;
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 49);
		b[3] |= get_bits_uint64(data0, 50, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 59);
		b[3] |= get_bits_uint64(data0, 60, 60) << 1;
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 5);
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 11);
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = delta_bits_b = 5;
		break;
	case 6 :	// Original mode 18
		// m[4:0],r0[7:0],g3[4],b2[4],g0[7:0],b3[2],g2[4],b0[7:0],b3[3],b3[4],
		// r1[5:0],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[5:0],r3[5:0]
		r[0] = get_bits_uint64(data0, 5, 12);
		g[3] = get_bits_uint64(data0, 13, 13) << 4;
		b[2] = get_bits_uint64(data0, 14, 14) << 4;
		g[0] = get_bits_uint64(data0, 15, 22);
		b[3] = get_bits_uint64(data0, 23, 23) << 2;
		g[2] = get_bits_uint64(data0, 24, 24) << 4;
		b[0] = get_bits_uint64(data0, 25, 32);
		b[3] |= get_bits_uint64(data0, 33, 33) << 3;
		b[3] |= get_bits_uint64(data0, 34, 34) << 4;
		r[1] = get_bits_uint64(data0, 35, 40);
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 49);
		b[3] |= get_bits_uint64(data0, 50, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 59);
		b[3] |= get_bits_uint64(data0, 60, 60) << 1;
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 6);
		r[3] = get_bits_uint64(data1, 7, 12);
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = 6;
		delta_bits_g = delta_bits_b = 5;
		break;
	case 7 :	// Original mode 22
		// m[4:0],r0[7:0],b3[0],b2[4],g0[7:0],g2[5],g2[4],b0[7:0],g3[5],b3[4],
		// r1[4:0],g3[4],g2[3:0],g1[5:0],g3[3:0],b1[4:0],b3[1],b2[3:0],r2[4:0],
		// b3[2],r3[4:0],b3[3]
		r[0] = get_bits_uint64(data0, 5, 12);
		b[3] = get_bits_uint64(data0, 13, 13);
		b[2] = get_bits_uint64(data0, 14, 14) << 4;
		g[0] = get_bits_uint64(data0, 15, 22);
		g[2] = get_bits_uint64(data0, 23, 23) << 5;
		g[2] |= get_bits_uint64(data0, 24, 24) << 4;
		b[0] = get_bits_uint64(data0, 25, 32);
		g[3] = get_bits_uint64(data0, 33, 33) << 5;
		b[3] |= get_bits_uint64(data0, 34, 34) << 4;
		r[1] = get_bits_uint64(data0, 35, 39);
		g[3] |= get_bits_uint64(data0, 40, 40) << 4;
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 59);
		b[3] |= get_bits_uint64(data0, 60, 60) << 1;
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 5);
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 11);
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_b = 5;
		delta_bits_g = 6;
		break;
	case 8 :	// Original mode 26
		// m[4:0],r0[7:0],b3[1],b2[4],g0[7:0],b2[5],g2[4],b0[7:0],b3[5],b3[4],
		// r1[4:0],g3[4],g2[3:0],g1[4:0],b3[0],g3[3:0],b1[5:0],b2[3:0],r2[4:0],
		// b3[2],r3[4:0],b3[3]
		r[0] = get_bits_uint64(data0, 5, 12);
		b[3] = get_bits_uint64(data0, 13, 13) << 1;
		b[2] = get_bits_uint64(data0, 14, 14) << 4;
		g[0] = get_bits_uint64(data0, 15, 22);
		b[2] |= get_bits_uint64(data0, 23, 23) << 5;
		g[2] = get_bits_uint64(data0, 24, 24) << 4;
		b[0] = get_bits_uint64(data0, 25, 32);
		b[3] |= get_bits_uint64(data0, 33, 33) << 5;
		b[3] |= get_bits_uint64(data0, 34, 34) << 4;
		r[1] = get_bits_uint64(data0, 35, 39);
		g[3] = get_bits_uint64(data0, 40, 40) << 4;
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 49);
		b[3] |= get_bits_uint64(data0, 50, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 60);
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 5);
		b[3] |= get_bits_uint64(data1, 6, 6) << 2;
		r[3] = get_bits_uint64(data1, 7, 11);
		b[3] |= get_bits_uint64(data1, 12, 12) << 3;
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
		delta_bits_r = delta_bits_g = 5;
		delta_bits_b = 6;
		break;
	case 9 :	// Original mode 30
		// m[4:0],r0[5:0],g3[4],b3[0],b3[1],b2[4],g0[5:0],g2[5],b2[5],b3[2],
		// g2[4],b0[5:0],g3[5],b3[3],b3[5],b3[4],r1[5:0],g2[3:0],g1[5:0],g3[3:0],
		// b1[5:0],b2[3:0],r2[5:0],r3[5:0]
		r[0] = get_bits_uint64(data0, 5, 10);
		g[3] = get_bits_uint64(data0, 11, 11) << 4;
		b[3] = get_bits_uint64(data0, 12, 13);
		b[2] = get_bits_uint64(data0, 14, 14) << 4;
		g[0] = get_bits_uint64(data0, 15, 20);
		g[2] = get_bits_uint64(data0, 21, 21) << 5;
		b[2] |= get_bits_uint64(data0, 22, 22) << 5;
		b[3] |= get_bits_uint64(data0, 23, 23) << 2;
		g[2] |= get_bits_uint64(data0, 24, 24) << 4;
		b[0] = get_bits_uint64(data0, 25, 30);
		g[3] |= get_bits_uint64(data0, 31, 31) << 5;
		b[3] |= get_bits_uint64(data0, 32, 32) << 3;
		b[3] |= get_bits_uint64(data0, 33, 33) << 5;
		b[3] |= get_bits_uint64(data0, 34, 34) << 4;
		r[1] = get_bits_uint64(data0, 35, 40);
		g[2] |= get_bits_uint64(data0, 41, 44);
		g[1] = get_bits_uint64(data0, 45, 50);
		g[3] |= get_bits_uint64(data0, 51, 54);
		b[1] = get_bits_uint64(data0, 55, 60);
		b[2] |= get_bits_uint64(data0, 61, 63);
		b[2] |= get_bits_uint64(data1, 0, 0) << 3;
		r[2] = get_bits_uint64(data1, 1, 6);
		r[3] = get_bits_uint64(data1, 7, 12);
		partition_set_id = get_bits_uint64(data1, 13, 17);
		block.index = 64 + 18;
//		delta_bits_r = delta_bits_g = delta_bits_b = 6;
		break;
	case 10 :	// Original mode 3
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[9:0],g1[9:0],b1[9:0]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 44);
		g[1] = get_bits_uint64(data0, 45, 54);
		b[1] = get_bits_uint64(data0, 55, 63);
		b[1] |= get_bits_uint64(data1, 0, 0) << 9;
		partition_set_id = 0;
		block.index = 65;
//		delta_bits_r = delta_bits_g = delta_bits_b = 10;
		break;
	case 11 :	// Original mode 7
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[8:0],r0[10],g1[8:0],g0[10],b1[8:0],b0[10]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 43);
		r[0] |= get_bits_uint64(data0, 44, 44) << 10;
		g[1] = get_bits_uint64(data0, 45, 53);
		g[0] |= get_bits_uint64(data0, 54, 54) << 10;
		b[1] = get_bits_uint64(data0, 55, 63);
		b[0] |= get_bits_uint64(data1, 0, 0) << 10;
		partition_set_id = 0;
		block.index = 65;
		delta_bits_r = delta_bits_g = delta_bits_b = 9;
		break;
	case 12 :	// Original mode 11
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[7:0],r0[10:11],g1[7:0],g0[10:11],
		// b1[7:0],b0[10:11]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 42);
		r[0] |= get_reversed_bits_uint64(data0, 44, 43) << 10;	// Reversed.
		g[1] = get_bits_uint64(data0, 45, 52);
		g[0] |= get_reversed_bits_uint64(data0, 54, 53) << 10;	// Reversed.
		b[1] = get_bits_uint64(data0, 55, 62);
		b[0] |= get_bits_uint64(data0, 63, 63) << 11;	// MSB
		b[0] |= get_bits_uint64(data1, 0, 0) << 10;	// LSB
		partition_set_id = 0;
		block.index = 65;
		delta_bits_r = delta_bits_g = delta_bits_b = 8;
		break;
	case 13 :	// Original mode 15
		// m[4:0],r0[9:0],g0[9:0],b0[9:0],r1[3:0],r0[10:15],g1[3:0],g0[10:15],
		// b1[3:0],b0[10:15]
		r[0] = get_bits_uint64(data0, 5, 14);
		g[0] = get_bits_uint64(data0, 15, 24);
		b[0] = get_bits_uint64(data0, 25, 34);
		r[1] = get_bits_uint64(data0, 35, 38);
		r[0] |= get_reversed_bits_uint64(data0, 44, 39) << 10;	// Reversed.
		g[1] = get_bits_uint64(data0, 45, 48);
		g[0] |= get_reversed_bits_uint64(data0, 54, 49) << 10;	// Reversed.
		b[1] = get_bits_uint64(data0, 55, 58);
		b[0] |= get_reversed_bits_uint64(data0, 63, 59) << 11;	// Reversed.
		b[0] |= get_bits_uint64(data1, 0, 0) << 10;
		partition_set_id = 0;
		block.index = 65;
		delta_bits_r = delta_bits_g = delta_bits_b = 4;
		break;
	}
	int nu_subsets;
	if (mode >= 10)
		nu_subsets = 1;
	else
		nu_subsets = 2;
	if (signed_flag) {
		r[0] = sign_extend(r[0], bptc_float_EPB[mode], 32);
		g[0] = sign_extend(g[0], bptc_float_EPB[mode], 32);
		b[0] = sign_extend(b[0], bptc_float_EPB[mode], 32);
	}
	if (mode != 9 && mode != 10) {
		// Transformed endpoints.
		for (int i = 1; i < nu_subsets * 2; i++) {
			r[i] = sign_extend(r[i], delta_bits_r, 32);
			r[i] = (r[0] + r[i]) & (((uint32_t)1 << bptc_float_EPB[mode]) - 1);
			g[i] = sign_extend(g[i], delta_bits_g, 32);
			g[i] = (g[0] + g[i]) & (((uint32_t)1 << bptc_float_EPB[mode]) - 1);
			b[i] = sign_extend(b[i], delta_bits_b, 32);
			b[i] = (b[0] + b[i]) & (((uint32_t)1 << bptc_float_EPB[mode]) - 1);
			if (signed_flag) {
				r[i] = sign_extend(r[i], bptc_float_EPB[mode], 32);
				g[i] = sign_extend(g[i], bptc_float_EPB[mode], 32);
				b[i] = sign_extend(b[i], bptc_float_EPB[mode], 32);
			}
		}
	}
	else	// Mode 9 or 10, no transformed endpoints.
	if (signed_flag)
		for (int i = 1; i < nu_subsets * 2; i++) {
			r[i] = sign_extend(r[i], bptc_float_EPB[mode], 32);
			g[i] = sign_extend(g[i], bptc_float_EPB[mode], 32);
			b[i] = sign_extend(b[i], bptc_float_EPB[mode], 32);
		}

	// Unquantize endpoints.
	if (signed_flag)
		for (int i = 0; i < 2 * nu_subsets; i++) {
			r[i] = unquantize_signed(r[i], mode);
			g[i] = unquantize_signed(g[i], mode);
			b[i] = unquantize_signed(b[i], mode);
		}
	else
		for (int i = 0; i < 2 * nu_subsets; i++) {
			r[i] = unquantize(r[i], mode);
			g[i] = unquantize(g[i], mode);
			b[i] = unquantize(b[i], mode);
		}

	uint8_t subset_index[16];
	for (int i = 0; i < 16; i++) {
		// subset_index[i] is a number from 0 to 1, depending on the number of subsets.
		subset_index[i] = bptc_float_get_partition_index(nu_subsets, partition_set_id, i);
	}
	uint8_t anchor_index[4];	// Only need max. 2 elements
	for (int i = 0; i < nu_subsets; i++)
		anchor_index[i] = get_anchor_index(partition_set_id, i, nu_subsets);
	uint8_t color_index[16];
	// Extract index bits.
	int color_index_bit_count = 3;
	if ((bitstring[0] & 3) == 3)	// This defines original modes 3, 7, 11, 15
		color_index_bit_count = 4;
	// Because the index bits are all in the second 64-bit word, there is no need to use
	// block_extract_bits().
	data1 >>= (block.index - 64);
	uint8_t mask1 = (1 << color_index_bit_count) - 1;
	uint8_t mask2 = (1 << (color_index_bit_count - 1)) - 1;
	for (int i = 0; i < 16; i++) {
		if (i == anchor_index[subset_index[i]]) {
			// Highest bit is zero.
//			color_index[i] = block_extract_bits(&block, color_index_bit_count - 1);
			color_index[i] = data1 & mask2;
			data1 >>= color_index_bit_count - 1;
		}
		else {
//			color_index[i] = block_extract_bits(&block, color_index_bit_count);
			color_index[i] = data1 & mask1;	
			data1 >>= color_index_bit_count;
		}
	}

	for (int i = 0; i < 16; i++) {
		int32_t endpoint_start_r, endpoint_start_g, endpoint_start_b;
		int32_t endpoint_end_r, endpoint_end_g, endpoint_end_b;
		endpoint_start_r = r[2 * subset_index[i]];
		endpoint_end_r = r[2 * subset_index[i] + 1];
		endpoint_start_g = g[2 * subset_index[i]];
		endpoint_end_g = g[2 * subset_index[i] + 1];
		endpoint_start_b = b[2 * subset_index[i]];
		endpoint_end_b = b[2 * subset_index[i] + 1];
		uint64_t output;
		if (signed_flag) {
			int32_t r16 = interpolate_float(endpoint_start_r, endpoint_end_r, color_index[i],
				color_index_bit_count);
			if (r16 < 0)
				r16 = - (((- r16) * 31) >> 5);
			else
				r16 = (r16 * 31) >> 5;
			int s = 0;
			if (r16 < 0) {
				s = 0x8000;
				r16 = - r16;
			}
			r16 |= s;
			int32_t g16 = interpolate_float(endpoint_start_g, endpoint_end_g, color_index[i],
				color_index_bit_count);
			if (g16 < 0)
				g16 = - (((- g16) * 31) >> 5);
			else
				g16 = (g16 * 31) >> 5;
			s = 0;
			if (g16 < 0) {
				s = 0x8000;
				g16 = - g16;
			}
			g16 |= s;
			int32_t b16 = interpolate_float(endpoint_start_b, endpoint_end_b, color_index[i],
				color_index_bit_count);
			if (b16 < 0)
				b16 = - (((- b16) * 31) >> 5);
			else
				b16 = (b16 * 31) >> 5;
			s = 0;
			if (b16 < 0) {
				s = 0x8000;
				b16 = - b16;
			}
			b16 |= s;
			output = pack_rgb16(r16, g16, b16);
		}
		else {
			output = pack_r16(interpolate_float(endpoint_start_r, endpoint_end_r, color_index[i],
				color_index_bit_count) * 31 / 64);
			output |= pack_g16(interpolate_float(endpoint_start_g, endpoint_end_g, color_index[i],
				color_index_bit_count) * 31 / 64);
			output |= pack_b16(interpolate_float(endpoint_start_b, endpoint_end_b, color_index[i],
				color_index_bit_count) * 31 / 64);
		}
		*(uint64_t *)&image_buffer[i * 2] = output;
	}
	return 1;
}

int draw_block4x4_bptc_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	return draw_block4x4_bptc_float_shared(bitstring, image_buffer, 0, flags);
}

int draw_block4x4_bptc_signed_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags) {
	return draw_block4x4_bptc_float_shared(bitstring, image_buffer, 1, flags);
}


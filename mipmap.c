/*
    mipmap.c -- part of texgenpack, a texture compressor using fgen.

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
#include <stdint.h>
#include <stdio.h>
#include "texgenpack.h"
#include "packing.h"

// Use the averaging method to create an RGB(A)8 mipmap level directly from the original size image. source_image
// must have dimensions that are a power of two. divider must be a power of two.

static void create_mipmap_with_averaging(Image *source_image, int divider, Image *dest_image) {
	int n = divider * divider;
	for (int y = 0; y < source_image->height; y += divider) {
		int dy = y / divider;
		for (int x = 0; x < source_image->width; x += divider) {
			int dx = x / divider;
			// Calculate the average values of the pixel block.
			int r = 0;
			int g = 0;
			int b = 0;
			int a = 0;
			for (int by = y; by < y + divider; by++)
				for (int bx = x; bx < x + divider; bx++) {
					unsigned int pixel = source_image->pixels[by * source_image->extended_width + bx];
					r += pixel_get_r(pixel);
					g += pixel_get_g(pixel);
					b += pixel_get_b(pixel);
					a += pixel_get_a(pixel);
				}
			r /= n;
			g /= n;
			b /= n;
			a /= n;
			if (source_image->alpha_bits == 0)
				a = 0xFF;
			else
			if (source_image->alpha_bits == 1)
				// Avoid smoothing out 1-bit alpha textures. If there are less than n / 2
				// alpha pixels in the source image with alpha 0xFF, then alpha becomes
				// zero, otherwise 0xFF.
				if (a >= 0x80)
					a = 0xFF;
				else
					a = 0;
			unsigned int pixel = pack_rgba(r, g, b, a);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
	}
}

// Use the averaging method to create an RGB(A)8 mipmap level from the previous level image (halving the
// dimension) for power-of-two textures.

static void create_mipmap_with_averaging_divider_2(Image *source_image, Image *dest_image) {
	for (int y = 0; y < source_image->height; y += 2) {
		int dy = y / 2;
		for (int x = 0; x < source_image->width; x += 2) {
			int dx = x / 2;
			// Calculate the average values of the pixel block.
			int r = 0;
			int g = 0;
			int b = 0;
			int a = 0;
			unsigned int pixel = source_image->pixels[y * source_image->extended_width + x];
			r += pixel_get_r(pixel);
			g += pixel_get_g(pixel);
			b += pixel_get_b(pixel);
			a += pixel_get_a(pixel);
			pixel = source_image->pixels[y * source_image->extended_width + x + 1];
			r += pixel_get_r(pixel);
			g += pixel_get_g(pixel);
			b += pixel_get_b(pixel);
			a += pixel_get_a(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x];
			r += pixel_get_r(pixel);
			g += pixel_get_g(pixel);
			b += pixel_get_b(pixel);
			a += pixel_get_a(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x + 1];
			r += pixel_get_r(pixel);
			g += pixel_get_g(pixel);
			b += pixel_get_b(pixel);
			a += pixel_get_a(pixel);
			r /= 4;
			g /= 4;
			b /= 4;
			a /= 4;
			if (source_image->alpha_bits == 0)
				a = 0xFF;
			else
			if (source_image->alpha_bits == 1)
				// Avoid smoothing out 1-bit alpha textures. If there are less than two
				// alpha pixels in the source image with alpha 0xFF, then alpha becomes
				// zero, otherwise 0xFF.
				if (a >= 0x80)
					a = 0xFF;
				else
					a = 0;
			pixel = pack_rgba(r, g, b, a);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
	}
}

// Use the averaging method to create a half-float mipmap level from the previous level image (halving the
// dimension) for power-of-two textures.

static void create_half_float_mipmap_with_averaging_divider_2(Image *source_image, Image *dest_image) {
	for (int y = 0; y < source_image->height; y += 2) {
		int dy = y / 2;
		for (int x = 0; x < source_image->width; x += 2) {
			int dx = x / 2;
			// Calculate the average values of the pixel block.
			float c[4];
			c[0] = c[1] = c[2] = c[3] = 0;
			uint64_t pixel = *(uint64_t *)&source_image->pixels[(y * source_image->extended_width + x) * 2];
			uint16_t h[4];
			h[0] = pixel64_get_r16(pixel);
			h[1] = pixel64_get_g16(pixel);
			h[2] = pixel64_get_b16(pixel);
			h[3] = pixel64_get_a16(pixel);
			float f[4];
			halfp2singles(&f[0], &h[0], 4);
			c[0] += f[0];
			c[1] += f[1];
			c[2] += f[2];
			c[3] += f[3];
			pixel = *(uint64_t *)&source_image->pixels[(y * source_image->extended_width + x + 1) * 2];
			h[0] = pixel64_get_r16(pixel);
			h[1] = pixel64_get_g16(pixel);
			h[2] = pixel64_get_b16(pixel);
			h[3] = pixel64_get_a16(pixel);
			halfp2singles(&f[0], &h[0], 4);
			c[0] += f[0];
			c[1] += f[1];
			c[2] += f[2];
			c[3] += f[3];
			pixel = *(uint64_t *)&source_image->pixels[((y + 1) * source_image->extended_width + x) * 2];
			h[0] = pixel64_get_r16(pixel);
			h[1] = pixel64_get_g16(pixel);
			h[2] = pixel64_get_b16(pixel);
			h[3] = pixel64_get_a16(pixel);
			halfp2singles(&f[0], &h[0], 4);
			c[0] += f[0];
			c[1] += f[1];
			c[2] += f[2];
			c[3] += f[3];
			pixel = *(uint64_t *)&source_image->pixels[((y + 1) * source_image->extended_width + x + 1) * 2];
			h[0] = pixel64_get_r16(pixel);
			h[1] = pixel64_get_g16(pixel);
			h[2] = pixel64_get_b16(pixel);
			h[3] = pixel64_get_a16(pixel);
			halfp2singles(&f[0], &h[0], 4);
			c[0] += f[0];
			c[1] += f[1];
			c[2] += f[2];
			c[3] += f[3];
			c[0] /= 4.0;
			c[1] /= 4.0;
			c[2] /= 4.0;
			c[3] /= 4.0;
			if (source_image->alpha_bits == 0) {
				c[3] = 1.0;
			}
			else
			if (source_image->alpha_bits == 1)
				// Avoid smoothing out 1-bit alpha textures. If there are less than two
				// alpha pixels in the source image with alpha 0xFF, then alpha becomes
				// zero, otherwise 0xFF.
				if (c[3] >= 0.5)
					c[3] = 1.0;
				else
					c[3] = 0;
			singles2halfp(&h[0], &c[0], 4);
			pixel = pack_rgba16(h[0], h[1], h[2], h[3]);
			*(uint64_t *)&dest_image->pixels[(dy * dest_image->extended_width + dx) * 2] = pixel;
		}
	}
}

// Use the averaging method to create a 16-bit mipmap level from the previous level image (halving the
// dimension) for power-of-two textures.

static void create_16_bit_mipmap_with_averaging_divider_2(Image *source_image, Image *dest_image) {
	for (int y = 0; y < source_image->height; y += 2) {
		int dy = y / 2;
		for (int x = 0; x < source_image->width; x += 2) {
			int dx = x / 2;
			// Calculate the average values of the pixel block.
			int r = 0;
			int g = 0;
			unsigned int pixel = source_image->pixels[y * source_image->extended_width + x];
			r += pixel_get_r16(pixel);
			g += pixel_get_g16(pixel);
			pixel = source_image->pixels[y * source_image->extended_width + x + 1];
			r += pixel_get_r16(pixel);
			g += pixel_get_g16(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x];
			r += pixel_get_r16(pixel);
			g += pixel_get_g16(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x + 1];
			r += pixel_get_r16(pixel);
			g += pixel_get_g16(pixel);
			r /= 4;
			g /= 4;
			pixel = pack_r16(r) | pack_g16(g);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
	}
}

// Use the averaging method to create a signed 16-bit mipmap level from the previous level image (halving the
// dimension) for power-of-two textures.

static void create_signed_16_bit_mipmap_with_averaging_divider_2(Image *source_image, Image *dest_image) {
	for (int y = 0; y < source_image->height; y += 2) {
		int dy = y / 2;
		for (int x = 0; x < source_image->width; x += 2) {
			int dx = x / 2;
			// Calculate the average values of the pixel block.
			int r = 0;
			int g = 0;
			unsigned int pixel = source_image->pixels[y * source_image->extended_width + x];
			r += pixel_get_signed_r16(pixel);
			g += pixel_get_signed_g16(pixel);
			pixel = source_image->pixels[y * source_image->extended_width + x + 1];
			r += pixel_get_signed_r16(pixel);
			g += pixel_get_signed_g16(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x];
			r += pixel_get_signed_r16(pixel);
			g += pixel_get_signed_g16(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x + 1];
			r += pixel_get_signed_r16(pixel);
			g += pixel_get_signed_g16(pixel);
			r /= 4;
			g /= 4;
			pixel = pack_r16((uint16_t)(int16_t)r) | pack_g16((uint16_t)(int16_t)g);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
	}
}

// Use the averaging method to create a signed 8-bit mipmap level from the previous level image (halving the
// dimension) for power-of-two textures.

static void create_signed_8_bit_mipmap_with_averaging_divider_2(Image *source_image, Image *dest_image) {
	for (int y = 0; y < source_image->height; y += 2) {
		int dy = y / 2;
		for (int x = 0; x < source_image->width; x += 2) {
			int dx = x / 2;
			// Calculate the average values of the pixel block.
			int r = 0;
			int g = 0;
			unsigned int pixel = source_image->pixels[y * source_image->extended_width + x];
			r += pixel_get_signed_r8(pixel);
			g += pixel_get_signed_g8(pixel);
			pixel = source_image->pixels[y * source_image->extended_width + x + 1];
			r += pixel_get_signed_r8(pixel);
			g += pixel_get_signed_g8(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x];
			r += pixel_get_signed_r8(pixel);
			g += pixel_get_signed_g8(pixel);
			pixel = source_image->pixels[(y + 1) * source_image->extended_width + x + 1];
			r += pixel_get_signed_r8(pixel);
			g += pixel_get_signed_g8(pixel);
			r /= 4;
			g /= 4;
			pixel = pack_r((uint8_t)(int8_t)r) | pack_g((uint8_t)(int8_t)g);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
	}
}

// Helper function to calculate the polyphase weight.

uint32_t calculate_polyphase_weight(int dx, int dy, int j, int i, int w, int h, int source_width, int source_height) {
	uint32_t weighty;
	if (source_height & 1)
		// Rounding down.
		switch (i) {
		case 0 : weighty = 65536 * (h - dy) / (2 * h + 1); break;
		case 1 : weighty = 65536 * h / (2 * h + 1); break;
		case 2 : weighty = 65536 * (1 + dy) / (2 * h + 1); break;
		}
	else
		// No rounding.
		if (i == 2)
			return 0;
		else
			weighty = 65536 / 2;
	uint32_t weightx;
	if (source_width & 1)
		// Rounding down.
		switch (j) {
		case 0 : weightx = 65536 * (w - dx) / (2 * w + 1); break;
		case 1 : weightx = 65536 * w / (2 * w + 1); break;
		case 2 : weightx = 65536 * (1 + dx) / (2 * w + 1); break;
		}
	else
		// No rounding.
		if (j == 2)
			return 0;
		else
			weightx = 65536 / 2;
	return ((uint64_t)weightx * (uint64_t)weighty) >> 16;
}

// Use the polyphase method to create an RGB(A)8 mipmap level from the previous level image (halving the
// dimension, rounding down if necessary). Works for non-power-of-two textures by rounding down.

static void create_mipmap_polyphase(Image *source_image, Image *dest_image) {
	int h = dest_image->height;
	int w = dest_image->width;
	for (int dy = 0; dy < h; dy++)
		for (int dx = 0; dx < w; dx++) {
			// Calculate the average values of the pixel block.
			uint32_t r = 0;
			uint32_t g = 0;
			uint32_t b = 0;
			uint32_t a = 0;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++) {
					uint32_t weight = calculate_polyphase_weight(dx, dy, j, i, w, h,
						source_image->width, source_image->height);
					// Add the pixel(2 * dx + j, 2 * dy + i) from the source image with weight.
					if (weight > 0) {
						uint32_t pixel = source_image->pixels[(2 * dy + i) *
							source_image->extended_width + 2 * dx + j];
						r += weight * pixel_get_r(pixel);
						g += weight * pixel_get_g(pixel);
						b += weight * pixel_get_b(pixel);
						a += weight * pixel_get_a(pixel);
					}
				}
			// Bits 16-23 of r, g, b and a hold the integer value, bits 0-15 is the fraction.
			// Round up if bit 15 is set.
			r = (r >> 16) + ((r & 0x8000) >> 15);
			g = (g >> 16) + ((g & 0x8000) >> 15);
			b = (b >> 16) + ((b & 0x8000) >> 15);
			a = (a >> 16) + ((a & 0x8000) >> 15);
			if (source_image->alpha_bits == 0)
				a = 0xFF;
			else
			if (source_image->alpha_bits == 1)
				// Avoid smoothing out 1-bit alpha textures.
				if (a >= 0x80)
					a = 0xFF;
				else
					a = 0;
			unsigned int pixel = pack_rgba(r, g, b, a);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
}

// Use the polyphase method to create a half-float mipmap level from the previous level image (halving the
// dimension, rounding down if necessary). Works for non-power-of-two textures by rounding down.

static void create_half_float_mipmap_polyphase(Image *source_image, Image *dest_image) {
	int h = dest_image->height;
	int w = dest_image->width;
	for (int dy = 0; dy < h; dy++)
		for (int dx = 0; dx < w; dx++) {
			// Calculate the average values of the pixel block.
			float c[4];
			c[0] = c[1] = c[2] = c[3] = 0;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++) {
					uint32_t weight = calculate_polyphase_weight(dx, dy, j, i, w, h,
						source_image->width, source_image->height);
					// Add the pixel(2 * dx + j, 2 * dy + i) from the source image with weight.
					if (weight > 0) {
						uint64_t pixel = *(uint64_t *)&source_image->pixels[((2 * dy + i) *
							source_image->extended_width + 2 * dx + j) * 2];
						uint16_t h[4];
						h[0] = pixel64_get_r16(pixel);
						h[1] = pixel64_get_g16(pixel);
						h[2] = pixel64_get_b16(pixel);
						h[3] = pixel64_get_a16(pixel);
						float f[4];
						halfp2singles(&f[0], &h[0], 4);
						c[0] += (float)weight / 65536.0 * f[0];
						c[1] += (float)weight / 65536.0 * f[1];
						c[2] += (float)weight / 65536.0 * f[2];
						c[3] += (float)weight / 65536.0 * f[3];
					}
				}
			if (source_image->alpha_bits == 0) {
				c[3] = 1.0;
			}
			else
			if (source_image->alpha_bits == 1)
				// Avoid smoothing out 1-bit alpha textures. If there are less than two
				// alpha pixels in the source image with alpha 0xFF, then alpha becomes
				// zero, otherwise 0xFF.
				if (c[3] >= 0.5)
					c[3] = 1.0;
				else
					c[3] = 0;
			uint16_t h[4];
			singles2halfp(&h[0], &c[0], 4);
			uint64_t pixel = pack_rgba16(h[0], h[1], h[2], h[3]);
			*(uint64_t *)&dest_image->pixels[(dy * dest_image->extended_width + dx) * 2] = pixel;
		}
}

// Use the polyphase method to create a 16-bit mipmap level from the previous level image (halving the
// dimension, rounding down if necessary). Works for non-power-of-two textures by rounding down.

static void create_16_bit_mipmap_polyphase(Image *source_image, Image *dest_image) {
	int h = dest_image->height;
	int w = dest_image->width;
	for (int dy = 0; dy < h; dy++)
		for (int dx = 0; dx < w; dx++) {
			// Calculate the average values of the pixel block.
			uint32_t r = 0;
			uint32_t g = 0;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++) {
					uint32_t weight = calculate_polyphase_weight(dx, dy, j, i, w, h,
						source_image->width, source_image->height);
					// Add the pixel(2 * dx + j, 2 * dy + i) from the source image with weight.
					if (weight > 0) {
						uint32_t pixel = source_image->pixels[(2 * dy + i) *
							source_image->extended_width + 2 * dx + j];
						r += weight * pixel_get_r16(pixel);
						g += weight * pixel_get_g16(pixel);
					}
				}
			// Bits 16-31 of r, g, b and a hold the integer value, bits 0-15 is the fraction.
			// Round up if bit 15 is set.
			r = (r >> 16) + ((r & 0x8000) >> 15);
			g = (g >> 16) + ((g & 0x8000) >> 15);
			uint32_t pixel = pack_r16(r) | pack_g16(g);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
}

// Use the polyphase method to create a signed 16-bit mipmap level from the previous level image (halving the
// dimension, rounding down if necessary). Works for non-power-of-two textures by rounding down.

static void create_signed_16_bit_mipmap_polyphase(Image *source_image, Image *dest_image) {
	int h = dest_image->height;
	int w = dest_image->width;
	for (int dy = 0; dy < h; dy++)
		for (int dx = 0; dx < w; dx++) {
			// Calculate the average values of the pixel block.
			int32_t r = 0;
			int32_t g = 0;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++) {
					uint32_t weight = calculate_polyphase_weight(dx, dy, j, i, w, h,
						source_image->width, source_image->height);
					// Add the pixel(2 * dx + j, 2 * dy + i) from the source image with weight.
					if (weight > 0) {
						uint32_t pixel = source_image->pixels[(2 * dy + i) *
							source_image->extended_width + 2 * dx + j];
						// weight is in the range [0, 65536].
						// pixel components are the in range [-32768, 32767].
						// So no overflow should occur.
						r += (int32_t)weight * pixel_get_signed_r16(pixel);
						g += (int32_t)weight * pixel_get_signed_g16(pixel);
					}
				}
			// Bits 16-31 of r, g, b and a hold the integer value, bits 0-15 is the fraction.
			// Round up if bit 15 is set.
			r = (r >> 16) + ((r & 0x8000) >> 15);
			g = (g >> 16) + ((g & 0x8000) >> 15);
			uint32_t pixel = pack_r16((uint16_t)(int16_t)r) | pack_g16((uint16_t)(int16_t)g);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
}

// Use the polyphase method to create a signed 8-bit mipmap level from the previous level image (halving the
// dimension, rounding down if necessary). Works for non-power-of-two textures by rounding down.

static void create_signed_8_bit_mipmap_polyphase(Image *source_image, Image *dest_image) {
	int h = dest_image->height;
	int w = dest_image->width;
	for (int dy = 0; dy < h; dy++)
		for (int dx = 0; dx < w; dx++) {
			// Calculate the average values of the pixel block.
			int32_t r = 0;
			int32_t g = 0;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++) {
					uint32_t weight = calculate_polyphase_weight(dx, dy, j, i, w, h,
						source_image->width, source_image->height);
					// Add the pixel(2 * dx + j, 2 * dy + i) from the source image with weight.
					if (weight > 0) {
						uint32_t pixel = source_image->pixels[(2 * dy + i) *
							source_image->extended_width + 2 * dx + j];
						// weight is in the range [0, 65536].
						// pixel components are the in range [-128, 127].
						// So no overflow should occur.
						r += (int32_t)weight * pixel_get_signed_r8(pixel);
						g += (int32_t)weight * pixel_get_signed_g8(pixel);
					}
				}
			// Bits 16-23 of r and g hold the integer value, bits 0-15 is the fraction.
			// Round up if bit 15 is set.
			r = (r >> 16) + ((r & 0x8000) >> 15);
			g = (g >> 16) + ((g & 0x8000) >> 15);
			uint32_t pixel = pack_r((uint8_t)(int8_t)r) | pack_g((uint8_t)(int8_t)g);
			dest_image->pixels[dy * dest_image->extended_width + dx] = pixel;
		}
}

// Generate a scaled down mipmap image according to the given divider.
// divider must be a power of two greater or equal to two.

static void generate_mipmap_level(Image *source_image, int divider, Image *dest_image) {
	// Always round down the new dimensions.
	dest_image->width = source_image->width / divider;
	dest_image->height = source_image->height / divider;
	dest_image->extended_width = dest_image->width;
	dest_image->extended_height = dest_image->height;
	dest_image->alpha_bits = source_image->alpha_bits;
	dest_image->nu_components = source_image->nu_components;
	dest_image->bits_per_component = source_image->bits_per_component;
	dest_image->srgb = 0;
	dest_image->is_signed = source_image->is_signed;
	dest_image->is_half_float = source_image->is_half_float;
	dest_image->pixels = (unsigned int *)malloc(dest_image->extended_width * dest_image->extended_height * 4);
	// Check whether the width and the height are a power of two.
	int count = 0;
	for (int i = 0; i < 30; i++) {
		if (source_image->width == (1 << i))
			count++;
		if (source_image->height == (1 << i))
			count++;
	}
	if (count != 2) {
		if (divider != 2) {
			printf("Error -- non-power-of-two mipmap must be generated from previous level.\n");
			exit(1);
		}
		if (source_image->is_half_float) {
			dest_image->pixels = (unsigned int *)realloc(dest_image->pixels,
				dest_image->extended_width * dest_image->extended_height * 8);
			create_half_float_mipmap_polyphase(source_image, dest_image);
		}
		else
		if (source_image->bits_per_component == 16 && !source_image->is_signed) {
			create_16_bit_mipmap_polyphase(source_image, dest_image);
		}
		else
		if (source_image->bits_per_component == 16 && source_image->is_signed) {
			create_signed_16_bit_mipmap_polyphase(source_image, dest_image);
		}
		else
		if (source_image->bits_per_component == 8 && source_image->is_signed) {
			create_signed_8_bit_mipmap_polyphase(source_image, dest_image);
		}
		else
			create_mipmap_polyphase(source_image, dest_image);
	}
	else
		if (divider == 2)
			if (source_image->is_half_float) {
				dest_image->pixels = (unsigned int *)realloc(dest_image->pixels,
					dest_image->extended_width * dest_image->extended_height * 8);
				create_half_float_mipmap_with_averaging_divider_2(source_image, dest_image);
			}
			else
			if (source_image->bits_per_component == 16 && !source_image->is_signed) {
				create_16_bit_mipmap_with_averaging_divider_2(source_image, dest_image);
			}
			else
			if (source_image->bits_per_component == 16 && source_image->is_signed) {
				create_signed_16_bit_mipmap_with_averaging_divider_2(source_image, dest_image);
			}
			else
			if (source_image->bits_per_component == 8 && source_image->is_signed) {
				create_signed_8_bit_mipmap_with_averaging_divider_2(source_image, dest_image);
			}
			else
				create_mipmap_with_averaging_divider_2(source_image, dest_image);
		else {
			if (source_image->is_half_float) {
				printf("Error -- cannot generate mipmaps for image with half-float components.");
				exit(1);
			}
			if (source_image->bits_per_component == 16 || source_image->is_signed) {
				printf("Error -- cannot generate mipmaps for image with 16-bit or signed components.\n");
				exit(1);
			}
			create_mipmap_with_averaging(source_image, divider, dest_image);
		}
}

void generate_mipmap_level_from_original(Image *source_image, int level, Image *dest_image) {
	int divider = 1 << level;
	generate_mipmap_level(source_image, divider, dest_image);
}

void generate_mipmap_level_from_previous_level(Image *source_image, Image *dest_image) {
	generate_mipmap_level(source_image, 2, dest_image);
}

int count_mipmap_levels(Image *image) {
	int i = 1;
	int divider = 2;
	for (;;) {
		int width = image->width / divider;
		int height = image->height / divider;
		if (width < 1 || height < 1)
			return i;
		i++;
		divider *= 2;
	}
}


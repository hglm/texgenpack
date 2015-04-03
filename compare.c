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
#include <math.h>
#include <float.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

#if defined(_WIN32) && !defined(__GNUC__)
#define isnan(x) _isnan(x)
#endif

static double compare_border_block_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data);
static double compare_border_block_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data);
static double compare_border_block_perceptive_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data);
static double compare_border_block_perceptive_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data);

// Compare block image with source image with regular RGBA encoded 32-bit pixels, any block size.

double compare_block_any_size_rgba(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + user_data->texture->block_width > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = user_data->texture->block_width;
	int h;
	if (user_data->y_offset + user_data->texture->block_height > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = user_data->texture->block_height;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			if (user_data->texture->type & TEXTURE_TYPE_ALPHA_BIT) {
				int a1 = pixel_get_a(pixel1);
				int a2 = pixel_get_a(pixel2);
				// When both alpha values are zero, the RGB values don't matter
				// for a correct result.
				if ((a1 | a2) == 0)
					continue;
				error += (a1 - a2) * (a1 - a2);
			}
			int r1 = pixel_get_r(pixel1);
			int g1 = pixel_get_g(pixel1);
			int b1 = pixel_get_b(pixel1);
			int r2 = pixel_get_r(pixel2);
			int g2 = pixel_get_g(pixel2);
			int b2 = pixel_get_b(pixel2);
			error += (r1 - r2) * (r1 - r2);
			error += (g1 - g2) * (g1 - g2);
			error += (b1 - b2) * (b1 - b2);
		}
		pix1 = &pix1[user_data->texture->block_width];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}


// Define a few macros to speed up image comparison for 8-bit RGB(A) formats.

#define compare_pixel_xy(valx, valy) \
	{ \
	int x = valx; \
	int y = valy; \
	unsigned int pixel1 = pix1[y * 4 + x]; \
	int r1 = pixel_get_r(pixel1); \
	int g1 = pixel_get_g(pixel1); \
	int b1 = pixel_get_b(pixel1); \
	unsigned int pixel2 = pix2[y * (user_data->image_rowstride / 4) + x]; \
	int r2 = pixel_get_r(pixel2); \
	int g2 = pixel_get_g(pixel2); \
	int b2 = pixel_get_b(pixel2); \
	error += (r1 - r2) * (r1 - r2); \
	error += (g1 - g2) * (g1 - g2); \
	error += (b1 - b2) * (b1 - b2); \
	}

#define compare_pixel_xy_with_alpha(valx, valy) \
	{ \
	int x = valx; \
	int y = valy; \
	unsigned int pixel1 = pix1[y * 4 + x]; \
	unsigned int pixel2 = pix2[y * (user_data->image_rowstride / 4) + x]; \
	int a1 = pixel_get_a(pixel1); \
	int a2 = pixel_get_a(pixel2); \
	if ((a1 | a2) != 0) { \
		error += (a1 - a2) * (a1 - a2); \
		int r1 = pixel_get_r(pixel1); \
		int g1 = pixel_get_g(pixel1); \
		int b1 = pixel_get_b(pixel1); \
		int r2 = pixel_get_r(pixel2); \
		int g2 = pixel_get_g(pixel2); \
		int b2 = pixel_get_b(pixel2); \
		error += (r1 - r2) * (r1 - r2); \
		error += (g1 - g2) * (g1 - g2); \
		error += (b1 - b2) * (b1 - b2); \
	} \
	}

#define get_component_differences_xy(valx, valy) \
	{ \
	int x = valx; \
	int y = valy; \
	unsigned int pixel1 = pix1[y * 4 + x]; \
	int r1 = pixel_get_r(pixel1); \
	int g1 = pixel_get_g(pixel1); \
	int b1 = pixel_get_b(pixel1); \
	unsigned int pixel2 = pix2[y * (user_data->image_rowstride / 4) + x]; \
	int r2 = pixel_get_r(pixel2); \
	int g2 = pixel_get_g(pixel2); \
	int b2 = pixel_get_b(pixel2); \
	red_diff[y][x] = r1 - r2; \
	green_diff[y][x] = g1 - g2; \
	blue_diff[y][x] = b1 - b2; \
	}

#define get_component_differences_with_alpha_xy(valx, valy) \
	{ \
	int x = valx; \
	int y = valy; \
	unsigned int pixel1 = pix1[y * 4 + x]; \
	unsigned int pixel2 = pix2[y * (user_data->image_rowstride / 4) + x]; \
	int a1 = pixel_get_a(pixel1); \
	int a2 = pixel_get_a(pixel2); \
	alpha_diff[y][x] = a1 - a2; \
	if ((a1 | a2) == 0) { \
		red_diff[y][x] = 0; \
		green_diff[y][x] = 0; \
		blue_diff[y][x] = 0; \
	} \
	else { \
		int r1 = pixel_get_r(pixel1); \
		int g1 = pixel_get_g(pixel1); \
		int b1 = pixel_get_b(pixel1); \
		int r2 = pixel_get_r(pixel2); \
		int g2 = pixel_get_g(pixel2); \
		int b2 = pixel_get_b(pixel2); \
		red_diff[y][x] = r1 - r2; \
		green_diff[y][x] = g1 - g2; \
		blue_diff[y][x] = b1 - b2; \
	} \
	}

#define get_component_differences_above_x(valx) \
	{ \
	int x = valx; \
	unsigned int pixel1 = pix1_above[3 * 4 + x]; \
	int r1 = pixel_get_r(pixel1); \
	int g1 = pixel_get_g(pixel1); \
	int b1 = pixel_get_b(pixel1); \
	unsigned int pixel2 = pix2[- (user_data->image_rowstride / 4) + x]; \
	int r2 = pixel_get_r(pixel2); \
	int g2 = pixel_get_g(pixel2); \
	int b2 = pixel_get_b(pixel2); \
	red_diff_above = r1 - r2; \
	green_diff_above = g1 - g2; \
	blue_diff_above = b1 - b2; \
	}

#define get_component_differences_with_alpha_above_x(valx) \
	{ \
	int x = valx; \
	unsigned int pixel1 = pix1_above[3 * 4 + x]; \
	unsigned int pixel2 = pix2[- (user_data->image_rowstride / 4) + x]; \
	int a1 = pixel_get_a(pixel1); \
	int a2 = pixel_get_a(pixel2); \
	alpha_diff_above = a1 - a2; \
	if ((a1 | a2) == 0) { \
		red_diff_above = 0; \
		green_diff_above = 0; \
		blue_diff_above = 0; \
	} \
	else { \
		int r1 = pixel_get_r(pixel1); \
		int g1 = pixel_get_g(pixel1); \
		int b1 = pixel_get_b(pixel1); \
		int r2 = pixel_get_r(pixel2); \
		int g2 = pixel_get_g(pixel2); \
		int b2 = pixel_get_b(pixel2); \
		red_diff_above = r1 - r2; \
		green_diff_above = g1 - g2; \
		blue_diff_above = b1 - b2; \
	} \
	}

#define get_component_differences_left_y(valy) \
	{ \
	int y = valy; \
	unsigned int pixel1 = pix1_left[y * 4 + 3]; \
	int r1 = pixel_get_r(pixel1); \
	int g1 = pixel_get_g(pixel1); \
	int b1 = pixel_get_b(pixel1); \
	unsigned int pixel2 = pix2[y * (user_data->image_rowstride / 4) - 1]; \
	int r2 = pixel_get_r(pixel2); \
	int g2 = pixel_get_g(pixel2); \
	int b2 = pixel_get_b(pixel2); \
	red_diff_left = r1 - r2; \
	green_diff_left = g1 - g2; \
	blue_diff_left = b1 - b2; \
	}

#define get_component_differences_with_alpha_left_y(valy) \
	{ \
	int y = valy; \
	unsigned int pixel1 = pix1_left[y * 4 + 3]; \
	unsigned int pixel2 = pix2[y * (user_data->image_rowstride / 4) - 1]; \
	int a1 = pixel_get_a(pixel1); \
	int a2 = pixel_get_a(pixel2); \
	alpha_diff_left = a1 - a2; \
	if ((a1 | a2) == 0) { \
		red_diff_left = 0; \
		green_diff_left = 0; \
		blue_diff_left = 0; \
	} \
	else { \
		int r1 = pixel_get_r(pixel1); \
		int g1 = pixel_get_g(pixel1); \
		int b1 = pixel_get_b(pixel1); \
		int r2 = pixel_get_r(pixel2); \
		int g2 = pixel_get_g(pixel2); \
		int b2 = pixel_get_b(pixel2); \
		red_diff_left = r1 - r2; \
		green_diff_left = g1 - g2; \
		blue_diff_left = b1 - b2; \
	} \
	}

#define compare_adjacent_pixels(x1, y1, x2, y2) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff[y2][x2]); \
	int dgreen = (green_diff[y1][x1] - green_diff[y2][x2]); \
	int dblue = (blue_diff[y1][x1] - blue_diff[y2][x2]); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue) / 4; \
	}

#define compare_adjacent_pixels_with_alpha(x1, y1, x2, y2) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff[y2][x2]); \
	int dgreen = (green_diff[y1][x1] - green_diff[y2][x2]); \
	int dblue = (blue_diff[y1][x1] - blue_diff[y2][x2]); \
	int dalpha = (alpha_diff[y1][x1] - alpha_diff[y2][x2]); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue + dalpha * dalpha) / 4; \
	}

#define compare_adjacent_pixels_check(x1, y1, x2, y2) \
	if (x1 < w && y1 < h && x2 < w && y2 < h) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff[y2][x2]); \
	int dgreen = (green_diff[y1][x1] - green_diff[y2][x2]); \
	int dblue = (blue_diff[y1][x1] - blue_diff[y2][x2]); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue) / 4; \
	}

#define compare_adjacent_pixels_with_alpha_check(x1, y1, x2, y2) \
	if (x1 < w && y1 < h && x2 < w && y2 < h) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff[y2][x2]); \
	int dgreen = (green_diff[y1][x1] - green_diff[y2][x2]); \
	int dblue = (blue_diff[y1][x1] - blue_diff[y2][x2]); \
	int dalpha = (alpha_diff[y1][x1] - alpha_diff[y2][x2]); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue + dalpha * dalpha) / 4; \
	}

#define compare_adjacent_pixels_block_above(x1, y1, x2, y2) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff_above); \
	int dgreen = (green_diff[y1][x1] - green_diff_above); \
	int dblue = (blue_diff[y1][x1] - blue_diff_above); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue) / 4; \
	}

#define compare_adjacent_pixels_with_alpha_block_above(x1, y1, x2, y2) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff_above); \
	int dgreen = (green_diff[y1][x1] - green_diff_above); \
	int dblue = (blue_diff[y1][x1] - blue_diff_above); \
	int dalpha = (alpha_diff[y1][x1] - alpha_diff_above); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue + dalpha * dalpha) / 4; \
	}

#define compare_adjacent_pixels_block_left(x1, y1, x2, y2) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff_left); \
	int dgreen = (green_diff[y1][x1] - green_diff_left); \
	int dblue = (blue_diff[y1][x1] - blue_diff_left); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue) / 4; \
	}

#define compare_adjacent_pixels_with_alpha_block_left(x1, y1, x2, y2) \
	{ \
	int dred = (red_diff[y1][x1] - red_diff_left); \
	int dgreen = (green_diff[y1][x1] - green_diff_left); \
	int dblue = (blue_diff[y1][x1] - blue_diff_left); \
	int dalpha = (alpha_diff[y1][x1] - alpha_diff_left); \
	error += (dred * dred + dgreen * dgreen + dblue * dblue + dalpha * dalpha) / 4; \
	}

// Compare block image with source image with regular RGB encoded into each 32-bit pixel, block size 4x4.

double compare_block_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data) {
	// When on the borders, use the unoptimized version.
	if ((user_data->x_offset + 4 > user_data->texture->width) ||
	(user_data->y_offset + 4 > user_data->texture->height))
		return compare_border_block_4x4_rgb(image_buffer, user_data);
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	// Optimized version.
	int error = 0;
	compare_pixel_xy(0, 0);
	compare_pixel_xy(1, 0);
	compare_pixel_xy(2, 0);
	compare_pixel_xy(3, 0);
	compare_pixel_xy(0, 1);
	compare_pixel_xy(1, 1);
	compare_pixel_xy(2, 1);
	compare_pixel_xy(3, 1);
	compare_pixel_xy(0, 2);
	compare_pixel_xy(1, 2);
	compare_pixel_xy(2, 2);
	compare_pixel_xy(3, 2);
	compare_pixel_xy(0, 3);
	compare_pixel_xy(1, 3);
	compare_pixel_xy(2, 3);
	compare_pixel_xy(3, 3);
	return (double)1 / error;
}

double compare_block_perceptive_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data) {
	// When on the borders, use the unoptimized version.
	if ((user_data->x_offset + 4 > user_data->texture->width) ||
	(user_data->y_offset + 4 > user_data->texture->height))
		return compare_border_block_perceptive_4x4_rgb(image_buffer, user_data);
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int red_diff[4][4], green_diff[4][4], blue_diff[4][4];
	get_component_differences_xy(0, 0);
	get_component_differences_xy(1, 0);
	get_component_differences_xy(2, 0);
	get_component_differences_xy(3, 0);
	get_component_differences_xy(0, 1);
	get_component_differences_xy(1, 1);
	get_component_differences_xy(2, 1);
	get_component_differences_xy(3, 1);
	get_component_differences_xy(0, 2);
	get_component_differences_xy(1, 2);
	get_component_differences_xy(2, 2);
	get_component_differences_xy(3, 2);
	get_component_differences_xy(0, 3);
	get_component_differences_xy(1, 3);
	get_component_differences_xy(2, 3);
	get_component_differences_xy(3, 3);
	int error = 0;
	// Add regular error.
	for (int y = 0; y < 4; y ++)
		for (int x = 0; x < 4; x++) {
			error += red_diff[y][x] * red_diff[y][x];
			error += green_diff[y][x] * green_diff[y][x];
			error += blue_diff[y][x] * blue_diff[y][x];
		}
	// For pixels below the top row and to the right of the left column, add errors based on
	// differences between adjacent pixels.
	// pixel (1, 1)
	compare_adjacent_pixels(1, 1, 1, 0);	// above
	compare_adjacent_pixels(1, 1, 0, 1);	// left
	compare_adjacent_pixels(1, 1, 1, 2);	// below
	compare_adjacent_pixels(1, 1, 2, 1);	// right
	// pixel (2, 1)
	compare_adjacent_pixels(2, 1, 2, 0);	// above
	compare_adjacent_pixels(2, 1, 2, 2);	// below
	compare_adjacent_pixels(2, 1, 3, 1);	// right
	// pixel (3, 1)
	compare_adjacent_pixels(3, 1, 3, 0);	// above
	compare_adjacent_pixels(3, 1, 3, 2);	// below
	// pixel (1, 2)
	compare_adjacent_pixels(1, 2, 0, 2);	// left
	compare_adjacent_pixels(1, 2, 1, 3);	// below
	compare_adjacent_pixels(1, 2, 2, 2);	// right
	// pixel (2, 2)
	compare_adjacent_pixels(2, 2, 2, 3);	// below
	compare_adjacent_pixels(2, 2, 3, 2);	// right
	// pixel (3, 2)
	compare_adjacent_pixels(3, 2, 3, 3);	// below
	// pixel (1, 3)
	compare_adjacent_pixels(1, 3, 0, 3);	// left
	compare_adjacent_pixels(1, 3, 2, 3);	// right
	// pixel (2, 3)
	compare_adjacent_pixels(2, 3, 3, 3);	// right

	// Add errors based on differences between adjacent pixels for top row pixels, comparing with the
	// block above.
	if (user_data->texture_pixels_above != NULL) {
		int red_diff_above, green_diff_above, blue_diff_above;
		unsigned int *pix1_above = user_data->texture_pixels_above;
		get_component_differences_above_x(0);
		compare_adjacent_pixels_block_above(0, 0, 0, 3);
		get_component_differences_above_x(1);
		compare_adjacent_pixels_block_above(1, 0, 1, 3);
		get_component_differences_above_x(2);
		compare_adjacent_pixels_block_above(2, 0, 2, 3);
		get_component_differences_above_x(3);
		compare_adjacent_pixels_block_above(3, 0, 3, 3);
	}
	// Add errors based on differences between adjacent pixels for left column pixels, comparing with the
	// block to the left.
	if (user_data->texture_pixels_left != NULL) {
		int red_diff_left, green_diff_left, blue_diff_left;
		unsigned int *pix1_left = user_data->texture_pixels_left;
		get_component_differences_left_y(0);
		compare_adjacent_pixels_block_left(0, 0, 3, 0);
		get_component_differences_left_y(1);
		compare_adjacent_pixels_block_left(0, 1, 3, 1);
		get_component_differences_left_y(2);
		compare_adjacent_pixels_block_left(0, 2, 3, 2);
		get_component_differences_left_y(3);
		compare_adjacent_pixels_block_left(0, 3, 3, 3);
	}
	return (double)1 / error;
}

// Compare block image with source image with regular RGBA encoded into each 32-bit pixel, block size 4x4.

double compare_block_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data) {
	// When on the borders, use the unoptimized version.
	if ((user_data->x_offset + 4 > user_data->texture->width) ||
	(user_data->y_offset + 4 > user_data->texture->height))
		return compare_border_block_4x4_rgba(image_buffer, user_data);
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	// Optimized version.
	int error = 0;
	compare_pixel_xy_with_alpha(0, 0);
	compare_pixel_xy_with_alpha(1, 0);
	compare_pixel_xy_with_alpha(2, 0);
	compare_pixel_xy_with_alpha(3, 0);
	compare_pixel_xy_with_alpha(0, 1);
	compare_pixel_xy_with_alpha(1, 1);
	compare_pixel_xy_with_alpha(2, 1);
	compare_pixel_xy_with_alpha(3, 1);
	compare_pixel_xy_with_alpha(0, 2);
	compare_pixel_xy_with_alpha(1, 2);
	compare_pixel_xy_with_alpha(2, 2);
	compare_pixel_xy_with_alpha(3, 2);
	compare_pixel_xy_with_alpha(0, 3);
	compare_pixel_xy_with_alpha(1, 3);
	compare_pixel_xy_with_alpha(2, 3);
	compare_pixel_xy_with_alpha(3, 3);
	return (double)1 / error;
}

double compare_block_perceptive_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data) {
	// When on the borders, use the unoptimized version.
	if ((user_data->x_offset + 4 > user_data->texture->width) ||
	(user_data->y_offset + 4 > user_data->texture->height))
		return compare_border_block_perceptive_4x4_rgba(image_buffer, user_data);
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int red_diff[4][4], green_diff[4][4], blue_diff[4][4], alpha_diff[4][4];
	get_component_differences_with_alpha_xy(0, 0);
	get_component_differences_with_alpha_xy(1, 0);
	get_component_differences_with_alpha_xy(2, 0);
	get_component_differences_with_alpha_xy(3, 0);
	get_component_differences_with_alpha_xy(0, 1);
	get_component_differences_with_alpha_xy(1, 1);
	get_component_differences_with_alpha_xy(2, 1);
	get_component_differences_with_alpha_xy(3, 1);
	get_component_differences_with_alpha_xy(0, 2);
	get_component_differences_with_alpha_xy(1, 2);
	get_component_differences_with_alpha_xy(2, 2);
	get_component_differences_with_alpha_xy(3, 2);
	get_component_differences_with_alpha_xy(0, 3);
	get_component_differences_with_alpha_xy(1, 3);
	get_component_differences_with_alpha_xy(2, 3);
	get_component_differences_with_alpha_xy(3, 3);
	int error = 0;
	// Add regular error.
	for (int y = 0; y < 4; y ++)
		for (int x = 0; x < 4; x++) {
			error += red_diff[y][x] * red_diff[y][x];
			error += green_diff[y][x] * green_diff[y][x];
			error += blue_diff[y][x] * blue_diff[y][x];
			error += alpha_diff[y][x] * alpha_diff[y][x];
		}
	// For pixels below the top row and to the right of the left column, add errors based on
	// differences between adjacent pixels.
	// pixel (1, 1)
	compare_adjacent_pixels_with_alpha(1, 1, 1, 0);	// above
	compare_adjacent_pixels_with_alpha(1, 1, 0, 1);	// left
	compare_adjacent_pixels_with_alpha(1, 1, 1, 2);	// below
	compare_adjacent_pixels_with_alpha(1, 1, 2, 1);	// right
	// pixel (2, 1)
	compare_adjacent_pixels_with_alpha(2, 1, 2, 0);	// above
	compare_adjacent_pixels_with_alpha(2, 1, 2, 2);	// below
	compare_adjacent_pixels_with_alpha(2, 1, 3, 1);	// right
	// pixel (3, 1)
	compare_adjacent_pixels_with_alpha(3, 1, 3, 0);	// above
	compare_adjacent_pixels_with_alpha(3, 1, 3, 2);	// below
	// pixel (1, 2)
	compare_adjacent_pixels_with_alpha(1, 2, 0, 2);	// left
	compare_adjacent_pixels_with_alpha(1, 2, 1, 3);	// below
	compare_adjacent_pixels_with_alpha(1, 2, 2, 2);	// right
	// pixel (2, 2)
	compare_adjacent_pixels_with_alpha(2, 2, 2, 3);	// below
	compare_adjacent_pixels_with_alpha(2, 2, 3, 2);	// right
	// pixel (3, 2)
	compare_adjacent_pixels_with_alpha(3, 2, 3, 3);	// below
	// pixel (1, 3)
	compare_adjacent_pixels_with_alpha(1, 3, 0, 3);	// left
	compare_adjacent_pixels_with_alpha(1, 3, 2, 3);	// right
	// pixel (2, 3)
	compare_adjacent_pixels_with_alpha(2, 3, 3, 3);	// right

	// Add errors based on differences between adjacent pixels for top row pixels, comparing with the
	// block above.
	if (user_data->texture_pixels_above != NULL) {
		int red_diff_above, green_diff_above, blue_diff_above, alpha_diff_above;
		unsigned int *pix1_above = user_data->texture_pixels_above;
		get_component_differences_with_alpha_above_x(0);
		compare_adjacent_pixels_with_alpha_block_above(0, 0, 0, 3);
		get_component_differences_with_alpha_above_x(1);
		compare_adjacent_pixels_with_alpha_block_above(1, 0, 1, 3);
		get_component_differences_with_alpha_above_x(2);
		compare_adjacent_pixels_with_alpha_block_above(2, 0, 2, 3);
		get_component_differences_with_alpha_above_x(3);
		compare_adjacent_pixels_with_alpha_block_above(3, 0, 3, 3);
	}
	// Add errors based on differences between adjacent pixels for left column pixels, comparing with the
	// block to the left.
	if (user_data->texture_pixels_left != NULL) {
		int red_diff_left, green_diff_left, blue_diff_left, alpha_diff_left;
		unsigned int *pix1_left = user_data->texture_pixels_left;
		get_component_differences_with_alpha_left_y(0);
		compare_adjacent_pixels_with_alpha_block_left(0, 0, 3, 0);
		get_component_differences_with_alpha_left_y(1);
		compare_adjacent_pixels_with_alpha_block_left(0, 1, 3, 1);
		get_component_differences_with_alpha_left_y(2);
		compare_adjacent_pixels_with_alpha_block_left(0, 2, 3, 2);
		get_component_differences_with_alpha_left_y(3);
		compare_adjacent_pixels_with_alpha_block_left(0, 3, 3, 3);
	}
	return (double)1 / error;
}

// Compare border block image with source image with regular RGB encoded into each 32-bit pixel, block size 4x4.

double compare_border_block_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r1 = pixel_get_r(pixel1);
			int g1 = pixel_get_g(pixel1);
			int b1 = pixel_get_b(pixel1);
			int r2 = pixel_get_r(pixel2);
			int g2 = pixel_get_g(pixel2);
			int b2 = pixel_get_b(pixel2);
			error += (r1 - r2) * (r1 - r2);
			error += (g1 - g2) * (g1 - g2);
			error += (b1 - b2) * (b1 - b2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

double compare_border_block_perceptive_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int red_diff[4][4], green_diff[4][4], blue_diff[4][4];
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r1 = pixel_get_r(pixel1);
			int g1 = pixel_get_g(pixel1);
			int b1 = pixel_get_b(pixel1);
			int r2 = pixel_get_r(pixel2);
			int g2 = pixel_get_g(pixel2);
			int b2 = pixel_get_b(pixel2);
			red_diff[y][x] = (r1 - r2);
			green_diff[y][x] = (g1 - g2);
			blue_diff[y][x] = (b1 - b2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	int error = 0;
	// Add regular error.
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			error += red_diff[y][x] * red_diff[y][x];
			error += green_diff[y][x] * green_diff[y][x];
			error += blue_diff[y][x] * blue_diff[y][x];
		}
	// For pixels below the top row and to the right of the left column, add errors based on
	// differences between adjacent pixels.
	// pixel (1, 1)
	compare_adjacent_pixels_check(1, 1, 1, 0);	// above
	compare_adjacent_pixels_check(1, 1, 0, 1);	// left
	compare_adjacent_pixels_check(1, 1, 1, 2);	// below
	compare_adjacent_pixels_check(1, 1, 2, 1);	// right
	// pixel (2, 1)
	compare_adjacent_pixels_check(2, 1, 2, 0);	// above
	compare_adjacent_pixels_check(2, 1, 2, 2);	// below
	compare_adjacent_pixels_check(2, 1, 3, 1);	// right
	// pixel (3, 1)
	compare_adjacent_pixels_check(3, 1, 3, 0);	// above
	compare_adjacent_pixels_check(3, 1, 3, 2);	// below
	// pixel (1, 2)
	compare_adjacent_pixels_check(1, 2, 0, 2);	// left
	compare_adjacent_pixels_check(1, 2, 1, 3);	// below
	compare_adjacent_pixels_check(1, 2, 2, 2);	// right
	// pixel (2, 2)
	compare_adjacent_pixels_check(2, 2, 2, 3);	// below
	compare_adjacent_pixels_check(2, 2, 3, 2);	// right
	// pixel (3, 2)
	compare_adjacent_pixels_check(3, 2, 3, 3);	// below
	// pixel (1, 3)
	compare_adjacent_pixels_check(1, 3, 0, 3);	// left
	compare_adjacent_pixels_check(1, 3, 2, 3);	// right
	// pixel (2, 3)
	compare_adjacent_pixels_check(2, 3, 3, 3);	// right

	// Add errors based on differences between adjacent pixels for top row pixels, comparing with the
	// block above.
	if (user_data->texture_pixels_above != NULL) {
		int red_diff_above, green_diff_above, blue_diff_above;
		unsigned int *pix1_above = user_data->texture_pixels_above;
		get_component_differences_above_x(0);
		compare_adjacent_pixels_block_above(0, 0, 0, 3);
		if (w > 1) {
			get_component_differences_above_x(1);
			compare_adjacent_pixels_block_above(1, 0, 1, 3);
			if (w > 2) {
				get_component_differences_above_x(2);
				compare_adjacent_pixels_block_above(2, 0, 2, 3);
				if (w > 3) {
					get_component_differences_above_x(3);
					compare_adjacent_pixels_block_above(3, 0, 3, 3);
				}
			}
		}
	}
	// Add errors based on differences between adjacent pixels for left column pixels, comparing with the
	// block to the left.
	if (user_data->texture_pixels_left != NULL) {
		int red_diff_left, green_diff_left, blue_diff_left;
		unsigned int *pix1_left = user_data->texture_pixels_left;
		get_component_differences_left_y(0);
		compare_adjacent_pixels_block_left(0, 0, 3, 0);
		if (h > 1) {
			get_component_differences_left_y(1);
			compare_adjacent_pixels_block_left(0, 1, 3, 1);
			if (h > 2) {
				get_component_differences_left_y(2);
				compare_adjacent_pixels_block_left(0, 2, 3, 2);
				if (h > 3) {
					get_component_differences_left_y(3);
					compare_adjacent_pixels_block_left(0, 3, 3, 3);
				}
			}
		}
	}
	return (double)1 / error;
}

// Compare border block image with source image with regular RGBA encoded into each 32-bit pixel, block size 4x4.

double compare_border_block_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			if (user_data->texture->type & TEXTURE_TYPE_ALPHA_BIT) {
				int a1 = pixel_get_a(pixel1);
				int a2 = pixel_get_a(pixel2);
				// When both alpha values are zero, the RGB values don't matter
				// for a correct result.
				if ((a1 | a2) == 0)
					continue;
				error += (a1 - a2) * (a1 - a2);
			}
			int r1 = pixel_get_r(pixel1);
			int g1 = pixel_get_g(pixel1);
			int b1 = pixel_get_b(pixel1);
			int r2 = pixel_get_r(pixel2);
			int g2 = pixel_get_g(pixel2);
			int b2 = pixel_get_b(pixel2);
			error += (r1 - r2) * (r1 - r2);
			error += (g1 - g2) * (g1 - g2);
			error += (b1 - b2) * (b1 - b2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

double compare_border_block_perceptive_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int red_diff[4][4], green_diff[4][4], blue_diff[4][4], alpha_diff[4][4];
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int a1 = pixel_get_a(pixel1);
			int a2 = pixel_get_a(pixel2);
			alpha_diff[y][x] = a1 - a2;
			if ((a1 | a2) == 0) {
				red_diff[y][x] = 0;
				green_diff[y][x] = 0;
				blue_diff[y][x] = 0;
			}
			else {
				int r1 = pixel_get_r(pixel1);
				int g1 = pixel_get_g(pixel1);
				int b1 = pixel_get_b(pixel1);
				int r2 = pixel_get_r(pixel2);
				int g2 = pixel_get_g(pixel2);
				int b2 = pixel_get_b(pixel2);
				red_diff[y][x] = (r1 - r2);
				green_diff[y][x] = (g1 - g2);
				blue_diff[y][x] = (b1 - b2);
			}
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	int error = 0;
	// Add regular error.
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			error += red_diff[y][x] * red_diff[y][x];
			error += green_diff[y][x] * green_diff[y][x];
			error += blue_diff[y][x] * blue_diff[y][x];
			error += alpha_diff[y][x] * alpha_diff[y][x];
		}
	// For pixels below the top row and to the right of the left column, add errors based on
	// differences between adjacent pixels.
	// pixel (1, 1)
	compare_adjacent_pixels_with_alpha_check(1, 1, 1, 0);	// above
	compare_adjacent_pixels_with_alpha_check(1, 1, 0, 1);	// left
	compare_adjacent_pixels_with_alpha_check(1, 1, 1, 2);	// below
	compare_adjacent_pixels_with_alpha_check(1, 1, 2, 1);	// right
	// pixel (2, 1)
	compare_adjacent_pixels_with_alpha_check(2, 1, 2, 0);	// above
	compare_adjacent_pixels_with_alpha_check(2, 1, 2, 2);	// below
	compare_adjacent_pixels_with_alpha_check(2, 1, 3, 1);	// right
	// pixel (3, 1)
	compare_adjacent_pixels_with_alpha_check(3, 1, 3, 0);	// above
	compare_adjacent_pixels_with_alpha_check(3, 1, 3, 2);	// below
	// pixel (1, 2)
	compare_adjacent_pixels_with_alpha_check(1, 2, 0, 2);	// left
	compare_adjacent_pixels_with_alpha_check(1, 2, 1, 3);	// below
	compare_adjacent_pixels_with_alpha_check(1, 2, 2, 2);	// right
	// pixel (2, 2)
	compare_adjacent_pixels_with_alpha_check(2, 2, 2, 3);	// below
	compare_adjacent_pixels_with_alpha_check(2, 2, 3, 2);	// right
	// pixel (3, 2)
	compare_adjacent_pixels_with_alpha_check(3, 2, 3, 3);	// below
	// pixel (1, 3)
	compare_adjacent_pixels_with_alpha_check(1, 3, 0, 3);	// left
	compare_adjacent_pixels_with_alpha_check(1, 3, 2, 3);	// right
	// pixel (2, 3)
	compare_adjacent_pixels_with_alpha_check(2, 3, 3, 3);	// right

	// Add errors based on differences between adjacent pixels for top row pixels, comparing with the
	// block above.
	if (user_data->texture_pixels_above != NULL) {
		int red_diff_above, green_diff_above, blue_diff_above, alpha_diff_above;
		unsigned int *pix1_above = user_data->texture_pixels_above;
		get_component_differences_with_alpha_above_x(0);
		compare_adjacent_pixels_with_alpha_block_above(0, 0, 0, 3);
		if (w > 1) {
			get_component_differences_with_alpha_above_x(1);
			compare_adjacent_pixels_with_alpha_block_above(1, 0, 1, 3);
			if (w > 2) {
				get_component_differences_with_alpha_above_x(2);
				compare_adjacent_pixels_with_alpha_block_above(2, 0, 2, 3);
				if (w > 3) {
					get_component_differences_with_alpha_above_x(3);
					compare_adjacent_pixels_with_alpha_block_above(3, 0, 3, 3);
				}
			}
		}
	}
	// Add errors based on differences between adjacent pixels for left column pixels, comparing with the
	// block to the left.
	if (user_data->texture_pixels_left != NULL) {
		int red_diff_left, green_diff_left, blue_diff_left, alpha_diff_left;
		unsigned int *pix1_left = user_data->texture_pixels_left;
		get_component_differences_with_alpha_left_y(0);
		compare_adjacent_pixels_with_alpha_block_left(0, 0, 3, 0);
		if (h > 1) {
			get_component_differences_with_alpha_left_y(1);
			compare_adjacent_pixels_with_alpha_block_left(0, 1, 3, 1);
			if (h > 2) {
				get_component_differences_with_alpha_left_y(2);
				compare_adjacent_pixels_with_alpha_block_left(0, 2, 3, 2);
				if (h > 3) {
					get_component_differences_with_alpha_left_y(3);
					compare_adjacent_pixels_with_alpha_block_left(0, 3, 3, 3);
				}
			}
		}
	}
	return (double)1 / error;
}

float *normalized_float_table = NULL;

void calculate_normalized_float_table() {
	if (normalized_float_table != NULL)
		return;
	normalized_float_table = (float *)malloc(sizeof(float) * 256);
	for (int i = 0; i < 256; i++) {
		normalized_float_table[i] = (float)i / 255.0;
	}
}

// Compare RGB block image with 32-bit pixels with source image with 64-bit half-float pixels block size 4x4.
// The higher precision of the image to compare to leads to higher quality.

double compare_block_4x4_rgb8_with_half_float(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			uint64_t pixel2 = pix2[x];
			float r1 = normalized_float_table[pixel_get_r(pixel1)];
			float g1 = normalized_float_table[pixel_get_g(pixel1)];
			float b1 = normalized_float_table[pixel_get_b(pixel1)];
			float r2 = half_float_table[pixel64_get_r16(pixel2)];
			float g2 = half_float_table[pixel64_get_g16(pixel2)];
			float b2 = half_float_table[pixel64_get_b16(pixel2)];
			error += (r1 - r2) * (r1 - r2);
			error += (g1 - g2) * (g1 - g2);
			error += (b1 - b2) * (b1 - b2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	return (double)1 / error;
}

// Compare RGBA block image with 32-bit pixels with source image with 64-bit half-float pixels block size 4x4.
// The higher precision of the image to compare to leads to higher quality.

double compare_block_4x4_rgba8_with_half_float(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			uint64_t pixel2 = pix2[x];
			if (user_data->texture->type & TEXTURE_TYPE_ALPHA_BIT) {
				float a1 = normalized_float_table[pixel_get_a(pixel1)];
				float a2 = half_float_table[pixel64_get_a16(pixel2)];
				// When both alpha values are zero, the RGB values don't matter
				// for a correct result.
				if (a1 == 0 && a2 == 0)
					continue;
				error += (a1 - a2) * (a1 - a2);
			}
			float r1 = normalized_float_table[pixel_get_r(pixel1)];
			float g1 = normalized_float_table[pixel_get_g(pixel1)];
			float b1 = normalized_float_table[pixel_get_b(pixel1)];
			float r2 = half_float_table[pixel64_get_r16(pixel2)];
			float g2 = half_float_table[pixel64_get_g16(pixel2)];
			float b2 = half_float_table[pixel64_get_b16(pixel2)];
			error += (r1 - r2) * (r1 - r2);
			error += (g1 - g2) * (g1 - g2);
			error += (b1 - b2) * (b1 - b2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	return (double)1 / error;
}

// Compare image with source image with any number of 8-bit components (no alpha) into each 32-bit pixel, block size 4x4.

double compare_block_4x4_8_bit_components(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int nu_components = user_data->texture->info->nu_components;
	int error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r1 = pixel_get_r(pixel1);
			int r2 = pixel_get_r(pixel2);
			error += (r1 - r2) * (r1 - r2);
			if (nu_components < 2)
				continue;
			int g1 = pixel_get_g(pixel1);
			int g2 = pixel_get_g(pixel2);
			error += (g1 - g2) * (g1 - g2);
			if (nu_components < 3)
				continue;
			int b1 = pixel_get_b(pixel1);
			int b2 = pixel_get_b(pixel2);
			error += (b1 - b2) * (b1 - b2);
			if (nu_components < 4)
				continue;
			int a1 = pixel_get_a(pixel1);
			int a2 = pixel_get_a(pixel2);
			error += (a1 - a2) * (a1 - a2);

		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare image with source image with any number of signed 8-bit components (no alpha) in each 32-bit pixel,
// block size 4x4.

double compare_block_4x4_signed_8_bit_components(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int nu_components = user_data->texture->info->nu_components;
	int error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r1 = pixel_get_signed_r8(pixel1);
			int r2 = pixel_get_signed_r8(pixel2);
			error += (r1 - r2) * (r1 - r2);
			if (nu_components < 2)
				continue;
			int g1 = pixel_get_signed_g8(pixel1);
			int g2 = pixel_get_signed_g8(pixel2);
			error += (g1 - g2) * (g1 - g2);

		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare image with any number of 8-bit components with source image with the same number of 16-bit components,
// block size 4x4. one or two components.

double compare_block_4x4_8_bit_components_with_16_bit(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int nu_components = user_data->texture->info->nu_components;
	uint64_t error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r1 = pixel_get_r(pixel1) * 65535 / 255;
			int r2 = pixel_get_r16(pixel2);
			error += ((int64_t)r1 - r2) * ((int64_t)r1 - r2);
			if (nu_components < 2)
				continue;
			int g1 = pixel_get_g(pixel1) * 65535 / 255;
			int g2 = pixel_get_g16(pixel2);
			error += ((int64_t)g1 - g2) * ((int64_t)g1 - g2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare image with any number of signed 8-bit components with source image with the same number of signed
// 16-bit components block size 4x4. One or two components.

double compare_block_4x4_signed_8_bit_components_with_16_bit(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	int nu_components = user_data->texture->info->nu_components;
	uint64_t error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			// Map r1 from [-128, 127] to [-32768, 32767]
			int r1 = (pixel_get_signed_r8(pixel1) + 128) * 65535 / 255 - 32768;
			int r2 = pixel_get_signed_r16(pixel2);
			error += ((int64_t)r1 - r2) * ((int64_t)r1 - r2);
			if (nu_components < 2)
				continue;
			int g1 = (pixel_get_signed_g8(pixel1) + 128) * 65535 / 255 - 32768;
			int g2 = pixel_get_signed_g16(pixel2);
			error += ((int64_t)g1 - g2) * ((int64_t)g1 - g2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare block image with source image with two 16-bit values encoded into each pixel, block size 4x4.

double compare_block_4x4_rg16(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	uint64_t error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			unsigned int r16_1 = pixel_get_r16(pixel1);
			unsigned int r16_2 = pixel_get_r16(pixel2);
			error += ((int64_t)r16_1 - r16_2) * ((int64_t)r16_1 - r16_2);
			unsigned int g16_1 = pixel_get_g16(pixel1);
			unsigned int g16_2 = pixel_get_g16(pixel2);
			error += ((int64_t)g16_1 - g16_2) * ((int64_t)g16_1 - g16_2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare block image with source image with one 16-bit value encoded into each pixel, block size 4x4.

double compare_block_4x4_r16(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	uint64_t error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r16_1 = pixel_get_r16(pixel1);
			int r16_2 = pixel_get_r16(pixel2);
			error += ((int64_t)r16_1 - r16_2) * ((int64_t)r16_1 - r16_2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare block image with source image with two signed 16-bit values encoded into each pixel.

double compare_block_4x4_rg16_signed(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	uint64_t error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r16_1 = pixel_get_signed_r16(pixel1);
			int r16_2 = pixel_get_signed_r16(pixel2);
			error += ((int64_t)r16_1 - r16_2) * ((int64_t)r16_1 - r16_2);
			int g16_1 = pixel_get_signed_g16(pixel1);
			int g16_2 = pixel_get_signed_g16(pixel2);
			error += ((int64_t)g16_1 - g16_2) * ((int64_t)g16_1 - g16_2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}

// Compare block image with source image with one signed 16-bit values encoded into each pixel.

double compare_block_4x4_r16_signed(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)

		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	unsigned int *pix1 = image_buffer;
	unsigned int *pix2 = user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 4) +
		user_data->x_offset;
	uint64_t error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int pixel1 = pix1[x];
			unsigned int pixel2 = pix2[x];
			int r16_1 = pixel_get_signed_r16(pixel1);
			int r16_2 = pixel_get_signed_r16(pixel2);
			error += ((int64_t)r16_1 - r16_2) * ((int64_t)r16_1 - r16_2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 4;
	}
	return (double)1 / error;
}


float *half_float_table = NULL;

void calculate_half_float_table() {
	if (half_float_table != NULL)
		return;
	half_float_table = (float *)malloc(sizeof(float) * 65536);
	for (int i = 0; i < 65536; i++) {
		uint16_t h[1];
		float f[1];
		h[0] = (uint16_t)i;
		halfp2singles(&f[0], &h[0], 1);
		half_float_table[i] = f[0];
	}
}

// Compare 4x4 rgba half-float block (64-bit pixels) in normalized format.

double compare_block_4x4_rgba_half_float(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	uint64_t *pix1 = (uint64_t *)image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint16_t *pix1p = (uint16_t *)&pix1[x];
			uint16_t *pix2p = (uint16_t *)&pix2[x];
			for (int i = 0; i < 4; i++) {
				float f = half_float_table[pix1p[i]];
				float g = half_float_table[pix2p[i]];
				error += (f - g) * (f - g);
			}
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	if (isnan(error)) {
		printf("Error -- unexpected NaN in half float block.\n");
//		exit(1);
	}
	return (double)1 / error;
}

// Compare 4x4 rgb half-float block (64-bit pixels) in normalized format.

double compare_block_4x4_rgb_half_float(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	uint64_t *pix1 = (uint64_t *)image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint16_t *pix1p = (uint16_t *)&pix1[x];
			uint16_t *pix2p = (uint16_t *)&pix2[x];
			for (int i = 0; i < 3; i++) {
				float f = half_float_table[pix1p[i]];
				float g = half_float_table[pix2p[i]];
				error += (f - g) * (f - g);
			}
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	if (isnan(error)) {
		printf("Error -- unexpected NaN in half float block.\n");
//		exit(1);
		return 0;
	}
	return (double)1 / error;
}

// Compare 4x4 rg half-float block (64-bit pixels).

double compare_block_4x4_rg_half_float(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	uint64_t *pix1 = (uint64_t *)image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint16_t *pix1p = (uint16_t *)&pix1[x];
			uint16_t *pix2p = (uint16_t *)&pix2[x];
			for (int i = 0; i < 2; i++) {
				float f = half_float_table[pix1p[i]];
				float g = half_float_table[pix2p[i]];
				error += (f - g) * (f - g);
			}
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	if (isnan(error)) {
		printf("Error -- unexpected NaN in half float block.\n");
//		exit(1);
	}
	return (double)1 / error;
}

// Compare 4x4 r half-float block (64-bit pixels).

double compare_block_4x4_r_half_float(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	uint64_t *pix1 = (uint64_t *)image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint16_t *pix1p = (uint16_t *)&pix1[x];
			uint16_t *pix2p = (uint16_t *)&pix2[x];
			float f = half_float_table[pix1p[0]];
			float g = half_float_table[pix2p[0]];
			error += (f - g) * (f - g);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	if (isnan(error)) {
		printf("Error -- unexpected NaN in half float block.\n");
//		exit(1);
	}
	return (double)1 / error;
}

float *gamma_corrected_half_float_table = NULL;

void calculate_gamma_corrected_half_float_table() {
	if (gamma_corrected_half_float_table != NULL)
		return;
	gamma_corrected_half_float_table = (float *)malloc(sizeof(float) * 65536);
	for (int i = 0; i < 65536; i++) {
		uint16_t h[1];
		float f[1];
		h[0] = (uint16_t)i;
		halfp2singles(&f[0], &h[0], 1);
		if (f[0] >= 0)
			gamma_corrected_half_float_table[i] = powf(f[0], 1 / 2.2);
		else
			gamma_corrected_half_float_table[i] = - powf(- f[0], 1 / 2.2);
	}
}

// Compare 4x4 rgba half-float block (64-bit pixels) in HDR (unnormalized) format.
// The alpha component is assumed to be in normalized format.

double compare_block_4x4_rgba_half_float_hdr(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	uint64_t *pix1 = (uint64_t *)image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	double error_alpha = 0;
	float range_min = FLT_MAX;
	float range_max = - FLT_MAX;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint16_t *pix1p = (uint16_t *)&pix1[x];
			uint16_t *pix2p = (uint16_t *)&pix2[x];
			for (int i = 0; i < 3; i++) {
				float f_corrected_comp = gamma_corrected_half_float_table[pix1p[i]];
				float g_corrected_comp = gamma_corrected_half_float_table[pix2p[i]];
				error += (f_corrected_comp - g_corrected_comp) * (f_corrected_comp - g_corrected_comp);
				if (g_corrected_comp < range_min)
					range_min = g_corrected_comp;
				if (g_corrected_comp > range_max)
					range_max = g_corrected_comp;
			}
			float a1, a2;
			a1 = half_float_table[pix1p[3]];
			a2 = half_float_table[pix2p[3]];
			error_alpha += (a1 - a2) * (a1 - a2);
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	double range = range_max - range_min;
	if (range != 0)
		error /= range * range;
	error += error_alpha;	// Range of alpha is 0 to 1.0.
	if (isnan(error)) {
		printf("Error -- unexpected NaN in half float block.\n");
//		exit(1);
	}
	return (double)1 / error;
}

// Compare 4x4 rgb half-float block (64-bit pixels) in HDR (unnormalized) format.

double compare_block_4x4_rgb_half_float_hdr(unsigned int *image_buffer, BlockUserData *user_data) {
	int w;
	if (user_data->x_offset + 4 > user_data->texture->width)
		w = user_data->texture->width - user_data->x_offset;
	else
		w = 4;
	int h;
	if (user_data->y_offset + 4 > user_data->texture->height)
		h = user_data->texture->height - user_data->y_offset;
	else
		h = 4;
	uint64_t *pix1 = (uint64_t *)image_buffer;
	uint64_t *pix2 = (uint64_t *)user_data->image_pixels + user_data->y_offset * (user_data->image_rowstride / 8) +
		user_data->x_offset;
	double error = 0;
	float range_min = FLT_MAX;
	float range_max = - FLT_MAX;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint16_t *pix1p = (uint16_t *)&pix1[x];
			uint16_t *pix2p = (uint16_t *)&pix2[x];
			for (int i = 0; i < 3; i++) {
				float f_corrected_comp = gamma_corrected_half_float_table[pix1p[i]];
				float g_corrected_comp = gamma_corrected_half_float_table[pix2p[i]];
				error += (f_corrected_comp - g_corrected_comp) * (f_corrected_comp - g_corrected_comp);
				if (g_corrected_comp < range_min)
					range_min = g_corrected_comp;
				if (g_corrected_comp > range_max)
					range_max = g_corrected_comp;
			}
		}
		pix1 = &pix1[4];
		pix2 += user_data->image_rowstride / 8;
	}
	double range = range_max - range_min;
	if (range != 0)
		error /= range * range;
	if (isnan(error)) {
		printf("Error -- unexpected NaN in half float block.\n");
//		exit(1);
		return 0;
	}
	return (double)1 / error;
}



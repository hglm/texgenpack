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
#include <math.h>
#include <malloc.h>
#include <ctype.h>
#include "texgenpack.h"
#include "packing.h"
#include "viewer.h"
#ifndef __GNUC__
#include "strcasecmp.h"
#endif


// Define the option variables used by the texgenpack modules.

int command;
int option_verbose = 0;
int option_max_threads = - 1;
int option_orientation = 0;
int option_texture_format = - 1;
int option_compression_level = SPEED_FAST;
int option_progress = 0;
int option_modal_etc2 = 0;
int option_allowed_modes_etc2 = - 1;
int option_mipmaps = 0;
int option_generations = - 1;
int option_islands = - 1;
int option_flip_vertical = 0;
int option_quiet = 0;
int option_block_width = 4;
int option_block_height = 4;
int option_half_float = 0;
int option_deterministic = 0;
int option_half_float_fit_to_range = 0;
int option_hdr = 0;

// Global variables

int current_nu_image_sets;
Image current_image[2][32];
Texture current_texture[2][32];
int current_nu_mipmaps[2];
int current_file_type[2];
double current_rmse[32];
float dynamic_range_min = 0;
float dynamic_range_max = 1.0;
float dynamic_range_gamma = 1.0;

// Variables used in this module only.

static char *source_filename[2];
static int source_filetype[2];

static int file_exists(const char *filename) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
		return 0;
	fclose(f);
	return 1;
}

static const char *extension[NU_FILE_TYPES] = {
	".png", ".ppm", ".ktx", ".pkm", ".dds", ".astc" };

static int file_type_id[NU_FILE_TYPES] = {
	FILE_TYPE_PNG, FILE_TYPE_PPM, FILE_TYPE_KTX, FILE_TYPE_PKM, FILE_TYPE_DDS, FILE_TYPE_ASTC };

int determine_filename_type(const char *filename) {
	if (strlen(filename) < 5) {
		printf("Error -- filename %s too short to be a valid file.\n", filename);
		exit(1);
	}
	for (int i = 0; i < NU_FILE_TYPES; i++)
		if (strcasecmp(&filename[strlen(filename) - strlen(extension[i])], extension[i]) == 0)
			return file_type_id[i];
	return FILE_TYPE_UNDEFINED;
}

static const char *get_extension(int filetype) {
	for (int i = 0; i < NU_FILE_TYPES; i++) {
		if (filetype == file_type_id[i])
			return extension[i];
	}
	return NULL;
}


// Convert the loaded images to Cairo's internal pixel format ARGB8.
// Swap red and blue components.

static void convert_image_set(int j) {
	for (int i = 0; i < current_nu_mipmaps[j]; i++) {
		for (int y = 0; y < current_image[j][i].height; y++)
			for (int x = 0; x < current_image[j][i].width; x++) {
				if (current_image[j][i].is_half_float) {
					uint64_t pixel64 = *(uint64_t *)&current_image[j][i].pixels[(y *
						current_image[j][i].extended_width + x) * 2];
					*(uint64_t *)&current_image[j][i].pixels[(y * current_image[j][i].extended_width
						+ x) * 2] = pack_rgba16(pixel64_get_b16(pixel64), pixel64_get_g16(pixel64),
						 pixel64_get_r16(pixel64), pixel64_get_a16(pixel64));
				}
				else {
					uint32_t pixel = current_image[j][i].pixels[y *
						current_image[j][i].extended_width + x];
					current_image[j][i].pixels[y * current_image[j][i].extended_width + x] =
						pack_rgba(pixel_get_b(pixel), pixel_get_g(pixel), pixel_get_r(pixel),
							pixel_get_a(pixel));
				}
			}	
	}
}

void destroy_image_set(int j) {
	if (j >= current_nu_image_sets)
		return;
	// Free pixel buffers associated with current_texture[j][i].
	if (current_file_type[j] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[j]; i++)
			free(current_texture[j][i].pixels); 
	// Free pixel buffers associated with current_image[j][i].
	if (current_nu_image_sets > j)
		for (int i = 0; i < current_nu_mipmaps[j]; i++)
			free(current_image[j][i].pixels); 
}

void convert_image_to_or_from_cairo_format(Image *image) {
	for (int y = 0; y < image->height; y++)
		for (int x = 0; x < image->width; x++) {
			if (image->is_half_float) {
				uint64_t pixel64 = *(uint64_t *)&image->pixels[(y *
					image->extended_width + x) * 2];
				*(uint64_t *)&image->pixels[(y * image->extended_width
					+ x) * 2] = pack_rgba16(pixel64_get_b16(pixel64), pixel64_get_g16(pixel64),
					 pixel64_get_r16(pixel64), pixel64_get_a16(pixel64));
			}
			else {
				uint32_t pixel = image->pixels[y * image->extended_width + x];
				image->pixels[y * image->extended_width + x] =
					pack_rgba(pixel_get_b(pixel), pixel_get_g(pixel), pixel_get_r(pixel),
					pixel_get_a(pixel));
			}
		}
}


void calculate_difference_images(Image *source_image1, Image *source_image2, int n, Image *dest_image) {
	for (int i = 0; i < n; i++) {
		int h = source_image1[i].height;
		if (source_image2[i].height < h)
			h = source_image2[i].height;
		int w = source_image1[i].width;
		if (source_image2[i].width < w)
			w = source_image2[i].width;
		dest_image[i] = source_image1[i]; // Copy fields.
		dest_image[i].pixels = (unsigned int *)malloc(h * source_image1[i].extended_width * 4);
		Image *source1;
		int source1_is_cloned = 0;
		Image cloned_image1;
		if (source_image1[i].is_half_float) {
			dest_image[i].is_half_float = 0;
			dest_image[i].bits_per_component = 8;
			clone_image(&source_image1[i], &cloned_image1);
			convert_image_from_half_float(&cloned_image1, dynamic_range_min, dynamic_range_max,
				dynamic_range_gamma);
			convert_image_to_or_from_cairo_format(&cloned_image1);
			source1 = &cloned_image1;
			source1_is_cloned = 1;
		}
		else
			source1 = &source_image1[i];
		Image *source2;
		int source2_is_cloned = 0;
		Image cloned_image2;
		if (source_image2[i].is_half_float) {
			clone_image(&source_image2[i], &cloned_image2);
			convert_image_from_half_float(&cloned_image2, dynamic_range_min, dynamic_range_max,
				dynamic_range_gamma);
			convert_image_to_or_from_cairo_format(&cloned_image2);
			source2 = &cloned_image2;
			source2_is_cloned = 1;
		}
		else
			source2 = &source_image2[i];
		for (int y = 0; y < h; y++)
			for (int x = 0; x < w; x++) {
				uint32_t pixel1 = source1->pixels[y * source1->extended_width + x];
				uint32_t pixel2 = source2->pixels[y * source2->extended_width + x];
				int difference = abs(pixel_get_r(pixel1) - pixel_get_r(pixel2)) +
					abs(pixel_get_g(pixel1) - pixel_get_g(pixel2)) +
					abs(pixel_get_b(pixel1) - pixel_get_b(pixel2));
				if (source1->alpha_bits > 0 && source2->alpha_bits > 0)
					difference += abs(pixel_get_a(pixel1) - pixel_get_a(pixel2));
				if (difference > 255)
					difference = 255;
				dest_image[i].pixels[y * dest_image[i].extended_width + x] =
					pack_rgb_alpha_0xff(difference, difference, difference);
			}
		if (source1_is_cloned)
			destroy_image(source1);
		if (source2_is_cloned)
			destroy_image(source2);
	}
}

int main(int argc, char **argv) {
	gui_initialize(&argc, &argv);
	gui_create_window_layout();
	gui_draw_window();

	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		printf("texview -- a texture and image file viewer.\n"
			"Usage: texview <filename1> <filename2>\n"
			"<filename2> is optional.\n");
		exit(1);
	}

	option_quiet = 1;
	current_nu_image_sets = 0;

	if (argc >= 2) {
		source_filename[0] = argv[1];
		source_filetype[0] = determine_filename_type(source_filename[0]);
	
		if (!(source_filetype[0] & FILE_TYPE_TEXTURE_BIT) && !(source_filetype[0] & FILE_TYPE_IMAGE_BIT)) {
			printf("Error -- expected image or texture filename as argument.\n");
			exit(1);
		}
		if (!file_exists(source_filename[0])) {
			printf("Error -- source file %s doesn't exist or is unreadable.\n", source_filename[0]);
			exit(1);
		}
		current_nu_image_sets = 1;
	}

	if (argc >= 3) {
		source_filename[1] = argv[2];
		source_filetype[1] = determine_filename_type(source_filename[1]);

		if (!(source_filetype[1] & FILE_TYPE_TEXTURE_BIT) && !(source_filetype[1] & FILE_TYPE_IMAGE_BIT)) {
			printf("Error -- expected image or texture filename as second argument.\n");
			exit(1);
		}

		if (!file_exists(source_filename[1])) {
			printf("Error -- source file %s doesn't exist or is unreadable.\n", source_filename[1]);
			exit(1);
		}	

		current_nu_image_sets = 2;
	}

	for (int j = 0; j < current_nu_image_sets; j++) {
		current_file_type[j] = source_filetype[j];
		if (source_filetype[j] & FILE_TYPE_TEXTURE_BIT) {
			current_nu_mipmaps[j] = load_texture(source_filename[j], source_filetype[j], 32,
				&current_texture[j][0]);
			for (int i = 0 ; i < current_nu_mipmaps[j]; i++)
				convert_texture_to_image(&current_texture[j][i], &current_image[j][i]);
		}
		else {
			load_image(source_filename[j], source_filetype[j], &current_image[j][0]);
			current_nu_mipmaps[j] = 1;
		}
		if (current_nu_mipmaps[j] > 1)
			printf("Found %d mipmap levels.\n", current_nu_mipmaps[j]);
	}

	current_rmse[0] = -1.0;
	if (current_nu_image_sets > 1)
		current_rmse[0] = compare_images(&current_image[0][0], &current_image[1][0]);

	for (int i = 0; i < current_nu_image_sets; i++) {
		if (!current_image[i][0].is_half_float &&
		current_image[i][0].bits_per_component == 8 &&
		current_image[i][0].nu_components > 2)
			convert_image_set(i);
	}
	gui_zoom_fit_to_window();
	for (int i = 0; i < current_nu_image_sets; i++)
		gui_create_base_surface(i);
	gui_draw_and_show_window();
	gui_run();
}


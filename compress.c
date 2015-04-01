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
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <malloc.h>
#include <fgen.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"

enum {
	// The RGB components of pixels in the block only have one color.
	BLOCK_FLAG_ONE_COLOR = 0x1,
	// The RGB components of pixels in the block only have two colors.
	BLOCK_FLAG_TWO_COLORS = 0x2,
	// The RGB components of pixels in the block only have three colors.
	BLOCK_FLAG_THREE_COLORS = 0x4,
	// The RGB components of pixels in the block only have four colors.
	BLOCK_FLAG_FOUR_COLORS = 0x8,
	// All alpha values of the block are 0xFF.
	BLOCK_FLAG_OPAQUE = 0x10,
	// None of the alpha values of the block are 0xFF.
	BLOCK_FLAG_NON_OPAQUE = 0x20,
	// The alpha values are either 0x00 or 0xFF
	BLOCK_FLAG_PUNCHTHROUGH = 0x40,
};

static void compress_with_single_population(Image *image, Texture *Texture);
static void compress_with_archipelago(Image *image, Texture *texture);
static void compress_multiple_blocks_concurrently(Image *image, Texture *texture);
static void compress_multiple_blocks_concurrently_second_pass(Image *image, Texture *texture);
static void set_alpha_pixels(Image *image, int x, int y, int w, int h, unsigned char *alpha_pixels);
static int get_block_flags_rgba8(Image *image, int x, int y, int w, int h, unsigned char *alpha_pixels,
unsigned int *colors);
static void optimize_alpha(Image *image, Texture *texture);
static double get_rmse_threshold(Texture *texture, int speed, Image *image);

static int nu_generations, nu_generations_second_pass;
static int population_size;
static int nu_islands, nu_islands_second_pass;
static float mutation_probability;
static float mutation_probability_second_pass;
static float crossover_probability;
static CompressCallbackFunction compress_callback_func;
static int mode_statistics[16];
static double rmse_threshold;

// Compress an image into a texture.

void compress_image(Image *image, int texture_type, CompressCallbackFunction callback_func, Texture *texture,
int genetic_parameters, float mutation_prob, float crossover_prob) {
	texture->info = match_texture_type(texture_type);
	if (texture_type & TEXTURE_TYPE_UNCOMPRESSED_BIT) {
		copy_image_to_uncompressed_texture(image, texture_type, texture);
		return;
	}
	if (texture_type & TEXTURE_TYPE_ASTC_BIT) {
		compress_image_to_astc_texture(image, texture_type, texture);
		return;
	}
	if ((texture_type & TEXTURE_TYPE_HALF_FLOAT_BIT) && !image->is_half_float) {
		printf("Error -- image is not in half float format.\n");
		exit(1);
	}
	texture->width = image->width;
	texture->height = image->height;
	texture->bits_per_block = texture->info->internal_bits_per_block;
	texture->type = texture_type;
	texture->block_width = texture->info->block_width;
	texture->block_height = texture->info->block_height;
	texture->extended_width = ((texture->width + texture->block_width - 1) / texture->block_width) *
		texture->block_width;
	texture->extended_height = ((texture->height + texture->block_height - 1) / texture->block_height)
		* texture->block_height;
	texture->pixels = (unsigned int *)malloc((texture->extended_height / texture->block_height) *
		(texture->extended_width / texture->block_width) * (texture->bits_per_block / 8));
	set_texture_decoding_function(texture, image);
	if ((texture_type & TEXTURE_TYPE_HALF_FLOAT_BIT) || image->is_half_float)
		calculate_half_float_table();
	if ((texture_type == TEXTURE_TYPE_BPTC_FLOAT || texture_type == TEXTURE_TYPE_BPTC_SIGNED_FLOAT) && option_hdr)
		calculate_gamma_corrected_half_float_table();
	if (image->is_half_float)
		calculate_normalized_float_table();
	rmse_threshold = get_rmse_threshold(texture, option_compression_level, image);

	if (option_verbose) {
		memset(mode_statistics, 0, sizeof(int) * 16);
	}
	compress_callback_func = callback_func;
	if (genetic_parameters) {
		mutation_probability = mutation_prob;
		crossover_probability = crossover_prob;
	}
	else {
		// Emperically determined mutation probability.
#if 0
		if (texture_type & TEXTURE_TYPE_128BIT_BIT)
			if (texture_type == TEXTURE_TYPE_BPTC_FLOAT)
				// bptc_float format needs higher mutation probability.
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.016;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.014;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.018;
				else	// SPEED_SLOW
					mutation_probability = 0.018;
			else
			if (texture_type == TEXTURE_TYPE_BPTC)
				// bptc
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.010;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.011;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.012;
				else	// SPEED_SLOW
					mutation_probability = 0.012;
			else
			if (texture_type == TEXTURE_TYPE_DXT5)
				// dxt5
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.020;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.017;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.018;
				else	// SPEED_SLOW
					mutation_probability = 0.018;
			else
				// Other 128-bit block texture types.
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.015;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.013;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.015;
				else	// SPEED_SLOW
					mutation_probability = 0.015;
		else	// 64-bit texture formats.
			if (texture_type == TEXTURE_TYPE_ETC1)
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.027;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.023;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.024;
				else	// SPEED_SLOW
					mutation_probability = 0.025;
			else
			if (texture_type == TEXTURE_TYPE_ETC2_RGB8)
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.023;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.022;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.024;
				else	// SPEED_SLOW
					mutation_probability = 0.025;
			else
			if (texture_type == TEXTURE_TYPE_DXT1)
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.028;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.025;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.026;
				else	// SPEED_SLOW
					mutation_probability = 0.026;
			else
			if (texture_type == TEXTURE_TYPE_R11_EAC)
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.029;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.028;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.026;
				else
					mutation_probability = 0.027;
			else	// Other 64-bit texture formats.
				if (option_speed == SPEED_ULTRA)
					mutation_probability = 0.025;
				else
				if (option_speed == SPEED_FAST)
					mutation_probability = 0.024;
				else
				if (option_speed == SPEED_MEDIUM)
					mutation_probability = 0.025;
				else
					mutation_probability = 0.026;
#else
		if (texture_type & TEXTURE_TYPE_128BIT_BIT)
			if (texture_type == TEXTURE_TYPE_BPTC_FLOAT)
				// bptc_float format needs higher mutation probability.
				mutation_probability = 0.014;
			else if (texture_type == TEXTURE_TYPE_BPTC)
				// bptc
				mutation_probability = 0.012;
			else if (texture_type == TEXTURE_TYPE_DXT5)
				mutation_probability = 0.017;
			else
				mutation_probability = 0.013;
		else	// 64-bit texture formats.
			if (texture_type == TEXTURE_TYPE_ETC1)
				mutation_probability = 0.023;
			else if (texture_type == TEXTURE_TYPE_ETC2_RGB8)
				mutation_probability = 0.022;
			else if (texture_type == TEXTURE_TYPE_DXT1)
				mutation_probability = 0.025;
			else if (texture_type == TEXTURE_TYPE_R11_EAC)
				mutation_probability = 0.028;
			else	// Other 64-bit texture formats.
				mutation_probability = 0.024;
#endif
		crossover_probability = 0.7;
	}
	mutation_probability_second_pass = mutation_probability * 0.66667f;
	if (option_compression_level < COMPRESSION_LEVEL_CLASS_1) {
		// Compression level class 0.
		population_size = 256;
		nu_generations = 100 + option_compression_level * 25;
		nu_islands = 8;
		nu_generations_second_pass = 4000;
		nu_islands_second_pass = 8;
	}
	else if (option_compression_level < COMPRESSION_LEVEL_CLASS_2 - 1) {
		// Compression level class 1.
		population_size = 128;
		nu_generations = 50;
		nu_islands = option_compression_level;		
		nu_generations_second_pass = 4000;
		nu_islands_second_pass = 4;
	}
	else {
		// Compression level class 2.
		population_size = 128;
		nu_generations = 50 + (option_compression_level - 32) * 10;
		nu_islands = 32;
		nu_generations_second_pass = 8000;
		nu_islands_second_pass = 4;
	}

	if (option_generations != - 1)
		nu_generations = option_generations;
	if (option_islands != - 1)
		nu_islands = option_islands;
	if (option_generations_second_pass != - 1)
		nu_generations_second_pass = option_generations_second_pass;
	if (option_islands_second_pass != - 1)
		nu_islands = option_islands_second_pass;
#if 0
	if (option_max_threads != -1) {
		if (option_compression_level >= COMPRESSION_LEVEL_CLASS_1) {
			// When the maximum number of threads is constrained, modify the GA parameters
			// for roughly similar quality by doubling the number of generations.
			while (nu_islands > option_max_threads) {
				nu_islands /= 2;
				nu_generations *= 2;
			}
			while (nu_islands_second_pass > option_max_threads) {
				nu_islands_second_pass /= 2;
				nu_generations_second_pass *= 2;
			}
		}
	}
#endif

	if (option_perceptive && option_compression_level >= COMPRESSION_LEVEL_CLASS_1
	&& texture->perceptive_comparison_function == NULL && !option_quiet)
		printf("Warning: Perceptive quality strategy not available for texture format.\n");
	if (option_compression_level < COMPRESSION_LEVEL_CLASS_1)
		compress_multiple_blocks_concurrently(image, texture);
	else
		compress_with_archipelago(image, texture);

	// Perform second pass (non-perceptive quality mode, compression level class 0 (ultra)).
	if (option_compression_level < COMPRESSION_LEVEL_CLASS_1) {
		if (option_verbose) {
			// Decompress the compressed texture and calculate the difference with the original
			// and report it.
			Image compressed_image;
			convert_texture_to_image(texture, &compressed_image);
			compare_images(image, &compressed_image);
			destroy_image(&compressed_image);
		}
		compress_multiple_blocks_concurrently_second_pass(image, texture);
	}

	// Optionally post-process the texture to optimize the alpha values.
//	optimize_alpha(image, texture);

	if (option_verbose) {
		int nu_modes = 0;
		if (texture->type == TEXTURE_TYPE_ETC1)
			nu_modes = 2;
		else if (texture->type == TEXTURE_TYPE_ETC2_RGB8 || texture->type == TEXTURE_TYPE_ETC2_EAC ||
		texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH)
			nu_modes = 5;
		else if (texture->type == TEXTURE_TYPE_BPTC)
			nu_modes = 8;
		else if (texture->type == TEXTURE_TYPE_BPTC_FLOAT || texture->type == TEXTURE_TYPE_BPTC_SIGNED_FLOAT)
			nu_modes = 14;
		if (nu_modes > 0) {
			printf("Mode statistics:\n");
			for (int i = 0; i < nu_modes; i++)
				printf("Mode %d: %d blocks\n", i, mode_statistics[i]);
		}
	}
}


// The fitness function of the genetic algorithm.

static double calculate_fitness(const FgenPopulation *pop, const unsigned char *bitstring) {
	unsigned int image_buffer[32];	// 16 required for regular pixels, 32 for 64-bit pixel formats like half floats.
	BlockUserData *user_data = (BlockUserData *)pop->user_data;
	int flags = user_data->flags;
	int r = user_data->texture->decoding_function(bitstring, image_buffer, flags);
	if (r == 0) {
//		printf("Invalid block.\n");
		return 0;	// Fitness is zero for invalid blocks.
	}
	if (option_perceptive && option_compression_level >= COMPRESSION_LEVEL_CLASS_1 &&
	user_data->texture->perceptive_comparison_function != NULL)
		return user_data->texture->perceptive_comparison_function(image_buffer, user_data);
	else
		return user_data->texture->comparison_function(image_buffer, user_data);
}

// The generation callback function of the genetic algorithm.

static void generation_callback(FgenPopulation *pop, int generation) {
	if (generation == 0)
		return;
	if (generation >= nu_generations * 2) {
		fgen_signal_stop(pop);
		return;
	}
	// Adaptive, if the fitness is above the threshold, stop, otherwise go on for another nu_generations
	// generations.
	FgenIndividual *best = fgen_best_individual_of_population(pop);
	double rmse = sqrt((1.0 / best->fitness) / 16);
	if (generation <= nu_generations && rmse < rmse_threshold)
		fgen_signal_stop(pop);
}

static void generation_callback_non_adaptive(FgenPopulation *pop, int generation) {
	if (generation == 0)
		return;
	if (generation >= nu_generations) {
		fgen_signal_stop(pop);
		return;
	}
}

static void generation_callback_archipelago(FgenPopulation *pop, int generation) {
	if (generation == 0)
		return;
	if (generation >= nu_generations * 3) {
		fgen_signal_stop(pop);
		return;
	}
	// Adaptive, if the fitness is above the threshold, stop, otherwise go on for another nu_generations
	// generations.
	FgenIndividual *best = fgen_best_individual_of_population(pop);
	double rmse = sqrt((1.0 / best->fitness) / 16);
	if (generation <= nu_generations) {
		if (rmse < rmse_threshold)
			fgen_signal_stop(pop);
	}
	else if (generation <= nu_generations * 2) {
		if (rmse < rmse_threshold * 1.5)
			fgen_signal_stop(pop);
	}
}

// Generation call-back for second-pass.

static void generation_callback_second_pass(FgenPopulation *pop, int generation) {
	if (generation == 0)
		return;
	if (generation >= nu_generations_second_pass) {
		fgen_signal_stop(pop);
		return;
	}
}

// Custom seeding function for 128-bit formats for archipelagos where each island is compressing the same block.

static void seed_128bit(FgenPopulation *pop, unsigned char *bitstring) {
	BlockUserData *user_data = (BlockUserData *)pop->user_data;
	FgenRNG *rng = fgen_get_rng(pop);
	int r = fgen_random_8(rng);
	// The chance of seeding with an already calculated neighbour block should be chosen carefully.
	// A too high probability results in less diversity in the archipelago.
	int factor;
	if (population_size == 128)
		factor = 1;
	else	// population_size == 64?
		factor = 2;
	if (r < 2 * factor && user_data->x_offset > 0) {
		// Seed with solution to the left with chance 1/128th (1/64th if population size is 64).
		int compressed_block_index = (user_data->y_offset / user_data->texture->block_height) *
			(user_data->texture->extended_width / user_data->texture->block_width) +
			(user_data->x_offset / user_data->texture->block_width) - 1;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 4];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 4 + 1];
		*(unsigned int *)&bitstring[8] = user_data->texture->pixels[compressed_block_index * 4 + 2];
		*(unsigned int *)&bitstring[12] = user_data->texture->pixels[compressed_block_index * 4 + 3];
		goto end;
	}
	if (r < 4 * factor && user_data->y_offset > 0) {
		// Seed with solution above with chance 1/128th (1/64th for x == 0).
		// 1/64th if population size is 64.
		int compressed_block_index = (user_data->y_offset / user_data->texture->block_height - 1) *
			(user_data->texture->extended_width / user_data->texture->block_width) +
			user_data->x_offset / user_data->texture->block_width;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 4];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 4 + 1];
		*(unsigned int *)&bitstring[8] = user_data->texture->pixels[compressed_block_index * 4 + 2];
		*(unsigned int *)&bitstring[12] = user_data->texture->pixels[compressed_block_index * 4 + 3];
		goto end;
	}
	if (r < 6 * factor && (user_data->x_offset > 0 || user_data->y_offset > 0)) {
		// Seed with a random already calculated solution with chance 1/128th.
		// 1/64th if population size is 64.
		int x, y;
		if (user_data->y_offset == 0) {
			x = fgen_random_n(rng, user_data->x_offset / user_data->texture->block_width);
			y = 0;
		}
		else {
			int i = fgen_random_n(rng, (user_data->y_offset / user_data->texture->block_height) *
				(user_data->texture->extended_width / user_data->texture->block_width)
				+ user_data->x_offset / user_data->texture->block_width);
			x = i % (user_data->texture->extended_width / user_data->texture->block_width);
			y = i / (user_data->texture->extended_width / user_data->texture->block_width);
		}
		int compressed_block_index = y * (user_data->texture->extended_width / user_data->texture->block_width) + x;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 4];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 4 + 1];
		*(unsigned int *)&bitstring[8] = user_data->texture->pixels[compressed_block_index * 4 + 2];
		*(unsigned int *)&bitstring[12] = user_data->texture->pixels[compressed_block_index * 4 + 3];
		goto end;
	}
	int nu_tries = 0;
again :
	fgen_seed_random(pop, bitstring);
	if (user_data->texture->set_mode_function != NULL)
		user_data->texture->set_mode_function(bitstring, user_data->flags);
	unsigned int image_buffer[16];
	bool valid;
	valid = user_data->texture->decoding_function(bitstring, image_buffer, user_data->flags);
	nu_tries++;
	if (nu_tries == 100)
		printf("Giving up seeding with constraints after 100 tries.\n");
	else if (!valid)
		goto again;
//	printf("Seed: %d tries.\n", nu_tries);

end :
	if (user_data->texture->type == TEXTURE_TYPE_BPTC)
		bptc_set_block_colors(bitstring, user_data->flags, user_data->colors);

	if (user_data->texture->type == TEXTURE_TYPE_DXT3)
		optimize_block_alpha_dxt3(bitstring, user_data->alpha_pixels);
	else if (user_data->texture->type == TEXTURE_TYPE_ETC2_EAC) {
		optimize_block_alpha_etc2_eac(bitstring, user_data->alpha_pixels, user_data->flags);
#if 0
		if (user_data->flags & MODES_ALLOWED_PUNCHTHROUGH_ONLY)
			printf("Seed: Block is punchthrough-only.\n");
		unsigned int image_buffer[16];
		int r =	draw_block4x4_etc2_eac(bitstring, image_buffer, ETC2_MODE_ALLOWED_ALL);
		if (!r)
			printf("Seed: Invalid block.\n");
		printf("Seed: Target alpha values: ");
		for (int i = 0; i < 16; i++) {
			int a = user_data->alpha_pixels[i];
			printf("0x%02X ", a);
		}
		printf("\n");
		printf("Seed: Compressed alpha values (0x%08X): ", *(unsigned int *)bitstring);
		for (int i = 0; i < 16; i++) {
			int a = pixel_get_a(image_buffer[i]);
			printf("0x%02X ", a);
		}
		printf("\n");
#endif
	}
}

// Seeding function for archipelagos where each island is compressing the same block.

static void seed(FgenPopulation *pop, unsigned char *bitstring) {
	BlockUserData *user_data = (BlockUserData *)pop->user_data;
	if (user_data->texture->bits_per_block == 128) {
		seed_128bit(pop, bitstring);
		return;
	}
	FgenRNG *rng = fgen_get_rng(pop);
	int r = fgen_random_8(rng);
	// The chance of seeding with an already calculated neighbour block should be chosen carefully.
	// A too high probability results in less diversity in the archipelago.
	int factor;
	if (population_size == 128)
		factor = 1;
	else	// population_size == 64
		factor = 2;
	if (r < 2 * factor && user_data->x_offset > 0) {
		// Seed with solution to the left with chance 1/128th.
		int compressed_block_index = (user_data->y_offset / user_data->texture->block_height) *
			(user_data->texture->extended_width / user_data->texture->block_width) +
			(user_data->x_offset / user_data->texture->block_width) - 1;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 2];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 2 + 1];
		goto end;
	}
	if (r < 4 * factor && user_data->y_offset > 0) {
		// Seed with solution above with chance 1/128th (1/64th for x == 0).
		int compressed_block_index = (user_data->y_offset / user_data->texture->block_height - 1) *
			(user_data->texture->extended_width / user_data->texture->block_width) +
			user_data->x_offset / user_data->texture->block_width;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 2];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 2 + 1];
		goto end;
	}
	if (r < 6 * factor && (user_data->x_offset > 0 || user_data->y_offset > 0)) {
		// Seed with a random already calculated solution with chance 1/128th.
		int x, y;
		if (user_data->y_offset == 0) {
			x = fgen_random_n(rng, user_data->x_offset / user_data->texture->block_width);
			y = 0;
		}
		else {
			int i = fgen_random_n(rng, (user_data->y_offset / user_data->texture->block_height) *
				(user_data->texture->extended_width / user_data->texture->block_width)
				+ user_data->x_offset / user_data->texture->block_width);
			x = i % (user_data->texture->extended_width / user_data->texture->block_width);
			y = i / (user_data->texture->extended_width / user_data->texture->block_width);
		}
		int compressed_block_index = y * (user_data->texture->extended_width / user_data->texture->block_width) + x;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 2];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 2 + 1];
		goto end;
	}
	int nu_tries = 0;
again :
	fgen_seed_random(pop, bitstring);
	if (user_data->texture->set_mode_function != NULL) {
		user_data->texture->set_mode_function(bitstring, user_data->flags);
	}
	unsigned int image_buffer[16];
	bool valid;
	valid = user_data->texture->decoding_function(bitstring, image_buffer, user_data->flags);
	nu_tries++;
	if (nu_tries == 100)
		printf("Giving up seeding with constraints after 100 tries.\n");
	else if (!valid)
		goto again;
//	printf("Seed: %d tries (flags = 0x%08X).\n", nu_tries, user_data->flags);
end :
	if (user_data->texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH)
		optimize_block_alpha_etc2_punchthrough(bitstring, user_data->alpha_pixels);
}

// 128-bit seeding function for archipelagos where each island is compressing a different block (--ultra setting).
// Population size is assumed to be 256.

static void seed2_128bit(FgenPopulation *pop, unsigned char *bitstring) {
	BlockUserData *user_data = (BlockUserData *)pop->user_data;
	FgenRNG *rng = fgen_get_rng(pop);
	int r = fgen_random_8(rng);
	// The chance of seeding with an already calculated neighbour block should be chosen carefully.
	// A too high probability results in less diversity in the archipelago.
	if (r < 3 && user_data->y_offset > 0) {
		// Seed with solution above with chance 3/256th.
		int compressed_block_index = (user_data->y_offset / user_data->texture->block_height - 1) *
			(user_data->texture->extended_width / user_data->texture->block_width) +
			user_data->x_offset / user_data->texture->block_width;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 4];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 4 + 1];
		*(unsigned int *)&bitstring[8] = user_data->texture->pixels[compressed_block_index * 4 + 2];
		*(unsigned int *)&bitstring[12] = user_data->texture->pixels[compressed_block_index * 4 + 3];
		goto end;
	}
	if (r < 6 && user_data->y_offset > 0) {
		// Seed with a random already calculated solution from the area above with chance 3/256th
		int x, y;
		int i = fgen_random_n(rng, (user_data->y_offset / user_data->texture->block_height) *
			(user_data->texture->extended_width / user_data->texture->block_width));
		x = i % (user_data->texture->extended_width / user_data->texture->block_width);
		y = i / (user_data->texture->extended_width / user_data->texture->block_width);
		int compressed_block_index = y * (user_data->texture->extended_width / user_data->texture->block_width) + x;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 4];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 4 + 1];
		*(unsigned int *)&bitstring[8] = user_data->texture->pixels[compressed_block_index * 4 + 2];
		*(unsigned int *)&bitstring[12] = user_data->texture->pixels[compressed_block_index * 4 + 3];
		goto end;
	}
	fgen_seed_random(pop, bitstring);
end : ;
	if (user_data->texture->type == TEXTURE_TYPE_DXT3)
		optimize_block_alpha_dxt3(bitstring, user_data->alpha_pixels);
}

// Seeding function for archipelagos where each island is compressing a different block (--ultra setting).

static void seed2(FgenPopulation *pop, unsigned char *bitstring) {
	BlockUserData *user_data = (BlockUserData *)pop->user_data;
	if (user_data->texture->bits_per_block == 128) {
		seed2_128bit(pop, bitstring);
		return;
	}
	FgenRNG *rng = fgen_get_rng(pop);
	int r = fgen_random_8(rng);
	// The chance of seeding with an already calculated neighbour block should be chosen carefully.
	// A too high probability results in less diversity in the archipelago.
	if (r < 3 && user_data->y_offset > 0) {
		// Seed with solution above with chance 3/256th
		int compressed_block_index = (user_data->y_offset / 4 - 1) * (user_data->texture->extended_width /
			user_data->texture->block_width) + user_data->x_offset / user_data->texture->block_width;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 2];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 2 + 1];
		goto end;
	}
	if (r < 6 && user_data->y_offset > 0) {
		// Seed with a random already calculated solution from the area above with chance 3/256th
		int x, y;
		int i = fgen_random_n(rng, (user_data->y_offset / user_data->texture->block_height) *
			(user_data->texture->extended_width / user_data->texture->block_width));
		x = i % (user_data->texture->extended_width / user_data->texture->block_width);
		y = i / (user_data->texture->extended_width / user_data->texture->block_width);
		int compressed_block_index = y * (user_data->texture->extended_width / user_data->texture->block_width) + x;
		*(unsigned int *)&bitstring[0] = user_data->texture->pixels[compressed_block_index * 2];
		*(unsigned int *)&bitstring[4] = user_data->texture->pixels[compressed_block_index * 2 + 1];
		goto end;
	}
	fgen_seed_random(pop, bitstring);
end : ;
	if (user_data->texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH)
		optimize_block_alpha_etc2_punchthrough(bitstring, user_data->alpha_pixels);
}

// Seeding function for second_pass. Seed whole population with same texture data.

static void seed_second_pass(FgenPopulation *pop, unsigned char *bitstring) {
	BlockUserData *user_data = (BlockUserData *)pop->user_data;
	Texture *texture = user_data->texture;
	int x_offset = user_data->x_offset;
	int y_offset = user_data->y_offset;
	int compressed_block_index = (y_offset / texture->block_height) * (texture->extended_width / texture->block_width)
		+ x_offset / texture->block_width;
	if (texture->bits_per_block == 64) {
		// Copy the 64-bit block.
		*(unsigned int *)bitstring = texture->pixels[compressed_block_index * 2]; 
		*(unsigned int *)&bitstring[4] = texture->pixels[compressed_block_index * 2 + 1];
	}
	else {
		// Copy the 128-bit block.
		*(unsigned int *)bitstring = texture->pixels[compressed_block_index * 4];
		*(unsigned int *)&bitstring[4] = texture->pixels[compressed_block_index * 4 + 1];
		*(unsigned int *)&bitstring[8] = texture->pixels[compressed_block_index * 4 + 2];
		*(unsigned int *)&bitstring[12] = texture->pixels[compressed_block_index * 4 + 3];
	}
}

// Set the auxilliary data field for the GA population.

static void set_user_data(BlockUserData *user_data, Image *image, Texture *texture, int pass) {
	user_data->flags = ENCODE_BIT;
	if (texture->type == TEXTURE_TYPE_ETC1)
		user_data->flags |= ETC_MODE_ALLOWED_ALL;
	else if (texture->type == TEXTURE_TYPE_ETC2_RGB8 || texture->type == TEXTURE_TYPE_ETC2_EAC)
		user_data->flags |= ETC2_MODE_ALLOWED_ALL;
	else if (texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH)
		user_data->flags |= ETC2_PUNCHTHROUGH_MODE_ALLOWED_ALL;
	else if (texture->type == TEXTURE_TYPE_BPTC)
		user_data->flags |= BPTC_MODE_ALLOWED_ALL;
	else if (texture->type == TEXTURE_TYPE_BPTC_FLOAT || texture->type == TEXTURE_TYPE_BPTC_SIGNED_FLOAT)
		user_data->flags |= BPTC_FLOAT_MODE_ALLOWED_ALL;
	user_data->image_pixels = image->pixels;
	if (image->is_half_float)
		user_data->image_rowstride = image->extended_width * 8;
	else
		user_data->image_rowstride = image->extended_width * 4;
	user_data->texture = texture;
	user_data->stop_signalled = 0;
	user_data->pass = pass;
}

static void set_user_data_block_flags(BlockUserData *user_data, Texture *texure, int block_flags) {
	user_data->flags &= ~(MODES_ALLOWED_OPAQUE_ONLY | MODES_ALLOWED_NON_OPAQUE_ONLY |
		MODES_ALLOWED_PUNCHTHROUGH_ONLY | TWO_COLORS);
	if (block_flags & BLOCK_FLAG_OPAQUE)
		user_data->flags |= MODES_ALLOWED_OPAQUE_ONLY;
	else if (block_flags & BLOCK_FLAG_NON_OPAQUE)
		user_data->flags |= MODES_ALLOWED_NON_OPAQUE_ONLY;
	if (block_flags & BLOCK_FLAG_PUNCHTHROUGH)
		user_data->flags |= MODES_ALLOWED_PUNCHTHROUGH_ONLY;
	if (block_flags & BLOCK_FLAG_TWO_COLORS)
		user_data->flags |= TWO_COLORS;
}

static void set_user_data_mode_flags(int i, BlockUserData *user_data, int block_flags) {
		if ((user_data->texture->type & TEXTURE_TYPE_ETC_BIT) && option_allowed_modes_etc2 !=  - 1)
			user_data->flags = option_allowed_modes_etc2 |
				ENCODE_BIT;
		else if (user_data->texture->type == TEXTURE_TYPE_ETC1 && option_modal_etc2 && nu_islands >= 2) {
			if ((i & 1) == 0)
				user_data->flags =
					ETC_MODE_ALLOWED_INDIVIDUAL | ENCODE_BIT;
			else
				user_data->flags =
					ETC_MODE_ALLOWED_DIFFERENTIAL | ENCODE_BIT;
		}
		else if ((user_data->texture->type == TEXTURE_TYPE_ETC2_RGB8 ||
		user_data->texture->type == TEXTURE_TYPE_ETC2_EAC)
		&& option_modal_etc2 && nu_islands >= 8) {
			switch (i & 7) {
			case 0 :
			case 1 :
				user_data->flags =
					ETC_MODE_ALLOWED_INDIVIDUAL;
				break;
			case 2 :
			case 3 :
				user_data->flags =
					ETC_MODE_ALLOWED_DIFFERENTIAL;
				break;
			case 4 :
				user_data->flags =
					ETC2_MODE_ALLOWED_T;
				break;
			case 5 :
				user_data->flags =
					ETC2_MODE_ALLOWED_H;
				break;
			case 6 :
			case 7 :
				user_data->flags =
					ETC2_MODE_ALLOWED_PLANAR;
				break;
			}
			user_data->flags |= ENCODE_BIT;
		}
		else if (user_data->texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH && option_modal_etc2 &&
		nu_islands >= 8) {
			switch (i & 7) {
			case 0 :
			case 1 :
				user_data->flags =
					ETC_MODE_ALLOWED_DIFFERENTIAL | ENCODE_BIT;
				break;
			case 2 :
			case 3 :
				user_data->flags =
					ETC2_MODE_ALLOWED_T | ENCODE_BIT;
				break;
			case 4 :
			case 5 :
				user_data->flags =
					ETC2_MODE_ALLOWED_H | ENCODE_BIT;
				break;
			case 6 :
			case 7 :
				if (!(block_flags & BLOCK_FLAG_OPAQUE))
					if (i == 6)
						user_data->flags = ETC2_MODE_ALLOWED_T;
					else
						user_data->flags = ETC2_MODE_ALLOWED_H;
				else
					user_data->flags =
						ETC2_MODE_ALLOWED_PLANAR | ENCODE_BIT;
				break;
			}
		}
		// For the BPTC texture format, distribute different modes over the available islands.
		if (user_data->texture->type == TEXTURE_TYPE_BPTC && nu_islands >= 8) {
			if (block_flags & BLOCK_FLAG_OPAQUE)
				if (block_flags & BLOCK_FLAG_TWO_COLORS)
					// Only use mode 3.
					user_data->flags = (1 << 3) | ENCODE_BIT;
				else
					// Use modes 0 to 3 only.
					user_data->flags = (1 << (i & 3)) | ENCODE_BIT;
			else
				if ((block_flags & BLOCK_FLAG_PUNCHTHROUGH) && (block_flags & BLOCK_FLAG_TWO_COLORS))
					// Only use mode 5 and 6.
					user_data->flags = (1 << (5 + (i & 1))) | ENCODE_BIT;
				else
					// Modes 4 to 7.
					user_data->flags = (1 << (4 + (i & 3))) | ENCODE_BIT;
		}
		// For the BPTC_FLOAT texture format, distribute different modes over the available islands.
		if (user_data->texture->type == TEXTURE_TYPE_BPTC_FLOAT ||
		user_data->texture->type == TEXTURE_TYPE_BPTC_SIGNED_FLOAT) {
			if (nu_islands >= 8) {
				switch (i & 7) {
				case 0 :
				case 1 :	// Mode 0 (very common).
					user_data->flags = 0x1 | ENCODE_BIT;
					break;
				case 2 :	// Modes 5 (common) and 9.
					user_data->flags = (1 << 5) | (1 << 9) | ENCODE_BIT;
					break;
				case 3 :	// Modes 2 and 6 (common).
					user_data->flags = (1 << 2) | (1 << 6) | ENCODE_BIT;
					break;
				case 4 : 	// Modes 3 and 7 (common).
					user_data->flags = (1 << 3) | (1 << 7) | ENCODE_BIT;
					break;
				case 5 :	// Modes 1, 4, 8 (common).
					user_data->flags = (1 << 1) | (1 << 4) | (1 << 8)
						| ENCODE_BIT;
					break;
				case 6 :	// Mode 11.
					user_data->flags = (1 << 11) | ENCODE_BIT;
					break;
				case 7 :	// Modes 10, 11, 12, 13.
					user_data->flags = (0xF << 10) | ENCODE_BIT;
					break;
				}
			}
			else
			if (/* option_modal_etc2 && */ nu_islands >= 4) {
				switch (i & 3) {
				case 0 :	// Mode 0.
					user_data->flags = 0x1 | ENCODE_BIT;
					break;
				case 1 :	// Modes 2, 4, 6 (common), 8 (common), 9.
					user_data->flags = (1 << 2) | (1 << 4) |
						(1 << 6) | (1 << 8) | (1 << 9) | ENCODE_BIT;
					break;
				case 2 :	// Modes 1, 3, 5 (common), 7 (common).
					user_data->flags = (1 << 1) | (1 << 3) |
						(1 << 5) | (1 << 7) | ENCODE_BIT;
					break;
				case 3 :	// Modes 10, 11, 12, 13.
					user_data->flags = (0xF << 10) | ENCODE_BIT;
					break;
				}
			}
		}
}

static char *etc2_modestr = "IDTHP";

// Report the given GA solution and store its bitstring in the texture, printing information if required.

static void report_solution(FgenIndividual *best, BlockUserData *user_data, int nu_gens, bool compress_callback) {
	Texture *texture = user_data->texture;
	int x_offset = user_data->x_offset;
	int y_offset = user_data->y_offset;
	// Calculate the block index of the block.
	int compressed_block_index = (y_offset / texture->block_height) * (texture->extended_width / texture->block_width)
		+ x_offset / texture->block_width;
	if (texture->bits_per_block == 64) {
		// Copy the 64-bit block.
		texture->pixels[compressed_block_index * 2] = *(unsigned int *)best->bitstring;
		texture->pixels[compressed_block_index * 2 + 1] = *(unsigned int *)&best->bitstring[4];
	}
	else {
		// Copy the 128-bit block.
		texture->pixels[compressed_block_index * 4] = *(unsigned int *)best->bitstring;
		texture->pixels[compressed_block_index * 4 + 1] = *(unsigned int *)&best->bitstring[4];
		texture->pixels[compressed_block_index * 4 + 2] = *(unsigned int *)&best->bitstring[8];
		texture->pixels[compressed_block_index * 4 + 3] = *(unsigned int *)&best->bitstring[12];
	}
	// When perceptive compression is enabled, store the decompressed block data.
	if (option_perceptive && option_compression_level >= COMPRESSION_LEVEL_CLASS_1)
		user_data->texture->decoding_function(best->bitstring, user_data->texture_pixels, user_data->flags);
	// Print info.
	if (option_verbose) {
		printf("Block %d: ", (y_offset / texture->block_height) * (texture->extended_width / texture->block_width)
			+ (x_offset / texture->block_width));
		if (texture->type & TEXTURE_TYPE_ETC_BIT) {
			int mode = texture->get_mode_function(best->bitstring);
			printf("Mode: %c ", etc2_modestr[mode]);
			if (compress_callback)
				mode_statistics[mode]++;
		}
		else if (texture->type == TEXTURE_TYPE_BPTC ||
		texture->type == TEXTURE_TYPE_BPTC_FLOAT || texture->type == TEXTURE_TYPE_BPTC_SIGNED_FLOAT) {
			int mode = texture->get_mode_function(best->bitstring);
			printf("Mode: %d ", mode);
			if (compress_callback)
				mode_statistics[mode]++;
		}
		printf("Combined: ");
		printf("RMSE per pixel: %lf, ", sqrt((1.0 / best->fitness) / 16));
		printf("GA generations: %d\n", nu_gens);
	}
	if (option_progress) {
		int n = (texture->extended_width / texture->block_width) * (texture->extended_height / texture->block_height);
		int old_percentage = (compressed_block_index - 1) * 100 / n;
		int new_percentage = compressed_block_index * 100 / n;
		if (new_percentage == 99 && old_percentage == 98)
			printf("99%%\n");
		else
		if (new_percentage != old_percentage)
			printf("%d%% ", new_percentage);
		fflush(stdout);
	}
	if (compress_callback)
		compress_callback_func(user_data);
}

// Compress each block with a single GA population. Unused.

static void compress_with_single_population(Image *image, Texture *texture) {
	if (!option_quiet)
		printf("Running single GA for each pixel block.\n");
	FgenPopulation *pop = fgen_create(
		population_size,		// Population size.
		texture->bits_per_block,	// Number of bits.
		1,				// Data element size.
		generation_callback,
		calculate_fitness,
		fgen_seed_random,
		fgen_mutation_per_bit_fast,
		fgen_crossover_uniform_per_bit
		);
	fgen_set_parameters(
		pop,
		FGEN_ELITIST_SUS,
		FGEN_SUBTRACT_MIN_FITNESS,
		crossover_probability,	// Crossover prob.
		mutation_probability,	// Mutation prob. per bit
		0		// Macro-mutation prob.
		);
	fgen_set_generation_callback_interval(pop, nu_generations);
	if (!option_deterministic)
		fgen_random_seed_with_timer(fgen_get_rng(pop));
//	fgen_random_seed_rng(fgen_get_rng(pop), 0);
	pop->user_data = (BlockUserData *)malloc(sizeof(BlockUserData));
	set_user_data((BlockUserData *)pop->user_data, image, texture, 1);
	for (int y = 0; y < image->extended_height; y += texture->block_height)
		for (int x = 0; x < image->extended_width; x+= texture->block_width) {
			BlockUserData *user_data = (BlockUserData *)pop->user_data;
			user_data->x_offset = x;
			user_data->y_offset = y;
			// Threading a single population doesn't work well in this case because the of the relatively quick
			// fitness function, which causes too much overhead caused by threading.
//			if (use_threading)
//				fgen_run_threaded(pop, nu_generations);
//			else
				fgen_run(pop, nu_generations);
			report_solution(fgen_best_individual_of_population(pop), user_data, pop->generation, true);
			if (user_data->stop_signalled)
				goto end;
		}
end :
	free(pop->user_data);
	fgen_destroy(pop);
}

unsigned int *allocate_texture_pixels(Texture *texture) {
	int n = (texture->extended_height / texture->block_height) *
		(texture->extended_width / texture->block_width);
	int bytespp = 4;
	if (texture->type & TEXTURE_TYPE_HALF_FLOAT_BIT)
		bytespp = 8;	// 64-bit pixels
	unsigned int *texture_pixels = (unsigned int *)malloc(n * texture->block_width * texture->block_height
		* bytespp);
	return texture_pixels;
}

// Compress each block with an archipelago of algorithms running on the same block. The best one is chosen.

static void compress_with_archipelago(Image *image, Texture *texture) {
	unsigned char *alpha_pixels = (unsigned char *)alloca(texture->block_width * texture->block_height);
	unsigned int colors[2];
	FgenPopulation **pops = (FgenPopulation **)alloca(sizeof(FgenPopulation *) * nu_islands);
	FgenPopulation **pops2 = (FgenPopulation **)alloca(sizeof(FgenPopulation *) * nu_islands_second_pass);
	if (!option_quiet) {
		const char *quality_strategy;
		if (option_perceptive && option_compression_level >= COMPRESSION_LEVEL_CLASS_1 &&
		texture->perceptive_comparison_function != NULL)
			quality_strategy = "perceptive";
		else
			quality_strategy = "non-perceptive";
		printf("Running GA archipelago of size %d for each pixel block, population size %d, "
			"%d-%d generations, "
			"%s quality strategy, second pass population size 8, %d generations.\n",
			nu_islands, population_size, nu_generations, nu_generations * 3,
			quality_strategy, nu_generations_second_pass);
	}
	unsigned int *texture_pixels;
	if (option_perceptive)
		texture_pixels = allocate_texture_pixels(texture);
	for (int i = 0; i < nu_islands; i++) {
		pops[i] = fgen_create(
			population_size,		// Population size.
			texture->bits_per_block,	// Number of bits.
			1,				// Data element size.
			generation_callback_archipelago,
			calculate_fitness,
			seed,
			fgen_mutation_per_bit_fast,
			fgen_crossover_uniform_per_bit
			);
		fgen_set_parameters(
			pops[i],
			FGEN_ELITIST_SUS,
			FGEN_SUBTRACT_MIN_FITNESS,
			crossover_probability,	// Crossover prob.
			mutation_probability,	// Mutation prob. per bit
			0		// Macro-mutation prob.
			);
		fgen_set_generation_callback_interval(pops[i], nu_generations);
		fgen_set_migration_interval(pops[i], 0);	// No migration.
		fgen_set_migration_probability(pops[i], 0.05);
		pops[i]->user_data = (BlockUserData *)malloc(sizeof(BlockUserData));
		set_user_data((BlockUserData *)pops[i]->user_data, image, texture, 1);

	}
	for (int i = 0; i < nu_islands_second_pass; i++) {
		// Create second pass GAs.
		pops2[i] = fgen_create(
			8,				// Population size.
			texture->bits_per_block,	// Number of bits.
			1,				// Data element size.
			generation_callback_second_pass,
			calculate_fitness,
			seed_second_pass,
			fgen_mutation_per_bit_fast,
			fgen_crossover_uniform_per_bit
			);
		fgen_set_parameters(
			pops2[i],
			FGEN_ELITIST_SUS,
			FGEN_SUBTRACT_MIN_FITNESS,
			0,					// Crossover prob.
			mutation_probability_second_pass,	// Mutation prob. per bit
			0		// Macro-mutation prob.
			);
		fgen_set_generation_callback_interval(pops2[i], nu_generations_second_pass);
		fgen_set_migration_interval(pops2[i], 0);	// No migration.
		fgen_set_migration_probability(pops2[i], 0.01);
		pops2[i]->user_data = (BlockUserData *)malloc(sizeof(BlockUserData));
	}
	if (!option_deterministic)
		fgen_random_seed_with_timer(fgen_get_rng(pops[0]));
//	fgen_random_seed_rng(fgen_get_rng(pops[0]), 0);

	for (int y = 0; y < image->extended_height; y += texture->block_height)
		for (int x = 0; x < image->extended_width; x+= texture->block_width) {
			// Get block flags and prepare the alpha values of the image block for use in the
			// seeding function.
			int block_flags = 0;
			if ((texture->type & (TEXTURE_TYPE_DXTC_BIT | TEXTURE_TYPE_ALPHA_BIT)) ==
			(TEXTURE_TYPE_DXTC_BIT | TEXTURE_TYPE_ALPHA_BIT) || texture->type == TEXTURE_TYPE_ETC2_EAC ||
			texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH || texture->type == TEXTURE_TYPE_BPTC) {
				// Get early parameters for block (whether it is completely opaque or non-opaque,
				// whether it uses only a limited amount of colors), and set alpha pixels.
				block_flags = get_block_flags_rgba8(image, x, y, texture->block_width,
					texture->block_height, alpha_pixels, colors);
			}
			// Calculate pointers to decompressed pixel buffer for block, and block above and left.
			unsigned int *texture_pixels_block, *texture_pixels_above, *texture_pixels_left;
			if (option_perceptive) {
				int bytespp = 4;
				if (texture->type & TEXTURE_TYPE_HALF_FLOAT_BIT)
					bytespp = 8;	// 64-bit pixels
				texture_pixels_block = texture_pixels +
					((y / texture->block_height) * (image->extended_width /
						texture->block_width) + x / texture->block_width) *
					(texture->block_width * texture->block_height * bytespp) / 4;
				if (y == 0)
					texture_pixels_above = NULL;
				else
					texture_pixels_above = texture_pixels_block -
						(image->extended_width / texture->block_width) *
						(texture->block_width * texture->block_height * bytespp) / 4;
				if (x == 0)
					texture_pixels_left = NULL;
				else
					texture_pixels_left = texture_pixels_block -
						(texture->block_width * texture->block_height * bytespp) / 4;
			}
			// Set up the auxilliary information for each population.
 			for (int i = 0; i < nu_islands; i++) {
				BlockUserData *user_data = (BlockUserData *)pops[i]->user_data;
				user_data->x_offset = x;
				user_data->y_offset = y;
				user_data->alpha_pixels = alpha_pixels;
				user_data->colors = colors;
				if (option_perceptive) {
					user_data->texture_pixels = texture_pixels_block;
					user_data->texture_pixels_above = texture_pixels_above;
					user_data->texture_pixels_left = texture_pixels_left;
				}
				set_user_data_mode_flags(i, user_data, block_flags);
				set_user_data_block_flags(user_data, texture, block_flags);
			}
			// Run the genetic algorithm.
			int nu_concurrent_islands = nu_islands;
			if (option_max_threads != -1 && nu_islands > option_max_threads)
				nu_concurrent_islands = option_max_threads;
			for (int i = 0; i < nu_islands;) {
				int n;
				n = nu_concurrent_islands;
				if (i + n > nu_islands)
					n = nu_islands - i;
				if (n == 1)
					fgen_run(pops[i], - 1);
				else
					fgen_run_archipelago_threaded(n, &pops[i], - 1);
				i += n;
			}
			// Report the best solution.
			int best_island;
			FgenIndividual *best = fgen_best_individual_and_island_of_archipelago(nu_islands,
				pops, &best_island);
			bool compress_callback;
			if (isinf(best->fitness))
				compress_callback = true;
			else
				compress_callback = false;
//			if (texture->get_mode_function != NULL) {
//				int mode = texture->get_mode_function(best->bitstring);
//				printf("mode = %d", mode);
//			}
//			printf("\n");
			report_solution(best, (BlockUserData *)pops[best_island]->user_data,
				pops[best_island]->generation, compress_callback);
			if (option_verbose >= 2)
				for (int i = 0; i < nu_islands; i++) {
					printf("Block %d: ", (y / texture->block_height) * (texture->extended_width /
						texture->block_width) + (x / texture->block_width));
					if (texture->type & TEXTURE_TYPE_ETC_BIT) {
						printf("Modes: ");
						int modes_allowed = ((BlockUserData *)pops[i]->user_data)->flags;
						if (modes_allowed & ETC_MODE_ALLOWED_INDIVIDUAL)
							printf("I");
						if (modes_allowed & ETC_MODE_ALLOWED_DIFFERENTIAL)
							printf("D");
						if (modes_allowed & ETC2_MODE_ALLOWED_T)
							printf("T");
						if (modes_allowed & ETC2_MODE_ALLOWED_H)
							printf("H");
						if (modes_allowed & ETC2_MODE_ALLOWED_PLANAR)
							printf("P");
					}
					else if (texture->type == TEXTURE_TYPE_BPTC) {
						printf("Modes: ");
						int modes_allowed = ((BlockUserData *)pops[i]->user_data)->flags;
						for (int j = 0; j < 8; j++)
						if (modes_allowed & (1 << j))
							printf("%d", j);
					}
					FgenIndividual *best = fgen_best_individual_of_population(pops[i]);
					double rmse = sqrt((1.0 / best->fitness) / 16);
					printf(" RMSE per pixel: %lf\n", rmse);
					if (option_verbose >= 3 && rmse >= 1.0) {
						for (int j = 0; j < pops[i]->size; j++) {
							FgenIndividual *ind = pops[i]->ind[j];
							printf("  Individual %d: ", j);
							if (texture->get_mode_function != NULL) {
								int mode = texture->get_mode_function(ind->bitstring);
								printf("Mode: %d ", mode);
							}
							printf("RMSE per pixel: %lf\n",
								sqrt((1.0 / ind->fitness) / 16));
						}
					}
				}
			if (!isinf(best->fitness)) {
				// Copy block user_data from first pass.
				for (int i = 0; i < nu_islands_second_pass; i++) {
					*(BlockUserData *)pops2[i]->user_data =
						*(BlockUserData *)pops[best_island]->user_data;
					// For ETC1, ETC2, BPTC and BPTC_FLOAT, preserve the mode of the compressed
					// block during the second pass.
					if ((texture->type & TEXTURE_TYPE_ETC_BIT) ||
					texture->type == TEXTURE_TYPE_BPTC ||
					texture->type == TEXTURE_TYPE_BPTC_FLOAT ||
					texture->type == TEXTURE_TYPE_BPTC_SIGNED_FLOAT) {
						int mode = texture->get_mode_function(best->bitstring);
						((BlockUserData *)pops2[i]->user_data)->flags = (1 << mode) |
							 ENCODE_BIT;
						set_user_data_block_flags((BlockUserData *)pops2[i]->user_data,
							texture, block_flags);
					}
				}
				// Run the second pass.
				nu_concurrent_islands = nu_islands_second_pass;
				if (option_max_threads != -1 && nu_islands_second_pass > option_max_threads)
					nu_concurrent_islands = option_max_threads;
				for (int i = 0; i < nu_islands_second_pass;) {
					int n;
					n = nu_concurrent_islands;
					if (i + n > nu_islands_second_pass)
						n = nu_islands_second_pass - i;
					if (n == 1)
						fgen_run(pops2[i], - 1);
					else
						fgen_run_archipelago_threaded(n, &pops2[i], - 1);
					i += n;
				}
				best = fgen_best_individual_and_island_of_archipelago(nu_islands_second_pass, pops2,
					&best_island);
				report_solution(best, (BlockUserData *)pops2[best_island]->user_data,
					pops2[best_island]->generation, true);
			}
			if (((BlockUserData *)pops[0]->user_data)->stop_signalled)
				goto end;
		}
end :
	for (int i = 0; i < nu_islands; i++) {
		free(pops[i]->user_data);
		fgen_destroy(pops[i]);
	}
	for (int i = 0; i < nu_islands_second_pass; i++) {
		free(pops2[i]->user_data);
		fgen_destroy(pops2[i]);
	}
	if (option_perceptive)
		free(texture_pixels);
}

// Compress multiple blocks concurrently. Used by --ultra setting. Note that larger population size used in this case.

static void compress_multiple_blocks_concurrently(Image *image, Texture *texture) {
	unsigned char *alpha_pixels = (unsigned char *)alloca(texture->block_width * texture->block_height * nu_islands);
	if (option_generations != - 1)
		nu_generations = option_generations;
	if (option_islands != - 1)
		nu_islands = option_islands;
	FgenPopulation **pops = (FgenPopulation **)alloca(sizeof(FgenPopulation *) * nu_islands);
	if (!option_quiet)
		printf("Running single GA for each pixel block, %d concurrently, population size %d, "
			"%d generations, non-perceptive quality strategy.\n",
			nu_islands, population_size, nu_generations);
	for (int i = 0; i < nu_islands; i++) {
		pops[i] = fgen_create(
			population_size,		// Population size.
			texture->bits_per_block,	// Number of bits.
			1,				// Data element size.
			generation_callback,
			calculate_fitness,
			seed2,
			fgen_mutation_per_bit_fast,
			fgen_crossover_uniform_per_bit
			);
		fgen_set_parameters(
			pops[i],
			FGEN_ELITIST_SUS,
			FGEN_SUBTRACT_MIN_FITNESS,
			crossover_probability,	// Crossover prob.
			mutation_probability,	// Mutation prob. per bit
			0		// Macro-mutation prob.
			);
		fgen_set_generation_callback_interval(pops[i], nu_generations);
		fgen_set_migration_interval(pops[i], 0);	// No migration.
		fgen_set_migration_probability(pops[i], 0.01);
		pops[i]->user_data = (BlockUserData *)malloc(sizeof(BlockUserData));
		set_user_data((BlockUserData *)pops[i]->user_data, image, texture, 1);
		if (texture->type == TEXTURE_TYPE_ETC2_RGB8 || texture->type == TEXTURE_TYPE_ETC2_EAC)
			if (option_allowed_modes_etc2 != - 1)
				((BlockUserData *)pops[i]->user_data)->flags =
					option_allowed_modes_etc2 | ENCODE_BIT;
	}
	if (!option_deterministic)
		fgen_random_seed_with_timer(fgen_get_rng(pops[0]));
	// Calculate the number of full archipelagos of nu_islands that fit on each each row.
	int nu_full_archipelagos_per_row = image->extended_width / (nu_islands * texture->block_width);
	int x_marker = nu_full_archipelagos_per_row * nu_islands * texture->block_width;
	for (int y = 0; y < image->extended_height; y += texture->block_height) {
		// Handle nu_islands horizontal blocks at a time.
		for (int x = 0; x < x_marker; x+= nu_islands * texture->block_width) {
			for (int i = 0; i < nu_islands; i++) {
				BlockUserData *user_data = (BlockUserData *)pops[i]->user_data;
				user_data->x_offset = x + i * texture->block_width;
				user_data->y_offset = y;
				// For 1-bit alpha texture, prepare the alpha values of the image block for use in the
				// seeding function.
				if (texture->type == TEXTURE_TYPE_DXT3 ||
				texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH) {
					set_alpha_pixels(image, x + i * texture->block_width, y, texture->block_width,
						texture->block_height, &alpha_pixels[i * 16]);
					user_data->alpha_pixels = &alpha_pixels[i * 16];
				}
			}
			// Run with a seperate population on each island for different blocks.
			int nu_concurrent_islands = nu_islands;
			if (option_max_threads != -1 && nu_islands > option_max_threads)
				nu_concurrent_islands = option_max_threads;
			for (int i = 0; i < nu_islands;) {
				int n;
				n = nu_concurrent_islands;
				if (i + n > nu_islands)
					n = nu_islands - i;
				if (n == 1)
					fgen_run(pops[i], nu_generations_second_pass);
				else
					fgen_run_archipelago_threaded(n, &pops[i], nu_generations_second_pass);
				i += n;
			}
			for (int i = 0; i < nu_islands; i++) {
				FgenIndividual *best = fgen_best_individual_of_population(pops[i]);
				report_solution(best, (BlockUserData *)pops[i]->user_data, pops[i]->generation, true);
				if (((BlockUserData *)pops[i]->user_data)->stop_signalled)
					goto end;
			}
		}
		// Handle the remaining blocks on the row.
		int nu_blocks_left = (image->extended_width - x_marker + texture->block_width - 1) / texture->block_width;
		int x = x_marker;
		for (int i = 0; i < nu_blocks_left; i++) {
			BlockUserData *user_data = (BlockUserData *)pops[i]->user_data;
			user_data->x_offset = x + i * texture->block_width;
			user_data->y_offset = y;
			// For 1-bit alpha texture, prepare the alpha values of the image block for use in the
			// seeding function.
			if (texture->type == TEXTURE_TYPE_DXT3 || texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH) {
				set_alpha_pixels(image, x + i * texture->block_width, y, texture->block_width,
					texture->block_height, &alpha_pixels[i * texture->block_width *
					texture->block_height]);
				user_data->alpha_pixels = &alpha_pixels[i * texture->block_width * texture->block_height];
			}
		}
		if (nu_blocks_left > 0) {
			if (nu_blocks_left > 1)
				fgen_run_archipelago_threaded(nu_blocks_left, pops, - 1);
			else
				fgen_run(pops[0], - 1);
		}
		for (int i = 0; i < nu_blocks_left; i++) {
			FgenIndividual *best = fgen_best_individual_of_population(pops[i]);
			report_solution(best, (BlockUserData *)pops[i]->user_data, pops[i]->generation, true);
			if (((BlockUserData *)pops[i]->user_data)->stop_signalled)
				goto end;
		}
	}
end :
	for (int i = 0; i < nu_islands; i++) {
		free(pops[i]->user_data);
		fgen_destroy(pops[i]);
	}
}

// Perform the second-pass, processing multiple blocks concurrently, for ultra-class quality level.
// Expectes mutation_probability_second_pass and nu_generations_second_pass to be predefined.

static void compress_multiple_blocks_concurrently_second_pass(Image *image, Texture *texture) {
	int nu_islands_second_pass = 8;
	unsigned char *alpha_pixels = (unsigned char *)alloca(texture->block_width * texture->block_height *
		nu_islands_second_pass);
	FgenPopulation **pops = (FgenPopulation **)alloca(sizeof(FgenPopulation *) * nu_islands_second_pass);
	if (!option_quiet)
		printf("Running second pass GA for each pixel block, %d concurrently, population size 8, "
			"%d generations.\n", nu_islands_second_pass, nu_generations_second_pass);
	for (int i = 0; i < nu_islands_second_pass; i++) {
		pops[i] = fgen_create(
			8,				// Population size.
			texture->bits_per_block,	// Number of bits.
			1,				// Data element size.
			generation_callback_second_pass,
			calculate_fitness,
			seed_second_pass,
			fgen_mutation_per_bit_fast,
			fgen_crossover_uniform_per_bit
			);
		fgen_set_parameters(
			pops[i],
			FGEN_ELITIST_SUS,
			FGEN_SUBTRACT_MIN_FITNESS,
			0,					// Crossover prob.
			mutation_probability_second_pass,	// Mutation prob. per bit
			0		// Macro-mutation prob.
			);
		fgen_set_generation_callback_interval(pops[i], nu_generations_second_pass);
		fgen_set_migration_interval(pops[i], 0);	// No migration.
		fgen_set_migration_probability(pops[i], 0.01);
//		fgen_set_number_of_elites(pops[i], population_size / 2);
		pops[i]->user_data = (BlockUserData *)malloc(sizeof(BlockUserData));
		set_user_data((BlockUserData *)pops[i]->user_data, image, texture, 2);
		if (texture->type == TEXTURE_TYPE_ETC2_RGB8 || texture->type == TEXTURE_TYPE_ETC2_EAC)
			if (option_allowed_modes_etc2 != - 1)
				((BlockUserData *)pops[i]->user_data)->flags =
					option_allowed_modes_etc2 | ENCODE_BIT;
	}
	if (!option_deterministic)
		fgen_random_seed_with_timer(fgen_get_rng(pops[0]));
	// Calculate the number of full archipelagos of nu_islands_second_pass that fit on each each row.
	int nu_full_archipelagos_per_row = image->extended_width / (nu_islands * texture->block_width);
	int x_marker = nu_full_archipelagos_per_row * nu_islands_second_pass * texture->block_width;
	for (int y = 0; y < image->extended_height; y += texture->block_height) {
		// Handle nu_islands_second_pass horizontal blocks at a time.
		for (int x = 0; x < x_marker; x+= nu_islands_second_pass * texture->block_width) {
			for (int i = 0; i < nu_islands_second_pass; i++) {
				BlockUserData *user_data = (BlockUserData *)pops[i]->user_data;
				user_data->x_offset = x + i * texture->block_width;
				user_data->y_offset = y;
				// For 1-bit alpha texture, prepare the alpha values of the image block for use in the
				// seeding function.
				if (texture->type == TEXTURE_TYPE_DXT3 ||
				texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH) {
					set_alpha_pixels(image, x + i * texture->block_width, y, texture->block_width,
						texture->block_height, &alpha_pixels[i * 16]);
					user_data->alpha_pixels = &alpha_pixels[i * 16];
				}
			}
			// Run with a seperate population on each island for different blocks.
			int nu_concurrent_islands = nu_islands_second_pass;
			if (option_max_threads != -1 && nu_islands_second_pass > option_max_threads)
				nu_concurrent_islands = option_max_threads;
			for (int i = 0; i < nu_islands_second_pass;) {
				int n;
				n = nu_concurrent_islands;
				if (i + n > nu_islands_second_pass)
					n = nu_islands_second_pass - i;
				fgen_run_archipelago_threaded(n, &pops[i], nu_generations_second_pass);
				i += n;
			}
			for (int i = 0; i < nu_islands_second_pass; i++) {
				FgenIndividual *best = fgen_best_individual_of_population(pops[i]);
				report_solution(best, (BlockUserData *)pops[i]->user_data, pops[i]->generation, true);
				if (((BlockUserData *)pops[i]->user_data)->stop_signalled)
					goto end;
			}
		}
		// Handle the remaining blocks on the row.
		int nu_blocks_left = (image->extended_width - x_marker + texture->block_width - 1) / texture->block_width;
		int x = x_marker;
		for (int i = 0; i < nu_blocks_left; i++) {
			BlockUserData *user_data = (BlockUserData *)pops[i]->user_data;
			user_data->x_offset = x + i * texture->block_width;
			user_data->y_offset = y;
			// For 1-bit alpha texture, prepare the alpha values of the image block for use in the
			// seeding function.
			if (texture->type == TEXTURE_TYPE_DXT3 || texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH) {
				set_alpha_pixels(image, x + i * texture->block_width, y, texture->block_width,
					texture->block_height, &alpha_pixels[i * texture->block_width *
					texture->block_height]);
				user_data->alpha_pixels = &alpha_pixels[i * texture->block_width * texture->block_height];
			}
		}
		if (nu_blocks_left > 0) {
			if (nu_blocks_left > 1)
				fgen_run_archipelago_threaded(nu_blocks_left, pops, - 1);
			else
				fgen_run(pops[0], - 1);
		}
		for (int i = 0; i < nu_blocks_left; i++) {
			FgenIndividual *best = fgen_best_individual_of_population(pops[i]);
			report_solution(best, (BlockUserData *)pops[i]->user_data, pops[i]->generation, true);
			if (((BlockUserData *)pops[i]->user_data)->stop_signalled)
				goto end;
		}
	}
end :
	for (int i = 0; i < nu_islands_second_pass; i++) {
		free(pops[i]->user_data);
		fgen_destroy(pops[i]);
	}
}

// Copy the alpha pixel values of a block into an array.

static void set_alpha_pixels(Image *image, int x, int y, int w, int h, unsigned char *alpha_pixels) {
	for (int by = 0; by < h; by++)
		for (int bx = 0; bx < w; bx++)
			if (y + by < image->height && x + bx < image->width)
				alpha_pixels[by * w + bx] = pixel_get_a(
					image->pixels[(y + by) * image->extended_width + x + bx]);
			else
				// If the pixel falls on the border, put 0xFF.
				alpha_pixels[by * w + bx] = 0xFF;
}

// Determine block flags for RGBA8 block (whether it is completely opaque or non-opaque,
// whether it uses only a limited amount of colors) and copy alpha pixel values into an array.

static int get_block_flags_rgba8(Image *image, int x, int y, int w, int h, unsigned char *alpha_pixels,
unsigned int *colors) {
	int block_flags;
	if (image->alpha_bits > 0)
		block_flags = BLOCK_FLAG_OPAQUE | BLOCK_FLAG_NON_OPAQUE | BLOCK_FLAG_PUNCHTHROUGH |
			BLOCK_FLAG_TWO_COLORS;
	else
		block_flags = BLOCK_FLAG_OPAQUE | BLOCK_FLAG_PUNCHTHROUGH | BLOCK_FLAG_TWO_COLORS;
	int color0 = - 1;
	int color1 = - 1;
	for (int by = 0; by < h; by++)
		for (int bx = 0; bx < w; bx++) {
			if (image->alpha_bits > 0) {
				int alpha;
				if (y + by < image->height && x + bx < image->width) {
					unsigned int pixel = image->pixels[(y + by) * image->extended_width + x + bx];
					alpha = pixel_get_a(pixel);
				}
				else {
					// If the pixel falls on the border, conform to the current findings.
					if (block_flags & BLOCK_FLAG_OPAQUE)
						alpha = 0xFF;
					else
						alpha = 0x00;
				}
				if (alpha == 0xFF)
					block_flags &= (~BLOCK_FLAG_NON_OPAQUE);
				else {
					block_flags &= (~BLOCK_FLAG_OPAQUE);
					if (alpha != 0x00)
						block_flags &= (~BLOCK_FLAG_PUNCHTHROUGH);
				}
				alpha_pixels[by * w + bx] = alpha;
			}
			if (y + by < image->height && x + bx < image->width) {
				unsigned int pixel = image->pixels[(y + by) * image->extended_width + x + bx];
				int rgb = pixel_get_r(pixel) + (pixel_get_g(pixel) << 8) + (pixel_get_b(pixel) << 16);
				if (color0 == - 1)
					color0 = rgb;
				else if (rgb != color0)
					if (color1 == - 1)
						color1 = rgb;
					else if (rgb != color1)
						block_flags &= (~BLOCK_FLAG_TWO_COLORS);
			}
		}
	if (block_flags & BLOCK_FLAG_TWO_COLORS) {
		colors[0] = color0;
		if (color1 == - 1)
			colors[1] = colors[0];
		else
			colors[1] = color1;
	}
	return block_flags;
}


// Optimize the alpha component of 1-bit alpha textures for the whole image.

static void optimize_alpha(Image *image, Texture *texture) {
	if (!(texture->type == TEXTURE_TYPE_ETC2_PUNCHTHROUGH || texture->type == TEXTURE_TYPE_DXT3))
		return;
	unsigned char alpha_pixels[16];
	for (int y = 0; y < image->extended_height; y += texture->block_height)
		for (int x = 0; x < image->extended_width; x+= texture->block_width) {
			set_alpha_pixels(image, x, y, texture->block_width, texture->block_height, alpha_pixels);
			int compressed_block_index = (y / texture->block_height) *
				(texture->extended_width / texture->block_width) + x / texture->block_width;
			unsigned char *bitstring;
			switch (texture->type) {
			case TEXTURE_TYPE_ETC2_PUNCHTHROUGH :
				bitstring = (unsigned char *)&texture->pixels[compressed_block_index * 2];
				optimize_block_alpha_etc2_punchthrough(bitstring, alpha_pixels);
				break;
			case TEXTURE_TYPE_DXT3 :
				bitstring = (unsigned char *)&texture->pixels[compressed_block_index * 4];
				optimize_block_alpha_dxt3(bitstring, alpha_pixels);
				break;
			}
		}
}

// Calculate the RMSE threshold for adaptive block optimization.

static double get_rmse_threshold(Texture *texture, int level, Image *source_image) {
	// Convert level to fixed speed grade for legacy code.
	// A better implementation of threshold determination would emperically determine the threshold
	// by compressing a number of sample blocks from the image.
	int speed;
	if (level < COMPRESSION_LEVEL_CLASS_1)
		speed = SPEED_ULTRA;
	else if (level < SPEED_MEDIUM)
		speed = SPEED_FAST;
	else if (level < SPEED_SLOW)
		speed = SPEED_MEDIUM;
	else
		speed = SPEED_SLOW;
	double threshold;
	if (!(texture->type & TEXTURE_TYPE_128BIT_BIT)) {
		// 64-bit texture formats.
		if (texture->type & TEXTURE_TYPE_ETC_BIT)
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 13.0;
				break;
			case SPEED_FAST :
				threshold = 11.5;
				break;
			case SPEED_MEDIUM :
				threshold = 11.0;
				break;
			case SPEED_SLOW :
				threshold = 10.5;
				break;
			}
		else
		if (texture->type & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT)
			// R11_EAC and SIGNED_RGTC1
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 1100;
				break;
			case SPEED_FAST :
				threshold = 900;
				break;
			case SPEED_MEDIUM :
				threshold = 800;
				break;
			case SPEED_SLOW :
				threshold = 700;
				break;
			}
		else
		if (texture->info->nu_components == 1 && !(texture->type & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT))
			// RGTC1, one 8-bit component.
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 6.0;
				break;
			case SPEED_FAST :
				threshold = 5.0;
				break;
			case SPEED_MEDIUM :
				threshold = 4.0;
				break;
			case SPEED_SLOW :
				threshold = 3.5;
				break;
			}
		else	// DXT1/DXT1A
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 11.5;
				break;
			case SPEED_FAST :
				threshold = 9.0;
				break;
			case SPEED_MEDIUM :
				threshold = 8.5;
				break;
			case SPEED_SLOW :
				threshold = 8.0;
				break;
			}
	}
	else {
		// 128-bit texture formats.
		if (texture->type == TEXTURE_TYPE_BPTC)
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 10.5;
				break;
			case SPEED_FAST :
				threshold = 7.0;
				break;
			case SPEED_MEDIUM :
				threshold = 6.0;
				break;
			case SPEED_SLOW :
				threshold = 5.5;
				break;
			}
		else
		if (texture->type & TEXTURE_TYPE_HALF_FLOAT_BIT)
			if (option_hdr)
				switch (speed) {
				case SPEED_ULTRA :
					threshold = 0.35;
					break;
				case SPEED_FAST :
					threshold = 0.20;
					break;
				case SPEED_MEDIUM :
					threshold = 0.15;
					break;
				case SPEED_SLOW :
					threshold = 0.10;
					break;
				}
			else
				switch (speed) {
				case SPEED_ULTRA :
					threshold = 0.060;
					break;
				case SPEED_FAST :
					threshold = 0.050;
					break;
				case SPEED_MEDIUM :
					threshold = 0.040;
					break;
				case SPEED_SLOW :
					threshold = 0.035;
					break;
				}
		else
		if (texture->type & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT)
			// RG11_EAC, SIGNED_RGTC2
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 2000;
				break;
			case SPEED_FAST :
				threshold = 1400;
				break;
			case SPEED_MEDIUM :
				threshold = 1200;
				break;
			case SPEED_SLOW :
				threshold = 950;
				break;
			}
		else
		if (texture->info->nu_components == 2 && !(texture->type & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT))
			// RGTC2, two 16-bit components.
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 13.0;
				break;
			case SPEED_FAST :
				threshold = 8.0;
				break;
			case SPEED_MEDIUM :
				threshold = 7.0;
				break;
			case SPEED_SLOW :
				threshold = 6.5;
				break;
			}
		else	// 128-bit alpha formats ETC2_EAC, DXT3, DXT5
			switch (speed) {
			case SPEED_ULTRA :
				threshold = 11.5;
				break;
			case SPEED_FAST :
				threshold = 9.0;
				break;
			case SPEED_MEDIUM :
				threshold = 8.5;
				break;
			case SPEED_SLOW :
				threshold = 8.0;
				break;
			}
	}
	if (source_image->is_half_float && !(texture->type & TEXTURE_TYPE_HALF_FLOAT_BIT))
		// If the source image is in half-float format, but the texture type is regular RGB(A)8,
		// correct the threshold to half-float comparison values.
		threshold *= 1.0 / 255.0;
	else
	if (source_image->bits_per_component == 16 && !(texture->type & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT))
		threshold *= 256.0;
	// Adjust threshold upward for perceptive quality mode.
	if (option_perceptive && option_compression_level >= COMPRESSION_LEVEL_CLASS_1
	&& texture->perceptive_comparison_function != NULL)
		threshold *= 1.2;
	return threshold;
}


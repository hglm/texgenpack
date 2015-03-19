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
#include <ctype.h>
#include <malloc.h>
#include "texgenpack.h"
#include "decode.h"
#ifndef __GNUC__
#include "strcasecmp.h"
#endif


// Local functions in this file.

static int file_exists(const char *filename);
static int determine_filename_type(const char *filename);
static void compare_files();
static void decompress();
static void compress();
static void calibrate();

// Variables reflecting command-line options.

int command;
int option_verbose = 0;
int option_max_threads = - 1;
char *source_filename;
char *dest_filename;
int source_filetype;
int dest_filetype;
int option_orientation = 0;
int option_texture_format = - 1;
int option_compression_level = SPEED_FAST;
int option_progress = 0;
int option_modal_etc2 = 1;
int option_allowed_modes_etc2 = - 1;
int option_mipmaps = 0;
int option_generations = - 1;
int option_islands = - 1;
int option_generations_second_pass = - 1;
int option_islands_second_pass = - 1;
int option_flip_vertical = 0;
int option_quiet = 0;
int option_block_width = 4;
int option_block_height = 4;
int option_half_float = 0;
int option_hdr = 0;
int option_perceptive = 1;

// Other option variables that are not actually set by command-line options.

int option_deterministic = 0;

static char *instructions1 =
"texgenpack v0.9 -- Texture conversion and compression using a genetic algorithm.\n"
"Usage: texgenpack <command> <options> <source filename> <destination filename>.\n"
"\n"
"Commands:\n";

#define NU_COMMANDS 4

static const char *commands[NU_COMMANDS] = {
	"--compress", "--decompress", "--compare", "--calibrate" };

#define NU_OPTIONS 24

enum {
	OPTION_COMPRESSION_LEVEL = 0,
	OPTION_ULTRA,
	OPTION_FAST,
	OPTION_MEDIUM,
	OPTION_SLOW,
	OPTION_NON_PERCEPTIVE,
	OPTION_TEXTURE_FORMAT,
	OPTION_HALF_FLOAT,
	OPTION_HDR,
	OPTION_MIPMAPS,
	OPTION_ORIENTATION,
	OPTION_FLIP_VERTICAL,
	OPTION_MODAL_ETC2,
	OPTION_ALLOWED_MODES,
	OPTION_MAXTHREADS,
	OPTION_GENERATIONS,
	OPTION_ISLANDS,
	OPTION_GENERATIONS_SECOND_PASS,
	OPTION_ISLANDS_SECOND_PASS,
	OPTION_PROGRESS,
	OPTION_VERBOSE,
	OPTION_VERY_VERBOSE,
	OPTION_QUIET,
	OPTION_VERBOSITY,
};

static const char *options[NU_OPTIONS] = {
	"--level", "--ultra", "--fast", "--medium", "--slow",
	"--non-perceptive",
	"--format",
	"--half-float", "--hdr",
	"--mipmaps",
	"--orientation", "--flip-vertical",
	"--modal", "--allowed-modes",
	"--maxthreads", "--generations", "--islands", "--generations-second-pass", "--islands-second-pass",
	"--progress", "--verbose", "--very-verbose", "--quiet", "--verbosity"
};

static const char *option_argument[NU_OPTIONS] = {
	"<number>", "", "", "", "",
	"",
	"<format>",
	"", "",
	"",
	"<direction>", "",
	"", "<modes>",
	"<number>", "<number>", "<number>", "<number>", "<number>",
	"", "", "", "", "<number>",
};

static const char *option_description[NU_OPTIONS] = {
	"Compression level (0 to 50, 0 = fastest, 50 = slowest/best compression).",
	"Ultra fast compression optimizing different blocks concurrently.",
	"Fast compression method (level = 8) (default).",
	"Medium compression method (level = 16).",
	"Slow compression method (level = 32).",
	"Use non-perceptive quality strategy (defaalt for --ultra setting).",
	"Texture format. One of the following: ",
	"Convert regular images to half-float format before compression.",
	"The half-float format contains a HDR texture that is not normalized. This affects compression.",
	"Generate mipmaps when compressing an image into a texture. When decompressing to an image file, generate a "
	"sequence of image files named filename-mipmap*.png holding the mipmap levels.",
	"Write orientation key when writing .ktx file. Direction must be up or down.",
	"Flip the texture vertically during the conversion process.",
	"Use a different technique for ETC2 compression with islands tied to specific ETC2 modes.",
	"Specify the ETC2 modes to use. Argument is a string containing a subset of the letters IDTHP.",
	"Specify the maximum number of threads to use (not recommended; uses --islands option to control threading level).",
	"Set the number of generations for the genetic algorithm per block (adjusted from compression level).",
	"Set the number of concurrent islands for the genetic algorithm (adjusted from compression level).",
	"Set the number of generations for the second pass of the genetic algorithm per block (adjusted from "
	"compression level).",
	"Set the number of concurrent islands for the second pass of the genetic algorithm (adjusted from "
	"compression level).",
	"Display a percentage progress indicator.",
	"Be verbose (information for each block).",
	"Be very verbose (more information for each block).",
	"Don't print anything.",
	"Set verbosity level (0 to 3), 0 = quiet, 1 = verbose, 2 = very verbose.",
};

int main(int argc, char **argv) {
	if (argc <= 2) {
		printf("%s", instructions1);
		for (int i = 0; i < NU_COMMANDS; i++)
			printf("%s\n", commands[i]);
		printf("\nOptions:\n");
		for (int i = 0; i < NU_OPTIONS; i++) {
			printf("%s %s\n        %s\n", options[i], option_argument[i], option_description[i]);
			if (i == OPTION_TEXTURE_FORMAT) {
				int n = get_number_of_texture_formats();
				for (int j = 0; j < n - 1; j++) {
					printf("%s, ", get_texture_format_index_text(j, 0));
					const char *text2 = get_texture_format_index_text(j, 1);
					if (strlen(text2) > 0)
						printf("%s, ", text2);
				}
				printf("%s\n", get_texture_format_index_text(n - 1, 0));
			}
		}
		exit(0);
	}

	command = - 1;
	for (int i = 0; i < NU_COMMANDS; i++)
		if (strcmp(argv[1], commands[i]) == 0) {
			command = i;
			break;
		}
	if (command == -1) {
		printf("Error -- no valid command found as first argument. Valid command are --compress,\n"
			"--decompress and --compare. Run with no arguments for help.\n");
		exit(1);
	}

	int i = 2;
	for (; i < argc;) {
		int option = - 1;
		for (int j = 0; j < NU_OPTIONS; j++)
			if (strcmp(argv[i], options[j]) == 0) {
				option = j;	
				break;
			}
		if (option == - 1)
			break;
		// Single argument options.
		switch (option) {
		case OPTION_VERBOSE :
			option_verbose = 1;
			i++;
			continue;
		case OPTION_VERY_VERBOSE :
			option_verbose = 2;
			i++;
			continue;
		case OPTION_FAST :
			option_compression_level = SPEED_FAST;
			i++;
			continue;
		case OPTION_MEDIUM :
			option_compression_level = SPEED_MEDIUM;
			i++;
			continue;
		case OPTION_SLOW :
			option_compression_level = SPEED_SLOW;
			i++;
			continue;
		case OPTION_NON_PERCEPTIVE :
			option_perceptive = 0;
			i++;
			continue;
		case OPTION_PROGRESS :
			option_progress = 1;
			i++;
			continue;
		case OPTION_MODAL_ETC2 :
			option_modal_etc2 = 1;
			i++;
			continue;
		case OPTION_ULTRA :
			option_compression_level = SPEED_ULTRA;
			i++;
			continue;
		case OPTION_MIPMAPS :
			option_mipmaps = 1;
			i++;
			continue;
		case OPTION_FLIP_VERTICAL :
			option_flip_vertical = 1;
			i++;
			continue;
		case OPTION_QUIET :
			option_quiet = 1;
			i++;
			continue;
		case OPTION_HALF_FLOAT :
			option_half_float = 1;
			i++;
			continue;
		case OPTION_HDR :
			option_hdr = 1;
			i++;
			continue;
		}
		// Two argument options.
		if (i + 1 >= argc) {
			printf("Error -- no filenames specified.\n");
			exit(1);
		}
		int value;
		switch (option) {
		case OPTION_MAXTHREADS :
			value = atoi(argv[i + 1]);
			if (value < 1 || value > 256) {
				printf("Error -- invalid number of max threads specified.\n");
				exit(1);
			}
			option_max_threads = value;
			i += 2;
			break;
		case OPTION_ORIENTATION :
			if (strcasecmp(argv[i + 1], "down") == 0)
				option_orientation = ORIENTATION_DOWN;
			else
			if (strcasecmp(argv[i + 1], "up") == 0)
				option_orientation = ORIENTATION_UP;
			else {
				printf("Error -- orientation should be down or up.\n");
				exit(1);
			}
			i += 2;
			break;
		case OPTION_TEXTURE_FORMAT :
			{
			TextureInfo *info;
			info = match_texture_description(argv[i + 1]);
			if (info != NULL)
				option_texture_format = info->type;
			else {
				printf("Error -- unknown texture format specified.\n");
				exit(1);
			}
			i += 2;
			break;
			}
		case OPTION_ALLOWED_MODES :
			option_allowed_modes_etc2 = 0;
			for (int j = 0; j < strlen(argv[i + 1]); j++)
				switch (argv[i + 1][j]) {
				case 'I' :
					option_allowed_modes_etc2 |= ETC_MODE_ALLOWED_INDIVIDUAL;
					break;
				case 'D' :
					option_allowed_modes_etc2 |= ETC_MODE_ALLOWED_DIFFERENTIAL;
					break;
				case 'T' :
					option_allowed_modes_etc2 |= ETC2_MODE_ALLOWED_T;
					break;
				case 'H' :
					option_allowed_modes_etc2 |= ETC2_MODE_ALLOWED_H;
					break;
				case 'P' :
					option_allowed_modes_etc2 |= ETC2_MODE_ALLOWED_PLANAR;
					break;
				default :
					printf("Error -- invalid character in modes arguments for --allowed-modes.\n");
					exit(1);
				}
			i += 2;
			break;
		case OPTION_GENERATIONS :
			value = atoi(argv[i + 1]);
			if (value < 1 || value > 10000) {
				printf("Error -- invalid number of generations specified (range 1-10000).\n");
				exit(1);
			}
			option_generations = value;
			i += 2;
			break;
		case OPTION_ISLANDS :
			value = atoi(argv[i + 1]);
			if (value < 1 || value > 64) {
				printf("Error -- invalid number of islands specified (range 1-64).\n");
				exit(1);
			}
			option_islands = value;
			i += 2;
			break;
		case OPTION_GENERATIONS_SECOND_PASS :
			value = atoi(argv[i + 1]);
			if (value < 1 || value > 100000) {
				printf("Error -- invalid number of generations specified (range 1-100000).\n");
				exit(1);
			}
			option_generations_second_pass = value;
			i += 2;
			break;
		case OPTION_ISLANDS_SECOND_PASS :
			value = atoi(argv[i + 1]);
			if (value < 1 || value > 64) {
				printf("Error -- invalid number of islands specified (range 1-64).\n");
				exit(1);
			}
			option_islands_second_pass = value;
			i += 2;
			break;
		case OPTION_COMPRESSION_LEVEL :
			value = atoi(argv[i + 1]);
			if (value < 0 || value > 50) {
				printf("Error -- invalid compression level (range 0-50).\n");
				exit(1);
			}
			option_compression_level = value;
			i += 2;
			break;
		case OPTION_VERBOSITY :
			value = atoi(argv[i + 1]);
			if (value < 0 || value > 3) {
				printf("Error -- invalid verbosity (range 0-3).\n");
				exit(1);
			}
			option_verbose = value;
			i += 2;
			break;
#if 0
		case OPTION_BLOCK_SIZE :
			{
			int w, h;
			int r = sscanf(argv[i + 1], "%dx%d", &w, &h);
			if (r < 2) {
				printf("Error -- expected valid block size, such as 4x4, 5x4 etc.\n");
				exit(1);
			}
			int j = match_astc_block_size(w, h);
			if (j == - 1) {
				printf("Error -- ASTC block size not supported.\n");
				exit(1);
			}
			option_block_width = w;
			option_block_height = h;
			i += 2;
			break;
			}
#endif
		}
	}

	if (i >= argc - 1) {
		printf("Error -- expected two filenames at the end of the command line.\n");
		exit(1);
	}
	if ((option_block_width != 4 || option_block_height != 4) && !(option_texture_format & TEXTURE_TYPE_ASTC_BIT)) {
		printf("Error -- block size option only allowed when compressing to ASTC format.\n");
		exit(1);
	}

	source_filename = argv[i];
	dest_filename = argv[i + 1];
	source_filetype = determine_filename_type(source_filename);
	if (source_filetype == FILE_TYPE_UNDEFINED) {
		printf("Error -- unknown file %s (no known extension found).\n", source_filename);
		exit(1);
	}
	dest_filetype = determine_filename_type(dest_filename);
	if (dest_filetype == FILE_TYPE_UNDEFINED) {
		printf("Error -- unknown file %s (no known extension found).\n", dest_filename);
		exit(1);
	}
	if (!file_exists(source_filename)) {
		printf("Error -- source file doesn't exist or is unreadable.\n");
		exit(1);
	}
	if (strcasecmp(source_filename, dest_filename) == 0) {
		printf("Error -- source filename and destination filename are identical.\n");
		exit(1);
	}
	if (command == COMMAND_COMPARE) {
		if (!file_exists(dest_filename)) {
			printf("Error -- second filename to compare to doesn't exist or is unreadable.\n");
			exit(1);
		}
		compare_files();
		exit(0);
	}
	if (command == COMMAND_DECOMPRESS) {
		if ((source_filetype & FILE_TYPE_TEXTURE_BIT) == 0) {
			printf("Error -- expected texture file as first argument.\n");
			exit(1);
		}
//		if ((dest_filetype & FILE_TYPE_IMAGE_BIT) == 0) {
//			printf("Error -- expected image file as second argument.\n");
//			exit(1);
//		}
		decompress();
		exit(0);
	}
	if (command == COMMAND_COMPRESS) {
//		if ((source_filetype & FILE_TYPE_IMAGE_BIT) == 0) {
//			printf("Error -- expected image file as first argument.\n");
//			exit(1);
//		}
		if ((dest_filetype & FILE_TYPE_TEXTURE_BIT) == 0) {
			printf("Error -- expected texture file as second argument.\n");
			exit(1);
		}
		if (option_mipmaps && (dest_filetype & FILE_TYPE_MIPMAPS_BIT) == 0) {
			printf("Error -- destination file type cannot hold multiple mipmap levels.\n");
			exit(1);
		}
		compress();
	}
	if (command == COMMAND_CALIBRATE) {
		calibrate();
	}
}

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

static int determine_filename_type(const char *filename) {
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

static void compare_files() {
	Image source_image, dest_image;
	if (!option_quiet)
		printf("Comparing %s to %s.\n", source_filename, dest_filename);
	load_image(source_filename, source_filetype, &source_image);
	if (option_flip_vertical)
		flip_image_vertical(&source_image);
	load_image(dest_filename, dest_filetype, &dest_image);
	if (source_image.width != dest_image.width || source_image.height != dest_image.height) {
		printf("Error -- image files to compare must be have same dimensions.\n");
		exit(1);
	}
	compare_images(&source_image, &dest_image);
}

static void decompress() {
	if (dest_filetype & FILE_TYPE_TEXTURE_BIT) {
		if (option_texture_format != -1)
			if (!(option_texture_format & TEXTURE_TYPE_UNCOMPRESSED_BIT)) {
				printf("Error -- expected uncompressed texture type for destination.\n");
				exit(1);
			}
		if (option_mipmaps && !(source_filetype & FILE_TYPE_MIPMAPS_BIT)) {
			printf("Error -- destination texture file type cannot hold mipmaps.\n");
			exit(1);
		}
	}
	if (option_mipmaps && (source_filetype & FILE_TYPE_MIPMAPS_BIT)) {
		Image image[32];
		int n = load_mipmap_images(source_filename, source_filetype, 32, &image[0]);
		if (dest_filetype & FILE_TYPE_IMAGE_BIT) {
			// Saving multiple mipmaps to multiple image files.
			int j = strlen(dest_filename);
			char *basefilename = (char *)alloca(j - 4 + 1);
			strncpy(basefilename, dest_filename, j - 4);
			basefilename[j - 4] = '\0';
			char *filename = (char *)alloca(j + 12);
			for (int i = 0; i < n; i++) {
				sprintf(filename, "%s-mipmap%d%s", basefilename, i, get_extension(dest_filetype));
				if (option_flip_vertical)
					flip_image_vertical(&image[i]);
				save_image(&image[i], filename, dest_filetype);
			}
		}
		else {
			// Saving multiple mipmaps to uncompressed texture file.
			Texture texture[32];
			int texture_type;
			if (option_texture_format != - 1)
				texture_type = option_texture_format;
			else
			if (image[0].alpha_bits > 0)
				texture_type = TEXTURE_TYPE_UNCOMPRESSED_RGBA8;
			else
				texture_type = TEXTURE_TYPE_UNCOMPRESSED_RGB8;
			for (int i = 0; i < n; i++)
				copy_image_to_uncompressed_texture(&image[i], texture_type, &texture[i]);
			save_texture(&texture[0], n, dest_filename, dest_filetype);
		}
		return;
	}
	Image image;
	if (!option_quiet)
		printf("Decompressing %s to %s.\n", source_filename, dest_filename);
	// Load image, decompressing texture if necessary.
	load_image(source_filename, source_filetype, &image);
	if (option_flip_vertical)
		flip_image_vertical(&image);
	// Save to an image or uncompressed texture file.
	if (dest_filetype & FILE_TYPE_TEXTURE_BIT) {
		int texture_type;
		if (option_texture_format != - 1)
			texture_type = option_texture_format;
		else
		if (image.alpha_bits > 0)
			texture_type = TEXTURE_TYPE_UNCOMPRESSED_RGBA8;
		else
			texture_type = TEXTURE_TYPE_UNCOMPRESSED_RGB8;
		Texture texture;
		copy_image_to_uncompressed_texture(&image, texture_type, &texture);
		save_texture(&texture, 1, dest_filename, dest_filetype);
	}
	else
		save_image(&image, dest_filename, dest_filetype);
}

static void compress_callback(BlockUserData *user_data) {
	// Do nothing.
}

static void compress() {
	Image image[32];
	if (!option_quiet)
		printf("Compressing %s to %s.\n", source_filename, dest_filename);
	int nu_mipmaps;
	if (source_filetype & FILE_TYPE_MIPMAPS_BIT)
		nu_mipmaps = load_mipmap_images(source_filename, source_filetype, 32, &image[0]);
	else {
		load_image(source_filename, source_filetype, &image[0]);
		nu_mipmaps = 1;
	}
	if (option_flip_vertical)
		for (int i = 0; i < nu_mipmaps; i++)
			flip_image_vertical(&image[i]);
	int texture_type;
	if (option_texture_format != - 1)
		texture_type = option_texture_format;
	else
		// Set default compression format for the given the file type.
		if (dest_filetype == FILE_TYPE_KTX || dest_filetype == FILE_TYPE_PKM)
			texture_type = TEXTURE_TYPE_ETC1;
		else
		if (dest_filetype == FILE_TYPE_DDS)
			texture_type = TEXTURE_TYPE_DXT1;
	const char *texture_type_str = texture_type_text(texture_type);
	if (!option_quiet) {
		if (nu_mipmaps > 1)
			printf("Number of mipmaps in source file: %d\n", nu_mipmaps);
		printf("Target texture format: %s\n", texture_type_str);
	}
	// If there is only one mipmap in the source, and the --mipmaps option was given, generate mipmaps.
	int generate_mipmaps = 0;
	if (option_mipmaps && nu_mipmaps == 1) {
		nu_mipmaps = count_mipmap_levels(&image[0]);
		generate_mipmaps = 1;
		if (!option_quiet)
			printf("Generating %d mipmaps.\n", nu_mipmaps);
	}
	Texture *texture = (Texture *)alloca(sizeof(Texture) * nu_mipmaps);
	Image *mipmap_image = (Image *)alloca(sizeof(Image) * nu_mipmaps);
	if (generate_mipmaps) {
		mipmap_image[0] = image[0];
		if ((texture_type & TEXTURE_TYPE_SRGB_BIT) && nu_mipmaps > 1) {
			// For sRGB textures, perform mipmapping after first converting to RGB and then convert the
			// RGB mipmaps back to sRGB.
			Image *rgb_mipmap_image = (Image *)alloca(sizeof(Image) * nu_mipmaps);
			if (!option_quiet)
				printf("Converting image from sRGB to RGB for mipmap generation.\n");
			convert_image_from_srgb_to_rgb(&image[0], &rgb_mipmap_image[0]);
			// Generate the mipmaps in RGB space.
			for (int i = 1; i < nu_mipmaps; i++) {
				generate_mipmap_level_from_previous_level(&rgb_mipmap_image[i - 1], &rgb_mipmap_image[i]);
			}
			// Convert them to sRGB
			for (int i = 1; i < nu_mipmaps; i++) {
				convert_image_from_rgb_to_srgb(&rgb_mipmap_image[i], &mipmap_image[i]);
			}
		}
		else
			for (int i = 1; i < nu_mipmaps; i++) {
				generate_mipmap_level_from_previous_level(&mipmap_image[i - 1], &mipmap_image[i]);
			}
	}
	else {
		// The mipmaps are present in the source file.
		for (int i = 0; i < nu_mipmaps; i++) {
			mipmap_image[i] = image[i];
			printf("Source mipmap %d: %d x %d\n", i, mipmap_image[i].width, mipmap_image[i].height);
		}
	}
	for (int i = 0; i < nu_mipmaps; i++) {
		// Compress the image into a texture.
		if (!option_quiet)
			printf("Mipmap level: %d (%d x %d)\n", i, mipmap_image[i].width, mipmap_image[i].height);
		compress_image(&mipmap_image[i], texture_type, compress_callback, &texture[i], 0, 0, 0);
		// Decompress the compressed texture and calculate the difference with the original.
		Image compressed_image;
		convert_texture_to_image(&texture[i], &compressed_image);
		compare_images(&mipmap_image[i], &compressed_image);
		destroy_image(&compressed_image);
	}
	// Save texture.
	save_texture(&texture[0], nu_mipmaps, dest_filename, dest_filetype);
}

static void calibrate() {
	Image image;
	if (!option_quiet)
		printf("Calibrating genetic parameters for compression of source file %s.\n", source_filename);
	load_image(source_filename, source_filetype, &image);
	int texture_type;
	if (option_texture_format != - 1)
		texture_type = option_texture_format;
	else
		// Set default compression format for the given the file type.
		if (dest_filetype == FILE_TYPE_KTX || dest_filetype == FILE_TYPE_PKM)
			texture_type = TEXTURE_TYPE_ETC1;
		else
		if (dest_filetype == FILE_TYPE_DDS)
			texture_type = TEXTURE_TYPE_DXT1;
	calibrate_genetic_parameters(&image, texture_type);	
}


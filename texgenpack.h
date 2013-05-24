/*
    texgenpack.h -- part of texgenpack, a texture compressor using fgen.

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

#define NU_FILE_TYPES		6

#define FILE_TYPE_PNG		0x101
#define FILE_TYPE_PPM		0x102
#define FILE_TYPE_KTX		0x210
#define FILE_TYPE_PKM		0x201
#define FILE_TYPE_DDS		0x212
#define FILE_TYPE_ASTC		0x203
#define FILE_TYPE_IMAGE_BIT	0x100
#define FILE_TYPE_TEXTURE_BIT	0x200
#define FILE_TYPE_MIPMAPS_BIT	0x010
#define FILE_TYPE_UNDEFINED	0x000
#define FILE_TYPE_IMAGE_UNKNOWN 0x100

// Structures.

typedef struct {
	unsigned int *pixels;
	int width;
	int height;
	int extended_width;
	int extended_height;
	int alpha_bits;			// 0 for no alpha, 1 if alpha is limited to 0 and 0xFF, 8 otherwise.
	int nu_components;		// Indicates the number of components.
	int bits_per_component;		// 8 or 16.
	int is_signed;			// 1 if the components are signed, 0 if unsigned.
	int srgb;			// Whether the image is stored in sRGB format.
	int is_half_float;		// The image pixels are combinations of half-floats. The pixel size is 64-bit.
} Image;


#define TEXTURE_TYPE_UNCOMPRESSED_RGB8			0x2000
#define TEXTURE_TYPE_UNCOMPRESSED_RGBA8			0x2021
#define TEXTURE_TYPE_UNCOMPRESSED_ARGB8 		0x2022
#define TEXTURE_TYPE_UNCOMPRESSED_RGB_HALF_FLOAT	0x6402
#define TEXTURE_TYPE_UNCOMPRESSED_RGBA_HALF_FLOAT 	0x6422
#define TEXTURE_TYPE_UNCOMPRESSED_RG16			0x2400
#define TEXTURE_TYPE_UNCOMPRESSED_RG_HALF_FLOAT		0x6800
#define TEXTURE_TYPE_UNCOMPRESSED_R16			0x2401
#define TEXTURE_TYPE_UNCOMPRESSED_R_HALF_FLOAT		0x6400
#define TEXTURE_TYPE_UNCOMPRESSED_RG8			0x2001
#define TEXTURE_TYPE_UNCOMPRESSED_R8			0x2002
#define TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG16		0x2C00
#define TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R16		0x2C01
#define TEXTURE_TYPE_UNCOMPRESSED_SIGNED_RG8		0x2801
#define TEXTURE_TYPE_UNCOMPRESSED_SIGNED_R8		0x2802
#define TEXTURE_TYPE_ETC1				0x0100
#define TEXTURE_TYPE_ETC2_RGB8				0x0101
#define TEXTURE_TYPE_ETC2_EAC				0x0162
#define TEXTURE_TYPE_ETC2_PUNCHTHROUGH			0x0123
#define TEXTURE_TYPE_R11_EAC				0x0400
#define TEXTURE_TYPE_RG11_EAC				0x0440
#define TEXTURE_TYPE_SIGNED_R11_EAC			0x0C00
#define TEXTURE_TYPE_SIGNED_RG11_EAC			0x0C40
#define TEXTURE_TYPE_ETC2_SRGB8				0x1104
#define TEXTURE_TYPE_ETC2_SRGB_PUNCHTHROUGH		0x1125
#define TEXTURE_TYPE_ETC2_SRGB_EAC			0x1166
#define TEXTURE_TYPE_DXT1				0x0200
#define TEXTURE_TYPE_DXT3				0x0261
#define TEXTURE_TYPE_DXT5				0x0262
#define TEXTURE_TYPE_DXT1A				0x0223
#define TEXTURE_TYPE_BPTC				0x0264
#define TEXTURE_TYPE_BPTC_FLOAT				0x4645
#define TEXTURE_TYPE_BPTC_SIGNED_FLOAT			0x4E46
#define TEXTURE_TYPE_RGTC1				0x0001
#define TEXTURE_TYPE_SIGNED_RGTC1			0x0C01
#define TEXTURE_TYPE_RGTC2				0x0041
#define TEXTURE_TYPE_SIGNED_RGTC2			0x0C41
#define TEXTURE_TYPE_RGBA_ASTC_4X4			0x8000
#define TEXTURE_TYPE_RGBA_ASTC_5X4			0x8001
#define TEXTURE_TYPE_RGBA_ASTC_5X5			0x8002
#define TEXTURE_TYPE_RGBA_ASTC_6X5			0x8003
#define TEXTURE_TYPE_RGBA_ASTC_6X6			0x8004
#define TEXTURE_TYPE_RGBA_ASTC_8X5			0x8005
#define TEXTURE_TYPE_RGBA_ASTC_8X6			0x8006
#define TEXTURE_TYPE_RGBA_ASTC_8X8			0x8007
#define TEXTURE_TYPE_RGBA_ASTC_10X5			0x8008
#define TEXTURE_TYPE_RGBA_ASTC_10X6			0x8009
#define TEXTURE_TYPE_RGBA_ASTC_10X8			0x800A
#define TEXTURE_TYPE_RGBA_ASTC_10X10			0x800B
#define TEXTURE_TYPE_RGBA_ASTC_12X10			0x800C
#define TEXTURE_TYPE_RGBA_ASTC_12X12			0x800D
#define TEXTURE_TYPE_SRGB8_ALPHA8_ASTC_4X4		0x9000

#define TEXTURE_TYPE_ALPHA_BIT				0x0020
#define TEXTURE_TYPE_ETC_BIT				0x0100
#define TEXTURE_TYPE_DXTC_BIT				0x0200
#define TEXTURE_TYPE_128BIT_BIT				0x0040
#define TEXTURE_TYPE_16_BIT_COMPONENTS_BIT		0x0400
#define TEXTURE_TYPE_SIGNED_BIT				0x0800
#define TEXTURE_TYPE_SRGB_BIT				0x1000
#define TEXTURE_TYPE_UNCOMPRESSED_BIT			0x2000
#define TEXTURE_TYPE_HALF_FLOAT_BIT			0x4000
#define TEXTURE_TYPE_ASTC_BIT				0x8000

typedef struct {
	int type;
	int ktx_support;
	int dds_support;
	const char *text1;
	const char *text2;
	int block_width;		// The block width (1 for uncompressed textures).
	int block_height;		// The block height (1 for uncompressed textures).
	int bits_per_block;		// The number of bits per block (per pixel for uncompressed textures).
	int internal_bits_per_block;	// The number of bits per block as stored internally (per pixel for uncompressed).
	int alpha_bits;
	int nu_components;
	int gl_internal_format;
	int gl_format;
	int gl_type;
	const char *dx_four_cc;
	int dx10_format;
	uint64_t red_mask, green_mask, blue_mask, alpha_mask;
} TextureInfo;

typedef struct BlockUserData_t BlockUserData;

typedef int (*TextureDecodingFunction)(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
typedef double (*TextureComparisonFunction)(unsigned int *image_buffer, BlockUserData *user_data);

typedef struct {
	unsigned int *pixels;
	int width;
	int height;
	int extended_width;
	int extended_height;
	int type;
	int bits_per_block;		// The bits per block of the real format. The internally used bits per block is
					// given by info->internal_bits_per_block.
	int block_width;
	int block_height;
	TextureDecodingFunction decoding_function;
	TextureComparisonFunction comparison_function;
	TextureInfo *info;
} Texture;

struct BlockUserData_t {
	unsigned int *image_pixels;
	int image_rowstride;
	int x_offset;
	int y_offset;
	int flags;
	Texture *texture;
	unsigned char *alpha_pixels;
	int stop_signalled;
};

typedef void (*CompressCallbackFunction)(BlockUserData *user_data);

// Command line options defined in texgenpack.c

#define COMMAND_COMPRESS	0
#define COMMAND_DECOMPRESS	1
#define COMMAND_COMPARE		2
#define COMMAND_CALIBRATE	3

#define ORIENTATION_DOWN	1
#define ORIENTATION_UP		2

#define SPEED_ULTRA	0
#define SPEED_FAST	1
#define SPEED_MEDIUM	2
#define SPEED_SLOW	3

extern int command;
extern int option_verbose;
extern int option_max_threads;
extern int option_orientation;
extern int option_speed;
extern int option_progress;
extern int option_modal_etc2;
extern int option_allowed_modes_etc2;
extern int option_generations;
extern int option_islands;
extern int option_texture_format;
extern int option_flip_vertical;
extern int option_quiet;
extern int option_block_width;
extern int option_block_height;
extern int option_half_float;
extern int option_deterministic;
extern int option_hdr;

// Defined in image.c

void load_image(const char *filename, int filetype, Image *image);
int load_mipmap_images(const char *filename, int filetype, int max_images, Image *image);
void save_image(Image *image, const char *filename, int filetype);
double compare_images(Image *image1, Image *image2);
int load_texture(const char *filename, int filetype, int max_mipmaps, Texture *texture);
void save_texture(Texture *texture, int nu_mipmaps, const char *filename, int filetype);
void convert_texture_to_image(Texture *texture, Image *image);
void destroy_texture(Texture *texture);
void destroy_image(Image *image);
void clone_image(Image *image1, Image *image2);
void clear_image(Image *image);
void pad_image_borders(Image *image);
void check_1bit_alpha(Image *image);
void convert_image_from_srgb_to_rgb(Image *source_image, Image *dest_image);
void convert_image_from_rgb_to_srgb(Image *source_image, Image *dest_image);
void copy_image_to_uncompressed_texture(Image *image, int texture_type, Texture *texture);
void flip_image_vertical(Image *image);
void print_image_info(Image *image);
void calculate_image_dynamic_range(Image *image, float *range_min_out, float *range_max_out);
void convert_image_from_half_float(Image *image, float range_min, float range_max, float gamma);
void convert_image_to_half_float(Image *image);
void extend_half_float_image_to_rgb(Image *image);
void remove_alpha_from_image(Image *image);
void add_alpha_to_image(Image *image);
void convert_image_from_16_bit_format(Image *image);
void convert_image_to_16_bit_format(Image *image, int nu_components, int signed_format);
void convert_image_from_8_bit_format(Image *image);
void convert_image_to_8_bit_format(Image *image, int nu_components, int signed_format);

// Defined in compress.c

void compress_image(Image *image, int texture_type, CompressCallbackFunction func, Texture *texture,
int genetic_parameters, float mutation_prob, float crossover_prob);

// Defined in mipmap.c

void generate_mipmap_level_from_original(Image *source_image, int level, Image *dest_image);
void generate_mipmap_level_from_previous_level(Image *source_image, Image *dest_image);
int count_mipmap_levels(Image *image);

// Defined in file.c

void load_pkm_file(const char *filename, Texture *texture);
void save_pkm_file(Texture *texture, const char *fikename);
int load_ktx_file(const char *filename, int max_mipmaps, Texture *texture);
void save_ktx_file(Texture *texture, int nu_mipmaps, const char *filename);
int load_dds_file(const char *filename, int max_mipmaps, Texture *texture);
void save_dds_file(Texture *texture, int nu_mipmaps, const char *filename);
void load_astc_file(const char *filename, Texture *texture);
void save_astc_file(Texture *texture, const char *filename);
void load_ppm_file(const char *filename, Image *image);
void load_png_file(const char *filename, Image *image);
void save_png_file(Image *image, const char *filename);

// Defined in texture.c

TextureInfo *match_texture_type(int type);
TextureInfo *match_texture_description(const char *s);
TextureInfo *match_ktx_id(int gl_internal_format, int gl_format, int gl_type);
TextureInfo *match_dds_id(const char *four_cc, int dx10_format, uint32_t pixel_format_flags, int bitcount,
uint32_t red_mask, uint32_t green_mask, uint32_t blue_mask, uint32_t alpha_mask);
const char *texture_type_text(int texture_type);
int get_number_of_texture_formats();
const char *get_texture_format_index_text(int i, int j);
void set_texture_decoding_function(Texture *texture, Image *image);

// Defined in compare.c

extern float *half_float_table;
extern float *gamma_corrected_half_float_table;
extern float *normalized_float_table;

double compare_block_any_size_rgba(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rgb(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rgba(unsigned int *image_buffer, BlockUserData *user_data);
void calculate_normalized_float_table();
double compare_block_4x4_rgb8_with_half_float(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rgba8_with_half_float(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_8_bit_components(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_signed_8_bit_components(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_8_bit_components_with_16_bit(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_signed_8_bit_components_with_16_bit(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_r16(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rg16(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_r16_signed(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rg16_signed(unsigned int *image_buffer, BlockUserData *user_data);
void calculate_half_float_table();
double compare_block_4x4_rgb_half_float(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rgba_half_float(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_r_half_float(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rg_half_float(unsigned int *image_buffer, BlockUserData *user_data);
void calculate_gamma_corrected_half_float_table();
double compare_block_4x4_rgb_half_float_hdr(unsigned int *image_buffer, BlockUserData *user_data);
double compare_block_4x4_rgba_half_float_hdr(unsigned int *image_buffer, BlockUserData *user_data);

// Defined in half_float.c

int halfp2singles(void *target, void *source, int numel);
int singles2halfp(void *target, void *source, int numel);

// Defined in calibrate.c

void calibrate_genetic_parameters(Image *image, int texture_type);


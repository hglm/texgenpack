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


// Flags values for the decoding functions.

enum {
	ETC_MODE_ALLOWED_INDIVIDUAL = 0x1,
	ETC_MODE_ALLOWED_DIFFERENTIAL = 0x2,
	ETC2_MODE_ALLOWED_T = 0x4,
	ETC2_MODE_ALLOWED_H = 0x8,
	ETC2_MODE_ALLOWED_PLANAR = 0x10,
	ETC_MODE_ALLOWED_ALL = 0x3,
	ETC2_MODE_ALLOWED_ALL = 0x1F,
	ETC2_PUNCHTHROUGH_MODE_ALLOWED_ALL = 0x1E,
	BPTC_MODE_ALLOWED_ALL = 0xFF,
	MODES_ALLOWED_OPAQUE_ONLY = 0x100,
	MODES_ALLOWED_NON_OPAQUE_ONLY = 0x200,
	MODES_ALLOWED_PUNCHTHROUGH_ONLY = 0x400,
	BPTC_FLOAT_MODE_ALLOWED_ALL = 0x3FFF,
	ENCODE_BIT = 0x10000,
	TWO_COLORS = 0x20000,
};

// Functions defined in etc2.c.

// Draw (decompress) a 64-bit 4x4 pixel block.
int draw_block4x4_etc1(const unsigned char *bitstring, unsigned int *image_buffer, int modes_allowed);
int draw_block4x4_etc2_rgb8(const unsigned char *bitstring, unsigned int *image_buffer, int modes_allowed);
int draw_block4x4_etc2_eac(const unsigned char *bitstring, unsigned int *image_buffer, int modes_allowed);
int draw_block4x4_etc2_punchthrough(const unsigned char *bitstring, unsigned int *image_buffer, int modes_allowed);
int draw_block4x4_r11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_rg11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_signed_r11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_signed_rg11_eac(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
// Return ETC1 mode (0 or 1).
int block4x4_etc1_get_mode(const unsigned char *bitstring);
// Return ETC2 mode number from 0 to 4.
int block4x4_etc2_rgb8_get_mode(const unsigned char *bitstring);
int block4x4_etc2_eac_get_mode(const unsigned char *bitstring);
int block4x4_etc2_punchthrough_get_mode(const unsigned char *bitstring);
// Modify bitstring to conform to modes.
void block4x4_etc1_set_mode(unsigned char *bitstring, int flags);
void block4x4_etc2_rgb8_set_mode(unsigned char *bitstring, int flags);
void block4x4_etc2_punchthrough_set_mode(unsigned char *bitstring, int flags);
void block4x4_etc2_eac_set_mode(unsigned char *bitstring, int flags);
// "Manual" optimization function.
void optimize_block_alpha_etc2_punchthrough(unsigned char *bitstring, unsigned char *alpha_values);
void optimize_block_alpha_etc2_eac(unsigned char *bitstring, unsigned char *alpha_values, int flags);

// Functions defined in dxtc.c.

int draw_block4x4_dxt1(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_dxt1a(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_dxt3(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_dxt5(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
void optimize_block_alpha_dxt3(unsigned char *bitstring, unsigned char *alpha_values);

// Functions defined in astc.c

int draw_block_rgba_astc(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
void convert_astc_texture_to_image(Texture *texture, Image *image);
void decompress_astc_file(const char *filename, Image *image);
void compress_image_to_astc_texture(Image *image, int texture_type, Texture *texture);
int match_astc_block_size(int w, int h);
int get_astc_block_size_width(int astc_block_type);
int get_astc_block_size_height(int astc_block_type);

// Functions defined in bptc.c

int draw_block4x4_bptc(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int block4x4_bptc_get_mode(const unsigned char *bitstring);
int draw_block4x4_bptc_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_bptc_signed_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int block4x4_bptc_float_get_mode(const unsigned char *bitstring);
void block4x4_bptc_set_mode(unsigned char *bitstring, int flags);
void block4x4_bptc_float_set_mode(unsigned char *bitstring, int flags);
// Try to preinitialize colors for particular modes.
void bptc_set_block_colors(unsigned char *bitstring, int flags, unsigned int *colors);

// Functions defined in rgtc.c

int draw_block4x4_rgtc1(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_signed_rgtc1(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_rgtc2(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_signed_rgtc2(const unsigned char *bitstring, unsigned int *image_buffer, int flags);

// Function defined in texture.c.

int draw_block4x4_uncompressed(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_argb8(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_uncompressed_rgb_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_uncompressed_rgba_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_uncompressed_r_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags);
int draw_block4x4_uncompressed_rg_half_float(const unsigned char *bitstring, unsigned int *image_buffer, int flags);


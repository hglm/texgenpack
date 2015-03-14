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

// Defined in viewer.c

extern int current_nu_image_sets;
extern Image current_image[2][32];
extern Texture current_texture[2][32];
extern int current_nu_mipmaps[2];
extern int current_file_type[2];
extern double current_rmse[32];
extern float dynamic_range_min, dynamic_range_max, dynamic_range_gamma;

int determine_filename_type(const char *filename);
void calculate_difference_images(Image *source_image1, Image *source_image2, int n, Image *dest_image);
void convert_image_to_or_from_cairo_format(Image *image);
void destroy_image_set(int j);

// Defined in gtk.c.

void gui_initialize(int *argc, char ***argv);
void gui_create_window_layout();
void gui_run();
void gui_handle_events();
void gui_draw_window();
void gui_draw_and_show_window();
void gui_create_base_surface(int i);
void gui_zoom_fit_to_window();


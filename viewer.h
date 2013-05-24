/*
    viewer.h -- part of texgenpack, a texture compressor using fgen.

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


/*
    gtk.c -- part of texgenpack, a texture compressor using fgen.

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
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#ifdef __GNUC__
#include <sys/time.h>
#else
#include <time.h>
#include <windows.h>
#endif
#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include "texgenpack.h"
#include "decode.h"
#include "packing.h"
#include "viewer.h"
#ifndef __GNUC__
#include "strcasecmp.h"
#endif

#if GTK_MAJOR_VERSION == 2

static GtkWidget *gtk_box_new(GtkOrientation orientation, gint spacing) {
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		return gtk_hbox_new(FALSE, spacing);
	else
		return gtk_vbox_new(FALSE, spacing);
}

#endif

#if defined(_WIN32) && !defined(__GNUC__)

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

static void gettimeofday(struct timeval *tv, struct timezone *tz) {
	FILETIME ft;
	uint64_t tmpres = 0;

	if (NULL != tv) {
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		tmpres /= 10;  /*convert into microseconds*/
		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS; 
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

 }

#endif

/*
 * A note about internal image storage:
 *
 * RGB(A) images are stored in cairo's AGRB8 format.
 * half-float images are stored in regular RGBA half-float format.
 * 8-bit and 16-bit images with two or less components are stored in native (non-cairo) format.
 */

#define SET_BORDER_WIDTH 16.0

typedef struct {
	int index;
	GtkWidget *image_drawing_area;
	cairo_surface_t *surface;
	cairo_surface_t *base_surface[2];
	double area_width, area_height;
} Window;

static GtkWidget *gtk_window;
static Window *window;
static int GUI_initialized = 0;
static int window_width = 1024;
static int window_height = 800;
static double zoom = 1.0;
static double zoom_origin_x = 0;
static double zoom_origin_y = 0;
static int difference_mode = 0;
static int compression_active = 0;

static gboolean delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    return FALSE;
}

static void destroy_cb(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
    exit(0);
}

static gboolean area_configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, Window *window) {
    if (!GUI_initialized) {
        return TRUE;
    }
    window->area_width = event->width;
    window->area_height = event->height;
    cairo_surface_destroy(window->surface);
	for (int i = 0; i < current_nu_image_sets; i++)
		gui_create_base_surface(i);
    window->surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, window->area_width, window->area_height);
    gui_draw_window();
    /* We've handled the configure event, no need for further processing. */
    return TRUE;
}

#if GTK_MAJOR_VERSION == 2

static gboolean area_expose_cb(GtkWidget *widget, GdkEventExpose *event, Window *window) {
	cairo_t *cr = gdk_cairo_create(widget->window);
	cairo_set_source_surface(cr, window->surface, 0, 0);
	cairo_paint(cr);
	return TRUE;
}

#else

static gboolean area_draw_cb(GtkWidget *widget, cairo_t *cr, Window *window) {
    cairo_set_source_surface(cr, window->surface, 0, 0);
    cairo_paint(cr);
    return TRUE;
}

#endif

static void gui_update_drawing_area(Window *window) {
    gtk_widget_queue_draw(GTK_WIDGET(window->image_drawing_area));
}

void gui_initialize(int *argc, char ***argv) {
    gtk_init(argc, argv);
}

static double last_press_event_x = 0;
static double last_press_event_y = 0;
static double last_press_time;

static gboolean area_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, Window *window) {
	if (event->button == 1) {
		last_press_event_x = event->x;
		last_press_event_y = event->y;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		last_press_time = tv.tv_sec * 1000000 + tv.tv_usec;
	}
	return FALSE;
}

static gboolean area_button_release_event_cb(GtkWidget *widget, GdkEventButton *event, Window *window) {
	if (event->button == 1) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		double release_time = tv.tv_sec * 1000000 + tv.tv_usec;
		double dx = event->x - last_press_event_x;
		double dy = event->y - last_press_event_y;
		if (release_time - last_press_time < 100000 || (dx * dx + dy * dy < 100)) {
			// Click centers on mouse position.
			double borders_x = 16 + SET_BORDER_WIDTH * (current_nu_image_sets - 1);
			double image_set_width = (window->area_width - borders_x) / current_nu_image_sets;
			// Figure out on which image set the click is.
			int j = - 1;
			for (int i = 0; i < current_nu_image_sets; i++) {
				double start_x = 8 + ((window->area_width - borders_x) / current_nu_image_sets
					+ SET_BORDER_WIDTH) * i;
				if (event->x >= start_x && event->x < start_x + image_set_width)
					j = i;
			}
			if (j == - 1)
				// Click on the border.
				return FALSE;
			double center_x = 8 + ((window->area_width - borders_x) / current_nu_image_sets +
				SET_BORDER_WIDTH) * j +	(image_set_width / 2);
			double center_y = 32 + (window->area_height - 32 - 8) / 2;
			zoom_origin_x -= (event->x - center_x); // * zoom;
			zoom_origin_y -= (event->y - center_y); // * zoom;
		}
		else {
			// Pan moves image.
			zoom_origin_x += dx;
			zoom_origin_y += dy;
		}
		gui_draw_and_show_window();
	}
	return FALSE;
}

static gboolean area_scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, Window *window) {
	double borders_x = 16 + SET_BORDER_WIDTH * (current_nu_image_sets - 1);
	double image_set_width = (window->area_width - borders_x) / current_nu_image_sets;
	double center_x = (image_set_width / 2);
	double center_y = (window->area_height - 32 - 8) / 2;
	if (event->direction == GDK_SCROLL_UP && zoom < 32) {
		// Zoom in.
		// Center defined by
		// zoom * x_orig + origin_x = area_width / 2
		// zoom * y_orig + origin_y = area_height / 2
		// new_zoom * x_orig + new_origin_x = area_width / 2
		// new_zoom * y_orig  + new_origin_y = area_height / 2	
		//
		zoom_origin_x = center_x - 1.2 * (center_x - zoom_origin_x);
		zoom_origin_y = center_y - 1.2 * (center_y - zoom_origin_y);
		zoom *= 1.2;
	}
	else
	if (event->direction == GDK_SCROLL_DOWN && zoom > (double)1 / 32) {
		// Zoom out.
		zoom_origin_x = center_x - (1 / 1.2) * (center_x - zoom_origin_x);
		zoom_origin_y = center_y - (1 / 1.2) * (center_y - zoom_origin_y);
		zoom *= 1 / 1.2;
	}
	else
		return FALSE;
	gui_draw_and_show_window();
	return FALSE;
}

void gui_zoom_fit_to_window() {
	zoom_origin_x = zoom_origin_y = 0;
	// Determine the scaling factor so that the image fits neatly within the window.
	double factorx, factory;
	double max_w = 0;
	double h = 0;
	for (int i = 0; i < current_nu_image_sets; i++) {
		if (current_nu_mipmaps[i] > 1) {
			if (current_image[i][0].width + 8 + current_image[i][0].width / 2 > max_w)
				max_w = current_image[i][0].width + 8 + current_image[i][0].width / 2;
		}
		else
			if (current_image[i][0].width > max_w)
				max_w = current_image[i][0].width;
		if (current_image[i][0].height > h)
			h = current_image[i][0].height;
	}
	double w = max_w * current_nu_image_sets;
	factorx = ((double)window->area_width - 16 + SET_BORDER_WIDTH * (current_nu_image_sets - 1)) / w;
	factory = ((double)window->area_height - 32 - 8) / h;
	double factor;
	if (factorx > factory) {
		factor = factory;
	}
	else {
		factor = factorx;
	}
	zoom = factor;
}

static void menu_item_reset_zoom_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	gui_zoom_fit_to_window();
	gui_draw_and_show_window();
}

static void menu_item_reset_zoom_real_size_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	zoom = 1.0;
	zoom_origin_x = zoom_origin_y = 0;
	gui_draw_and_show_window();
}

static void check_menu_item_show_difference_toggled_cb(GtkMenuItem *menu_item, gpointer data) {
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)))
		difference_mode = 1;
	else
		difference_mode = 0;
	gui_create_base_surface(1);
	gui_draw_and_show_window();
}

static void check_menu_item_hdr_toggled_cb(GtkMenuItem *menu_item, gpointer data) {
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)))
		option_hdr = 1;
	else
		option_hdr = 0;
}

static void menu_item_generate_mipmaps_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	for (int i = 1; i < current_nu_mipmaps[0]; i++)
		free(current_image[0][i].pixels);
	current_nu_mipmaps[0] = 1;
	int n = count_mipmap_levels(&current_image[0][0]);
	for (int i = 1; i < n; i++) {
		generate_mipmap_level_from_previous_level(&current_image[0][i - 1], &current_image[0][i]);
	}
	current_nu_mipmaps[0] = n;
	gui_zoom_fit_to_window();
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static int mipmap_level_being_compressed;
static int stop_compression;
static double compress_start_time;
static double last_compress_progress_time;
static double compress_progress; // [0, 1]
static const double compression_update_frequency_table[5] =
	{ 1.0 / 60.0, 0.1, 1, 10.0, 1E40 };
// Update screen at most every 1.0 seconds during compression by default.
// (Some GTK implementations, such as under Windows, may be slow, while GUI updates
// for large textures may impact compression speed on any system since the whole
// texture is potentially scaled/redrawn with cairo).
#ifdef _WIN32
static int compression_update_frequency_selection = 2;
#else
static int compression_update_frequency_selection = 2;
#endif

// Calculate progress and return true if the window needs to be updated.

static int calculate_progress(Texture *texture, int compressed_block_index) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double current_time = tv.tv_sec + tv.tv_usec * 0.000001;
	if (current_time - last_compress_progress_time >=
	compression_update_frequency_table[compression_update_frequency_selection]) {
		compress_progress = (double)compressed_block_index /
			((texture->extended_height / texture->block_height) *
			(texture->extended_width / texture->block_width));
		last_compress_progress_time = current_time;
		return 1;
	}
	return 0;
}


// This function is called by the compress function after each compressed block.

static void compress_callback(BlockUserData *user_data) {
	Texture *texture = user_data->texture;
	int x = user_data->x_offset;
	int y = user_data->y_offset;
	// Decompress the block.
	uint32_t image_buffer[16];
	int compressed_block_index = (y / texture->block_height) * (texture->extended_width / texture->block_width)
		+ x / texture->block_width;
	texture->decoding_function((unsigned char *)&texture->pixels[compressed_block_index *
		(texture->info->internal_bits_per_block / 32)], image_buffer, ETC2_MODE_ALLOWED_ALL);
	// Copy it to image[1].
	int h = 4;
	if (y + h > current_image[1][mipmap_level_being_compressed].height)
		h = current_image[1][mipmap_level_being_compressed].height - y;
	int w = 4;
	if (x + w > current_image[1][mipmap_level_being_compressed].width)
		w = current_image[1][mipmap_level_being_compressed].width - x;
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			uint32_t pixel = image_buffer[i * 4 + j];
			// Put the pixel in cairo's ARGB32 format.
			current_image[1][mipmap_level_being_compressed].pixels[(y + i) *
				current_image[1][mipmap_level_being_compressed].extended_width + (x + j)] =
				pack_rgba(pixel_get_b(pixel), pixel_get_g(pixel), pixel_get_r(pixel),
						pixel_get_a(pixel));
		}
        if (calculate_progress(texture, compressed_block_index)) {
		gui_create_base_surface(1);
		gui_draw_and_show_window();
	}
	gui_handle_events();
	if (stop_compression)
		user_data->stop_signalled = 1;
}

// This function is called by the compress function after each compressed block. Half-float version.

static void compress_half_float_callback(BlockUserData *user_data) {
	Texture *texture = user_data->texture;
	int x = user_data->x_offset;
	int y = user_data->y_offset;
	// Decompress the block.
	uint64_t image_buffer[16];	// 64-bit pixels
	int compressed_block_index = (y / texture->block_height) * (texture->extended_width / texture->block_width)
		+ x / texture->block_width;
	texture->decoding_function((unsigned char *)&texture->pixels[compressed_block_index *
		(texture->info->internal_bits_per_block / 32)], (unsigned int *)image_buffer,
		BPTC_FLOAT_MODE_ALLOWED_ALL);
	// Copy it to image[1].
	int h = 4;
	if (y + h > current_image[1][mipmap_level_being_compressed].height)
		h = current_image[1][mipmap_level_being_compressed].height - y;
	int w = 4;
	if (x + w > current_image[1][mipmap_level_being_compressed].width)
		w = current_image[1][mipmap_level_being_compressed].width - x;
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			uint64_t pixel64 = image_buffer[i * 4 + j];
			*(uint64_t *)&current_image[1][mipmap_level_being_compressed].pixels[((y + i) *
				current_image[1][mipmap_level_being_compressed].extended_width + (x + j)) * 2] =
				pixel64;
		}
        if (calculate_progress(texture, compressed_block_index)) {
		gui_create_base_surface(1);
		gui_draw_and_show_window();
	}
	gui_handle_events();
	if (stop_compression)
		user_data->stop_signalled = 1;
}

// This function is called by the compress function after each compressed block. 16-bit component version.

static void compress_rg16_callback(BlockUserData *user_data) {
	Texture *texture = user_data->texture;
	int x = user_data->x_offset;
	int y = user_data->y_offset;
	// Decompress the block.
	uint32_t image_buffer[16];
	int compressed_block_index = (y / texture->block_height) * (texture->extended_width / texture->block_width)
		+ x / texture->block_width;
	texture->decoding_function((unsigned char *)&texture->pixels[compressed_block_index *
		(texture->info->internal_bits_per_block / 32)], image_buffer, 0);
	// Copy it to image[1].
	int h = 4;
	if (y + h > current_image[1][mipmap_level_being_compressed].height)
		h = current_image[1][mipmap_level_being_compressed].height - y;
	int w = 4;
	if (x + w > current_image[1][mipmap_level_being_compressed].width)
		w = current_image[1][mipmap_level_being_compressed].width - x;
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			uint32_t pixel = image_buffer[i * 4 + j];
			current_image[1][mipmap_level_being_compressed].pixels[(y + i) *
				current_image[1][mipmap_level_being_compressed].extended_width + (x + j)] =
				pixel;
		}
       if (calculate_progress(texture, compressed_block_index)) {
		gui_create_base_surface(1);
		gui_draw_and_show_window();
	}
	gui_handle_events();
	if (stop_compression)
		user_data->stop_signalled = 1;
}

// This function is called by the compress function after each compressed block. 8-bit component version.

static void compress_rg8_callback(BlockUserData *user_data) {
	compress_rg16_callback(user_data);
}

static void popup_message(const char *message) {
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_window), (GtkDialogFlags)(GTK_DIALOG_MODAL |
		GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void menu_item_compress_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	int format = TEXTURE_TYPE_ETC1;
	if (option_texture_format != - 1)	
		format = option_texture_format;
// Compression from half-float source to RGBA texture type is now allowed.
//	if (current_image[0][0].is_half_float && !(format & TEXTURE_TYPE_HALF_FLOAT_BIT)) {
//		popup_message("Error -- texture compression format is unsuited for half-float images. "
//			"Use a half-float compression format or convert the image to regular format.");
//		return;
//	}
	if (!current_image[0][0].is_half_float && (format & TEXTURE_TYPE_HALF_FLOAT_BIT)) {
		popup_message("Error -- half-float texture compression format is unsuited for regular images. "
			"Convert the image to half-float format or use a regular RGB texture format.");
		return;
	}
	if (current_image[0][0].bits_per_component != 16 && (format & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT)) {
		popup_message("Error -- 16-bit component texture compression is unsuited for regular images or "
			"images with 8-bit components.");
		return;
	}
// Compression from 16-bit source to 8-bit texture type is now allowed.
//	if (current_image[0][0].bits_per_component == 16 && !(format & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT)) {
//		if (!current_image[0][0].is_half_float) {
//			popup_message("Error -- texture compression format is unsuited for 16-bit component images.");
//			return;
//		}
//	}
	TextureInfo *info = match_texture_type(format);
	if (current_image[0][0].nu_components <= 2 && current_image[0][0].nu_components != info->nu_components) {
		popup_message("Error -- mismatch in the number of components.");
		return;
	}
	if (current_image[0][0].nu_components >= 3 && info->nu_components < 3) {
		popup_message("Error -- mismatch in the number of components.");
		return;
	}
	if ((current_image[0][0].is_signed && !(format & TEXTURE_TYPE_SIGNED_BIT)) ||
	(!(current_image[0][0].is_signed) && (format & TEXTURE_TYPE_SIGNED_BIT))) {
		popup_message("Error -- image type signedness does not match texture type.");
		return;
	}

	compression_active = 1;
	stop_compression = 0;
	destroy_image_set(1);
	current_file_type[1] = FILE_TYPE_UNDEFINED;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Allocate and clear image[1][i].
		current_image[1][i] = current_image[0][i];	// Copy fields.
		if (current_image[0][i].is_half_float && !(format & TEXTURE_TYPE_HALF_FLOAT_BIT)) {
			current_image[1][i].is_half_float = 0;
			current_image[1][i].bits_per_component = 8;
			if (current_image[0][i].alpha_bits > 0)
				current_image[1][i].alpha_bits = 8;
		}
		if (current_image[0][i].bits_per_component == 16 && !(format & TEXTURE_TYPE_16_BIT_COMPONENTS_BIT)) {
			current_image[1][i].bits_per_component = 8;
		}
		int bpp = 4;
		if (current_image[1][i].is_half_float)
			bpp = 8;
		current_image[1][i].pixels = (unsigned int *)malloc(current_image[1][i].height *
			current_image[1][i].extended_width * bpp);
		clear_image(&current_image[1][i]);
		current_nu_image_sets = 2;
		current_nu_mipmaps[1] = i + 1;
		gui_zoom_fit_to_window();
		mipmap_level_being_compressed = i;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		compress_start_time = tv.tv_sec + tv.tv_usec * 0.000001;
		compress_progress = 0;
		last_compress_progress_time = compress_start_time;
		if (format & TEXTURE_TYPE_ASTC_BIT) {
			// Make sure the window shows compression has started.
			current_file_type[1] = FILE_TYPE_IMAGE_UNKNOWN;
			gui_create_base_surface(1);
			gui_draw_and_show_window();
			gui_handle_events();
			current_file_type[1] = FILE_TYPE_UNDEFINED;
		}
		if (current_image[1][i].bits_per_component == 16 && !current_image[1][i].is_half_float) {
			// The image is in r16 or rg16 format, or the signed version.
			compress_image(&current_image[0][i], format, compress_rg16_callback,
				&current_texture[1][i], 0, 0, 0);
		}	
		else
		if (current_image[1][i].bits_per_component == 8 && current_image[1][i].nu_components <= 2) {
			// The image is in r8 or rg8 format, or signed r8 or signed rg8.
			compress_image(&current_image[0][i], format, compress_rg8_callback,
				&current_texture[1][i], 0, 0, 0);
		}
		else
		if (!current_image[1][i].is_half_float && current_image[0][i].is_half_float) {
			// The source image is in half-float format, the destination is in regular RGB(A)8 format.
			compress_image(&current_image[0][i], format, compress_callback,
				&current_texture[1][i], 0, 0, 0);
		}
		else
		if (!current_image[1][i].is_half_float) {
			// The image is a regular RGB(A)8 image.
			// Create a temporary image in non-cairo format to compress from.
			Image image;
			image = current_image[0][i];
			image.pixels = (unsigned int *)malloc(current_image[0][i].height * current_image[0][i].extended_width * 4);
			memcpy(image.pixels, current_image[0][i].pixels, current_image[0][i].height *
				current_image[0][i].extended_width * 4);
			convert_image_to_or_from_cairo_format(&image);
			compress_image(&image, format, compress_callback, &current_texture[1][i], 0, 0, 0);
			free(image.pixels);
		}
		else {
			// The image is in half-float format.
			compress_image(&current_image[0][i], format, compress_half_float_callback,
				&current_texture[1][i], 0, 0, 0);
		}
		// Uncompressed textures didn't generate compress_callback so the image hasn't been filled in.
		if (current_texture[1][i].type & TEXTURE_TYPE_UNCOMPRESSED_BIT) {
			convert_texture_to_image(&current_texture[1][i], &current_image[1][i]);
			if (!(current_texture[1][i].type & TEXTURE_TYPE_HALF_FLOAT_BIT))
				convert_image_to_or_from_cairo_format(&current_image[1][i]);
		}
		if (current_texture[1][i].type & TEXTURE_TYPE_ASTC_BIT) {
			// ASTC compression is handled via a system call to astenc.
			// This generates a half-float image.
			convert_texture_to_image(&current_texture[1][i], &current_image[1][i]);
			// If the first image (on the left) is in regular RGB, convert the second image to RGB.
			if (!current_image[0][0].is_half_float) {
				convert_image_from_half_float(&current_image[1][i], 0, 1.0, 1.0);
				convert_image_to_or_from_cairo_format(&current_image[1][i]);
			}
		}
		// For two RGB(A)8 images, the comparison will be performed in cairo's pixel format,
		// which doesn't matter for the RMSE calculation (R and B swapped in both images).
		// However, if the first image is in half-float format and the second in RGB(A)8 cairo
		// format, we have to temporarily convert the second image to non-cairo format for the
		// comparison.
		if (!current_image[1][i].is_half_float && current_image[0][i].is_half_float) {
			convert_image_to_or_from_cairo_format(&current_image[1][i]);
			current_rmse[i] = compare_images(&current_image[0][i], &current_image[1][i]);
			convert_image_to_or_from_cairo_format(&current_image[1][i]);
		}
		else
			current_rmse[i] = compare_images(&current_image[0][i], &current_image[1][i]);
		if (stop_compression)
			break;
	}
	compression_active = 0;
	gui_create_base_surface(1);
	gui_draw_and_show_window();
}

static void menu_item_stop_compression_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	stop_compression = 1;
}

static void menu_item_flip_vertical_right_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	for (int i = 0; i < current_nu_mipmaps[1]; i++)
		flip_image_vertical(&current_image[1][i]);
	gui_create_base_surface(1);
	gui_draw_and_show_window();
}

static GtkWidget *file_save_dialog;
static char *current_filename = NULL;

static void menu_item_save_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_nu_image_sets < 2) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_window), (GtkDialogFlags)(GTK_DIALOG_MODAL |
			GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			"No image or texture defined as second image.");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	const char *extension;
	TextureInfo *info = match_texture_type(current_texture[1][0].type);
	// Guess the preferred texture container format.
	// Check whether saving/loading is supported for KTX or DDS.
 	// Usually DDS will be preferable for DXT1/3/5.
	if (current_texture[1][0].type & TEXTURE_TYPE_DXTC_BIT)
		extension = "dds";
	// Otherwise prioritize depending on the platform.
#ifdef _WIN32
	else if (info->dds_support)
		extension = "dds";
	else if (info->ktx_support)
		extension = "ktx";
#else
	else if (info->ktx_support)
		extension = "ktx";
	else if (info->dds_support)
		extension = "dds";
#endif
	// KTX supports most formats (fail-safe).
	else
		extension = "ktx";
	char *filename;
	if (current_filename == NULL)
		filename = strdup("untitled.EXT");
	else {
		int n = strlen(current_filename);
		int i;
		for (i = n - 1; i > 0; i--)
			if (current_filename[i] == '.')
				break;
		if (i == 0)
			i = n;
		filename = (char *)malloc(i + 4 + 1);
		strncpy(filename, current_filename, i);
		strcpy(&filename[i], ".EXT");
	}
	strcpy(&filename[strlen(filename) - 3], extension);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_save_dialog),
		filename);
	free(filename);
again : ;
	int r = gtk_dialog_run(GTK_DIALOG(file_save_dialog));
	if (r == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_save_dialog));
		int type = determine_filename_type(filename);
		if (type & FILE_TYPE_TEXTURE_BIT) {
			if (type == FILE_TYPE_KTX)
				if (!match_texture_type(current_texture[1][0].type)->ktx_support) {
					popup_message("Texture format not supported by KTX file.");
					g_free(filename);
					goto again;
				}
			if (type == FILE_TYPE_DDS)
				if (!match_texture_type(current_texture[1][0].type)->dds_support) {
					popup_message("Texture format not supported by DDS file.");
					g_free(filename);
					goto again;
				}
			if (type == FILE_TYPE_PKM)
				if (current_texture[1][0].type != TEXTURE_TYPE_ETC1) {
					popup_message("Texture format not supported by PKM file.");
					g_free(filename);
					goto again;
				}
			if (type == FILE_TYPE_ASTC)
				if (!(current_texture[1][0].type & TEXTURE_TYPE_ASTC_BIT)) {
					popup_message("Texture format not supported by ASTC file.");
					g_free(filename);
					goto again;
				}
			save_texture(&current_texture[1][0], current_nu_mipmaps[1], filename, type);
		}
		else
		if (type & FILE_TYPE_IMAGE_BIT) {
			convert_image_to_or_from_cairo_format(&current_image[1][0]);
			save_image(&current_image[1][0], filename, type);
			convert_image_to_or_from_cairo_format(&current_image[1][0]);
		}
		else {
			popup_message("Can only save to filename with an extension corresponding to "
				" a KTX, DDS, PKM or PNG file.");
			g_free(filename);
			goto again;
		}
		g_free(filename);
	}
	gtk_widget_hide(file_save_dialog);
}

static GtkWidget *file_load_dialog;

static void menu_item_load_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
again : ;
	int r = gtk_dialog_run(GTK_DIALOG(file_load_dialog));
	if (r == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_load_dialog));
		int type = determine_filename_type(filename);
		if (type & FILE_TYPE_TEXTURE_BIT) {
			destroy_image_set(0);
			current_nu_mipmaps[0] = load_texture(filename, type, 32, &current_texture[0][0]);
			for (int i = 0 ; i < current_nu_mipmaps[0]; i++) {
				convert_texture_to_image(&current_texture[0][i], &current_image[0][i]);
				if (!(current_texture[0][i].type & TEXTURE_TYPE_HALF_FLOAT_BIT))
					convert_image_to_or_from_cairo_format(&current_image[0][i]);
			}
			current_file_type[0] = type;
			if (current_nu_image_sets == 0)
				current_nu_image_sets = 1;
		}
		else
		if (type & FILE_TYPE_IMAGE_BIT) {
			destroy_image_set(0);
			load_image(filename, type, &current_image[0][0]);
			current_nu_mipmaps[0] = 1;
                        if (current_image[0][0].bits_per_component == 8 && current_image[0][0].nu_components > 2)
				convert_image_to_or_from_cairo_format(&current_image[0][0]);
			current_file_type[0] = type;
			if (current_nu_image_sets == 0)
				current_nu_image_sets = 1;
		}
		else {
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_window), (GtkDialogFlags)(GTK_DIALOG_MODAL |
				GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				"Can only load a file with an extension corresponding to a "
				"KTX, DDS, PKM or PNG file.");
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			g_free(filename);
			goto again;
		}
		const char *title_str = "texview texture viewer";
		char *s = (char *)malloc(strlen(filename) + strlen(title_str) + 3 + 1);
		sprintf(s, "%s (%s)", title_str, filename);
    		gtk_window_set_title(GTK_WINDOW(gtk_window), s);
		free(s);
		if (current_filename != NULL)
			free(current_filename);
		current_filename = strdup(filename);
		g_free(filename);
		current_rmse[0] = - 1.0;
		gui_zoom_fit_to_window();
		gui_create_base_surface(0);
		gui_create_base_surface(1);
		gui_draw_and_show_window();		
	}
	gtk_widget_hide(file_load_dialog);
}

static void menu_item_clear_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_nu_image_sets < 2)
		return;
	destroy_image_set(1);
	current_nu_image_sets = 1;
	gui_zoom_fit_to_window();
	gui_draw_and_show_window();
}

static void menu_item_quit_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	gtk_main_quit();
	exit(0);
}

static void menu_item_flip_vertical_left_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	for (int i = 0; i < current_nu_mipmaps[0]; i++)
		flip_image_vertical(&current_image[0][i]);
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_remove_alpha_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].alpha_bits == 0) {
		popup_message("Image doesn't have alpha component.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		remove_alpha_from_image(&current_image[0][i]);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_add_alpha_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].alpha_bits > 0) {
		popup_message("Image already has an alpha component.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		add_alpha_to_image(&current_image[0][i]);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_from_half_float_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (!current_image[0][0].is_half_float) {
		popup_message("Image is not in half-float format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from half_float to RGB.
		convert_image_from_half_float(&current_image[0][i], 0, 1.0, 1.0);
		if (current_image[0][i].nu_components >= 3)
			// Convert to cairo format.
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_half_float_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float || (current_image[0][0].nu_components <= 2 &&
	current_image[0][0].is_signed)) {
		popup_message("Image is not in regular RGB format or unsigned 8-bit or 16-bit component format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		if (current_image[0][i].nu_components >= 3)
			// Convert from cairo format.
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert from RGB to half_float.
		convert_image_to_half_float(&current_image[0][i]);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_extend_half_float_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (!current_image[0][0].is_half_float || current_image[0][0].nu_components >= 3) {
		popup_message("Image is not in half-float format or already has three or more components.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		extend_half_float_image_to_rgb(&current_image[0][i]);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_r16_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 16-bit format.
		convert_image_to_16_bit_format(&current_image[0][i], 1, 0);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_rg16_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 16-bit format.
		convert_image_to_16_bit_format(&current_image[0][i], 2, 0);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_signed_r16_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 16-bit format.
		convert_image_to_16_bit_format(&current_image[0][i], 1, 1);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_signed_rg16_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 16-bit format.
		convert_image_to_16_bit_format(&current_image[0][i], 2, 1);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_r8_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format if necessary.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 8-bit integer format.
		convert_image_to_8_bit_format(&current_image[0][i], 1, 0);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_rg8_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format if necessary.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 8-bit integer format.
		convert_image_to_8_bit_format(&current_image[0][i], 2, 0);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_signed_r8_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].is_half_float) {
		popup_message("Image is not in 8-bit or 16-bit format.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format if necessary.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 8-bit integer format.
		convert_image_to_8_bit_format(&current_image[0][i], 1, 1);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}

static void menu_item_convert_to_signed_rg8_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active)
		return;
	if (current_image[0][0].bits_per_component != 8 &&
	!(current_image[0][0].bits_per_component == 16 && current_image[0][0].nu_components == 2
	&& current_image[0][0].is_signed)) {
		popup_message("Image is not in regular RGB format or signed rg16 format.");
		return;
	}
	if (current_image[0][0].nu_components == 1) {
		popup_message("Image has too few components.");
		return;
	}
	// Free pixel buffers associated with current_texture[0][i].
	if (current_file_type[0] & FILE_TYPE_TEXTURE_BIT)
		for (int i = 0; i < current_nu_mipmaps[0]; i++)
			free(current_texture[0][i].pixels);
	current_file_type[0] = FILE_TYPE_IMAGE_UNKNOWN;
	for (int i = 0; i < current_nu_mipmaps[0]; i++) {
		// Convert from cairo format if necessary.
		if (current_image[0][i].nu_components >= 3)
			convert_image_to_or_from_cairo_format(&current_image[0][i]);
		// Convert to 8-bit integer format.
		convert_image_to_8_bit_format(&current_image[0][i], 2, 1);
	}
	gui_create_base_surface(0);
	gui_draw_and_show_window();
}


static void update_base_surfaces_for_half_float() {
	int update = 0;
	if (current_nu_image_sets > 0)
		if (current_image[0][0].is_half_float) {
			gui_create_base_surface(0);
			update = 1;
		}
	if (current_nu_image_sets > 1)
		if (current_image[1][0].is_half_float) {
			gui_create_base_surface(1);
			update = 1;
		}
	if (update)
		gui_draw_and_show_window();
}

static GtkWidget *label_range;

static void update_range_label() {
	char s[40];
	sprintf(s, "Range: %.3f - %.3f", dynamic_range_min, dynamic_range_max);
	gtk_label_set_text(GTK_LABEL(label_range), s);
}

static void button_widen_range_clicked_cb(GtkButton *button, gpointer data) {
	dynamic_range_max += dynamic_range_max - dynamic_range_min;
	update_range_label();
	update_base_surfaces_for_half_float();
}

static void button_shorten_range_clicked_cb(GtkButton *button, gpointer data) {
	dynamic_range_max -= (dynamic_range_max - dynamic_range_min) / 2.0;
	update_range_label();
	update_base_surfaces_for_half_float();
}

static void button_move_range_left_clicked_cb(GtkButton *button, gpointer data) {
	float diff = dynamic_range_max - dynamic_range_min;
	dynamic_range_min -= diff / 4.0;
	dynamic_range_max -= diff / 4.0;
	update_range_label();
	update_base_surfaces_for_half_float();
}

static void button_move_range_right_clicked_cb(GtkButton *button, gpointer data) {
	float diff = dynamic_range_max - dynamic_range_min;
	dynamic_range_min += diff / 4.0;
	dynamic_range_max += diff / 4.0;
	update_range_label();
	update_base_surfaces_for_half_float();
}

static void button_fit_to_range_clicked_cb(GtkButton *button, gpointer data) {
	if (current_nu_image_sets < 1)
		return;
	calculate_image_dynamic_range(&current_image[0][0], &dynamic_range_min, &dynamic_range_max);
	update_range_label();
	update_base_surfaces_for_half_float();
}

static void button_reset_range_0_to_1_clicked_cb(GtkButton *button, gpointer data) {
	dynamic_range_min = 0;
	dynamic_range_max = 1.0;
	update_range_label();
	update_base_surfaces_for_half_float();
}

static GtkWidget *radio_button_gamma[2];

static void radio_button_gamma_toggled_cb(GtkRadioButton *button, gpointer data) {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button_gamma[0])))
		dynamic_range_gamma = 1.0;
	else
		dynamic_range_gamma = 2.2;
	update_base_surfaces_for_half_float();
}

GtkWidget *hdr_display_settings_dialog;

static void menu_item_hdr_display_settings_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	gtk_widget_show_all(hdr_display_settings_dialog);
	int r = gtk_dialog_run(GTK_DIALOG(hdr_display_settings_dialog));
	gtk_widget_hide(hdr_display_settings_dialog);
}

GtkWidget *compression_settings_dialog;
GtkWidget *speed_radio_button[4];
GtkWidget *combo_box_texture_format;
GtkWidget *update_frequency_radio_button[5];

static void menu_item_compression_settings_activate_cb(GtkMenuItem *menu_item, gpointer data) {
	if (compression_active) {
		popup_message("Cannot change compression settings while compression is being performed.");
		return;
	}
	gtk_widget_show_all(compression_settings_dialog);
	int r = gtk_dialog_run(GTK_DIALOG(compression_settings_dialog));
	gtk_widget_hide(compression_settings_dialog);
	if (r == GTK_RESPONSE_REJECT)
		return;
	for (int i = 0; i < 4; i++)
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(speed_radio_button[i])))
			option_speed = i;
	for (int i = 0; i < 5; i++)
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(update_frequency_radio_button[i])))
			compression_update_frequency_selection = i;
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box_texture_format));
	if (i != - 1)
		option_texture_format = match_texture_description(get_texture_format_index_text(i, 0))->type;
}

void gui_create_window_layout() {
    gtk_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gtk_window), "texview texture viewer");

    g_signal_connect(G_OBJECT(gtk_window), "delete_event", G_CALLBACK(delete_event_cb), NULL);

    g_signal_connect(G_OBJECT(gtk_window), "destroy", G_CALLBACK(destroy_cb), NULL);

    gtk_container_set_border_width(GTK_CONTAINER(gtk_window), 0);

    // Create the menu vbox for holding the menu and the rest of the application.
    GtkWidget *menu_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(gtk_window), menu_vbox);
    // Create the menu items for the Action menu.
    GtkWidget *menu_item_reset_zoom = gtk_menu_item_new_with_label("Reset zoom (fit to window)");
    g_signal_connect(G_OBJECT(menu_item_reset_zoom), "activate", G_CALLBACK(menu_item_reset_zoom_activate_cb), NULL);
    GtkWidget *menu_item_reset_zoom_real_size = gtk_menu_item_new_with_label("Reset zoom (real size)");
    g_signal_connect(G_OBJECT(menu_item_reset_zoom_real_size), "activate",
	G_CALLBACK(menu_item_reset_zoom_real_size_activate_cb), NULL);
    GtkWidget *menu_item_compress = gtk_menu_item_new_with_label("Compress");
    g_signal_connect(G_OBJECT(menu_item_compress), "activate",
	G_CALLBACK(menu_item_compress_activate_cb), NULL);
    GtkWidget *menu_item_stop_compression = gtk_menu_item_new_with_label("Stop compression (non-resumable)");
    g_signal_connect(G_OBJECT(menu_item_stop_compression), "activate",
	G_CALLBACK(menu_item_stop_compression_activate_cb), NULL);
    GtkWidget *menu_item_generate_mipmaps = gtk_menu_item_new_with_label("Generate mipmaps");
    g_signal_connect(G_OBJECT(menu_item_generate_mipmaps), "activate",
	G_CALLBACK(menu_item_generate_mipmaps_activate_cb), NULL);
    GtkWidget *menu_item_flip_vertical_right = gtk_menu_item_new_with_label("Flip result vertically (right image)");
    g_signal_connect(G_OBJECT(menu_item_flip_vertical_right), "activate",
		G_CALLBACK(menu_item_flip_vertical_right_activate_cb), NULL);
    GtkWidget *menu_item_load = gtk_menu_item_new_with_label("Load source (left image)");
    g_signal_connect(G_OBJECT(menu_item_load), "activate", G_CALLBACK(menu_item_load_activate_cb), NULL);
    GtkWidget *menu_item_save = gtk_menu_item_new_with_label("Save result (right image)");
    g_signal_connect(G_OBJECT(menu_item_save), "activate", G_CALLBACK(menu_item_save_activate_cb), NULL);
    GtkWidget *menu_item_clear = gtk_menu_item_new_with_label("Clear result (right image)");
    g_signal_connect(G_OBJECT(menu_item_clear), "activate", G_CALLBACK(menu_item_clear_activate_cb), NULL);
    GtkWidget *menu_item_quit = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(G_OBJECT(menu_item_quit), "activate", G_CALLBACK(menu_item_quit_activate_cb), NULL);
    GtkWidget *action_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_reset_zoom);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_reset_zoom_real_size);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_generate_mipmaps);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_compress);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_stop_compression);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_flip_vertical_right);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_load);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_clear);
    gtk_menu_shell_append(GTK_MENU_SHELL(action_menu), menu_item_quit);
    // Create the menu bar.
    GtkWidget *menu_bar = gtk_menu_bar_new();
    // Create the Action menu.
    GtkWidget *action_item = gtk_menu_item_new_with_label("Action");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(action_item), action_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), action_item);
    // Create the menu items for the Left image menu.
    GtkWidget *menu_item_flip_vertical_left = gtk_menu_item_new_with_label("Flip vertically");
    g_signal_connect(G_OBJECT(menu_item_flip_vertical_left), "activate",
		G_CALLBACK(menu_item_flip_vertical_left_activate_cb), NULL);
    GtkWidget *menu_item_remove_alpha = gtk_menu_item_new_with_label(
		"Remove alpha component");
    g_signal_connect(G_OBJECT(menu_item_remove_alpha), "activate",
		G_CALLBACK(menu_item_remove_alpha_activate_cb), NULL);
    GtkWidget *menu_item_add_alpha = gtk_menu_item_new_with_label(
		"Add alpha component (opaque)");
    g_signal_connect(G_OBJECT(menu_item_add_alpha), "activate",
		G_CALLBACK(menu_item_add_alpha_activate_cb), NULL);
    GtkWidget *menu_item_convert_from_half_float = gtk_menu_item_new_with_label("Convert from normalized "
		"half-float to 8-bit format");
    g_signal_connect(G_OBJECT(menu_item_convert_from_half_float), "activate",
		G_CALLBACK(menu_item_convert_from_half_float_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_half_float = gtk_menu_item_new_with_label("Convert to normalized "
		"half-float");
    g_signal_connect(G_OBJECT(menu_item_convert_to_half_float), "activate",
		G_CALLBACK(menu_item_convert_to_half_float_activate_cb), NULL);
    GtkWidget *menu_item_extend_half_float = gtk_menu_item_new_with_label("Extend R or RG half-float to RGB");
    g_signal_connect(G_OBJECT(menu_item_extend_half_float), "activate",
		G_CALLBACK(menu_item_extend_half_float_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_r16 = gtk_menu_item_new_with_label(
		"Convert image to one 16-bit component (red)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_r16), "activate",
		G_CALLBACK(menu_item_convert_to_r16_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_rg16 = gtk_menu_item_new_with_label(
		"Convert image to two 16-bit components (red and green)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_rg16), "activate",
		G_CALLBACK(menu_item_convert_to_rg16_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_signed_r16 = gtk_menu_item_new_with_label(
		"Convert image to one signed 16-bit component (red)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_signed_r16), "activate",
		G_CALLBACK(menu_item_convert_to_signed_r16_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_signed_rg16 = gtk_menu_item_new_with_label(
		"Convert image to two signed 16-bit components (red and green)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_signed_rg16), "activate",
		G_CALLBACK(menu_item_convert_to_signed_rg16_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_r8 = gtk_menu_item_new_with_label(
		"Convert image to one 8-bit component (red)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_r8), "activate",
		G_CALLBACK(menu_item_convert_to_r8_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_rg8 = gtk_menu_item_new_with_label(
		"Convert image to two 8-bit components (red and green)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_rg8), "activate",
		G_CALLBACK(menu_item_convert_to_rg8_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_signed_r8 = gtk_menu_item_new_with_label(
		"Convert image to one signed 8-bit component (red)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_signed_r8), "activate",
		G_CALLBACK(menu_item_convert_to_signed_r8_activate_cb), NULL);
    GtkWidget *menu_item_convert_to_signed_rg8 = gtk_menu_item_new_with_label(
		"Convert image to two signed 8-bit components (red and green)");
    g_signal_connect(G_OBJECT(menu_item_convert_to_signed_rg8), "activate",
		G_CALLBACK(menu_item_convert_to_signed_rg8_activate_cb), NULL);
    GtkWidget *left_image_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_flip_vertical_left);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_remove_alpha);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_add_alpha);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_from_half_float);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_half_float);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_extend_half_float);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_r16);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_rg16);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_signed_r16);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_signed_rg16);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_r8);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_rg8);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_signed_r8);
    gtk_menu_shell_append(GTK_MENU_SHELL(left_image_menu), menu_item_convert_to_signed_rg8);
    // Create the Left image menu.
    GtkWidget *left_image_item = gtk_menu_item_new_with_label("Source image");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(left_image_item), left_image_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), left_image_item);
    // Create the menu items for the Settings menu.
    GtkWidget *check_menu_item_show_difference = gtk_check_menu_item_new_with_label("Show difference in second image");
    g_signal_connect(G_OBJECT(check_menu_item_show_difference), "toggled",
		G_CALLBACK(check_menu_item_show_difference_toggled_cb), NULL);
    GtkWidget *check_menu_item_hdr = gtk_check_menu_item_new_with_label("Compress half-float as HDR");
    g_signal_connect(G_OBJECT(check_menu_item_hdr), "toggled",
		G_CALLBACK(check_menu_item_hdr_toggled_cb), NULL);
    GtkWidget *menu_item_hdr_display_settings = gtk_menu_item_new_with_label("HDR display settings");
    g_signal_connect(G_OBJECT(menu_item_hdr_display_settings), "activate",
	G_CALLBACK(menu_item_hdr_display_settings_activate_cb), NULL);
    GtkWidget *menu_item_compression_settings = gtk_menu_item_new_with_label("Compression settings");
    g_signal_connect(G_OBJECT(menu_item_compression_settings), "activate",
	G_CALLBACK(menu_item_compression_settings_activate_cb), NULL);
    GtkWidget *settings_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), check_menu_item_show_difference);
    gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), check_menu_item_hdr);
    gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), menu_item_hdr_display_settings);
    gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), menu_item_compression_settings);
    // Create the Settings menu.
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Settings");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(settings_item), settings_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), settings_item);
    // Add the whole menu.
    gtk_box_pack_start(GTK_BOX(menu_vbox), menu_bar, FALSE, FALSE, 0);

	// Create the HDR display settings dialog.
	hdr_display_settings_dialog = gtk_dialog_new_with_buttons("HDR Display Settings",
		GTK_WINDOW(gtk_window), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	label_range = gtk_label_new("Range: 0.000 - 1.000");
	GtkWidget *button_widen_range = gtk_button_new_with_label("Widen range");
	GtkWidget *button_shorten_range = gtk_button_new_with_label("Shorten range");
	GtkWidget *button_move_range_left = gtk_button_new_with_label("Move range left");
	GtkWidget *button_move_range_right = gtk_button_new_with_label("Move range right");
	GtkWidget *button_reset_range_0_to_1 = gtk_button_new_with_label("Reset range to 0.000 - 1.000");
	GtkWidget *button_fit_to_range = gtk_button_new_with_label("Fit range to range of left image");
	g_signal_connect(G_OBJECT(button_widen_range), "clicked",
		G_CALLBACK(button_widen_range_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button_shorten_range), "clicked",
		G_CALLBACK(button_shorten_range_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button_move_range_left), "clicked",
		G_CALLBACK(button_move_range_left_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button_move_range_right), "clicked",
		G_CALLBACK(button_move_range_right_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button_reset_range_0_to_1), "clicked",
		G_CALLBACK(button_reset_range_0_to_1_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button_fit_to_range), "clicked",
		G_CALLBACK(button_fit_to_range_clicked_cb), NULL);
	radio_button_gamma[0] = gtk_radio_button_new_with_label(NULL, "Gamma 1.0 (linear)");
	radio_button_gamma[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button_gamma[0])), "Gamma 2.2");
	g_signal_connect(G_OBJECT(radio_button_gamma[0]), "toggled",
		G_CALLBACK(radio_button_gamma_toggled_cb), NULL);
	g_signal_connect(G_OBJECT(radio_button_gamma[1]), "toggled",
		G_CALLBACK(radio_button_gamma_toggled_cb), NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button_gamma[0]), TRUE);
	GtkWidget *hdr_display_settings_content_area = gtk_dialog_get_content_area(
		GTK_DIALOG(hdr_display_settings_dialog));
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), label_range, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), button_widen_range, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), button_shorten_range, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), button_move_range_left, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), button_move_range_right, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), button_reset_range_0_to_1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), button_fit_to_range, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), radio_button_gamma[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hdr_display_settings_content_area), radio_button_gamma[1], FALSE, FALSE, 0);

	// Create the compression settings dialog.
	compression_settings_dialog = gtk_dialog_new_with_buttons("Compression Settings",
		GTK_WINDOW(gtk_window), /*GTK_DIALOG_MODAL | */ GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *label_speed = gtk_label_new("Compression speed:");
	speed_radio_button[0] = gtk_radio_button_new_with_label(NULL,
            "Ultra (lowest quality)");
	speed_radio_button[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(speed_radio_button[0])),
                    "Fast");
	speed_radio_button[2] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(speed_radio_button[0])),
                    "Medium");
	speed_radio_button[3] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(speed_radio_button[0])),
                    "Slow (highest quality)");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(speed_radio_button[0]), TRUE);
	option_speed = SPEED_ULTRA;
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(compression_settings_dialog));
	gtk_box_pack_start(GTK_BOX(content_area), label_speed, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), speed_radio_button[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), speed_radio_button[1], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), speed_radio_button[2], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), speed_radio_button[3], FALSE, FALSE, 0);
	GtkWidget *label_space = gtk_label_new("");
	GtkWidget *label_texture_format = gtk_label_new("Texture format:");
	combo_box_texture_format = gtk_combo_box_text_new();
	int n = get_number_of_texture_formats();
	int j;
	for (int i = 0; i < n; i++) {
		char s[80];
		strcpy(s, get_texture_format_index_text(i, 0));
		const char *text2 = get_texture_format_index_text(i, 1);
		if (strlen(text2) > 0) {
			strcat(s, " (");
			strcat(s, text2);
			strcat(s, ")");
		}
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box_texture_format), s);
		if (strcasecmp(get_texture_format_index_text(i, 0), "etc1") == 0)
			j = i;
	}
	gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(combo_box_texture_format), 5);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box_texture_format), j);
	gtk_box_pack_start(GTK_BOX(content_area), label_space, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), label_texture_format, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), combo_box_texture_format, FALSE, FALSE, 0);

	GtkWidget *label_space2 = gtk_label_new("");
	GtkWidget *label_update_frequency = gtk_label_new("Maximum GUI update frequency:");
	update_frequency_radio_button[0] = gtk_radio_button_new_with_label(NULL, "60 Hz");
	update_frequency_radio_button[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(update_frequency_radio_button[0])),
		"10 Hz");
	update_frequency_radio_button[2] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(update_frequency_radio_button[0])), "Every second");
	update_frequency_radio_button[3] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(update_frequency_radio_button[0])), "Every 10 seconds");
	update_frequency_radio_button[4] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(update_frequency_radio_button[0])), "Never");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(update_frequency_radio_button[
		compression_update_frequency_selection]), TRUE);
	gtk_box_pack_start(GTK_BOX(content_area), label_space2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), label_update_frequency, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), update_frequency_radio_button[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), update_frequency_radio_button[1], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), update_frequency_radio_button[2], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), update_frequency_radio_button[3], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), update_frequency_radio_button[4], FALSE, FALSE, 0);

	// Create the file save dialog.
	file_save_dialog = gtk_file_chooser_dialog_new("Select a filename to save to (extension: dds, ktx, pkm or png)",
		GTK_WINDOW(gtk_window), GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_save_dialog), TRUE);
	// Create the file load dialog.
	file_load_dialog = gtk_file_chooser_dialog_new("Select a texture or image file to load (extension: dds, ktx, pkm or png)",
		GTK_WINDOW(gtk_window), GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    window = (Window *)malloc(sizeof(Window));
    // Create a hbox.
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
 
    // Add the settings vbox to the hbox
//    gtk_box_pack_start(GTK_BOX(hbox), settings_vbox, FALSE, FALSE, 8);

	// Add a drawing area to the hbox.
	window->image_drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(window->image_drawing_area, window_width, window_height);
	gtk_box_pack_start(GTK_BOX(hbox), window->image_drawing_area, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(menu_vbox), hbox, TRUE, TRUE, 0);
	window->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, window_width, window_height);
	window->base_surface[0] = NULL;
	window->base_surface[1] = NULL;
    window->area_width = window_width;
    window->area_height = window_height;
#if GTK_MAJOR_VERSION == 2
	g_signal_connect(window->image_drawing_area, "expose-event", G_CALLBACK(area_expose_cb), window);
#else
	g_signal_connect(window->image_drawing_area, "draw", G_CALLBACK(area_draw_cb), window);
#endif
    g_signal_connect(window->image_drawing_area, "configure-event", G_CALLBACK(area_configure_event_cb), window);
    gtk_widget_add_events(window->image_drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);
    g_signal_connect(window->image_drawing_area, "button-press-event", G_CALLBACK(area_button_press_event_cb), window);
    g_signal_connect(window->image_drawing_area, "button-release-event", G_CALLBACK(area_button_release_event_cb), window);
    g_signal_connect(window->image_drawing_area, "scroll-event", G_CALLBACK(area_scroll_event_cb), window);

    gtk_widget_show_all(gtk_window);
    GUI_initialized = 1;
}

void gui_create_base_surface(int j) {
	if (j >= current_nu_image_sets)
		return;
	cairo_t *cr;
	Image difference_image[32];
	int nu_difference_mipmaps;
	// Calculate the size of the base surface.
	int base_surface_width;
	int base_surface_height;
	if (current_nu_mipmaps[j] > 1) {
		base_surface_width = current_image[j][0].width + 8 + current_image[j][0].width / 2;
		base_surface_height = current_image[j][0].height;
	}
	else {
		base_surface_width = current_image[j][0].width;
		base_surface_height = current_image[j][0].height;
	}
	// Create the base surface.
	if (window->base_surface[j] != NULL)
		cairo_surface_destroy(window->base_surface[j]);
	window->base_surface[j] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, base_surface_width,
		base_surface_height);
	// Draw on the base surface.
	cr = cairo_create(window->base_surface[j]);
	cairo_surface_t *image_surface;
	Image *source_image;
	Image cloned_image;
	int half_float_mode = 0;
	if (current_image[j][0].is_half_float)
		half_float_mode = 1;
	if (half_float_mode) {
		clone_image(&current_image[j][0], &cloned_image);
		convert_image_from_half_float(&cloned_image, dynamic_range_min, dynamic_range_max, dynamic_range_gamma);
		// Because internal half-float format is RGB, we need to to convert to cairo's ARGB8.
		convert_image_to_or_from_cairo_format(&cloned_image);
		source_image = &cloned_image;
	}
	else
	if (current_image[j][0].bits_per_component == 16) {
		clone_image(&current_image[j][0], &cloned_image);
		convert_image_from_16_bit_format(&cloned_image);
		// Because internal 16-bit format is RG or R, we need to to convert to cairo's ARGB8.
		convert_image_to_or_from_cairo_format(&cloned_image);
		source_image = &cloned_image;
	}
	else
	if (current_image[j][0].bits_per_component == 8 && current_image[j][0].nu_components <= 2) {
		clone_image(&current_image[j][0], &cloned_image);
		convert_image_from_8_bit_format(&cloned_image);
		convert_image_to_or_from_cairo_format(&cloned_image);
		source_image = &cloned_image;
	}
	else
		source_image = &current_image[j][0];
	if (difference_mode && j == 1) {
		nu_difference_mipmaps = current_nu_mipmaps[0];
		if (current_nu_mipmaps[1] < nu_difference_mipmaps)
			nu_difference_mipmaps = current_nu_mipmaps[1];
		calculate_difference_images(&current_image[0][0], &current_image[1][0],
			nu_difference_mipmaps, &difference_image[0]);
		image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)difference_image[0].pixels, CAIRO_FORMAT_ARGB32,
			difference_image[0].width, difference_image[0].height, difference_image[0].extended_width * 4);
	}
	else
		image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)source_image->pixels, CAIRO_FORMAT_ARGB32,
			current_image[j][0].width, current_image[j][0].height, current_image[j][0].extended_width * 4);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);
	double factor;
	factor = 1.0;
	cairo_rectangle(cr, 0, 0, current_image[j][0].width * factor,
		current_image[j][0].height * factor);
	cairo_scale(cr, factor, factor);
	cairo_set_source_surface(cr, image_surface, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	if (current_image[j][0].alpha_bits > 0)
		cairo_mask(cr, cairo_get_source(cr));
	else
		cairo_paint(cr);
	cairo_identity_matrix(cr);
	cairo_surface_destroy(image_surface);
	if (half_float_mode || current_image[j][0].bits_per_component == 16 ||
	(current_image[j][0].bits_per_component == 8 && current_image[j][0].nu_components <= 2))
		destroy_image(&cloned_image);
	double y = 0;
	double x = current_image[j][0].width * factor + 8;
	for (int i = 1; i < current_nu_mipmaps[j]; i++) {
		if (difference_mode && j == 1) {
			if (i >= nu_difference_mipmaps)
				break;
			image_surface = cairo_image_surface_create_for_data(
				(unsigned char *)difference_image[i].pixels, CAIRO_FORMAT_ARGB32,
				difference_image[i].width, difference_image[i].height,
				difference_image[i].extended_width * 4);
		}
		else {
			if (half_float_mode) {
				clone_image(&current_image[j][i], &cloned_image);
				convert_image_from_half_float(&cloned_image, dynamic_range_min, dynamic_range_max,
					dynamic_range_gamma);
				convert_image_to_or_from_cairo_format(&cloned_image);
				source_image = &cloned_image;
			}
			else
			if (current_image[j][0].bits_per_component == 16) {
				clone_image(&current_image[j][i], &cloned_image);
				convert_image_from_16_bit_format(&cloned_image);
				convert_image_to_or_from_cairo_format(&cloned_image);
				source_image = &cloned_image;
			}
			else
			if (current_image[j][0].bits_per_component == 8 && current_image[j][0].nu_components <= 2) {
				clone_image(&current_image[j][i], &cloned_image);
				convert_image_from_8_bit_format(&cloned_image);
				convert_image_to_or_from_cairo_format(&cloned_image);
				source_image = &cloned_image;
			}
			else
				source_image = &current_image[j][i];
			image_surface = cairo_image_surface_create_for_data(
				(unsigned char *)source_image->pixels, CAIRO_FORMAT_ARGB32,
				current_image[j][i].width, current_image[j][i].height,
				current_image[j][i].extended_width * 4);
		}
		cairo_rectangle(cr, x, y, current_image[j][i].width * factor, current_image[j][i].height * factor);
		cairo_scale(cr, factor, factor);
		cairo_set_source_surface(cr, image_surface, x / factor,	y / factor);
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
		if (current_image[j][i].alpha_bits > 0)
			cairo_mask(cr, cairo_get_source(cr));
		else
			cairo_paint(cr);
		cairo_identity_matrix(cr);
		cairo_surface_destroy(image_surface);
		if ((half_float_mode || current_image[j][0].bits_per_component == 16 || 
		(current_image[j][0].bits_per_component == 8 && current_image[j][0].nu_components <= 2)) &&
		!(difference_mode && j == 1))
			destroy_image(&cloned_image);
		y += current_image[j][i].height * factor;
	}
	cairo_destroy(cr);

	if (difference_mode && j == 1) {
		// Destoy difference images.
		for (int i = 0; i < nu_difference_mipmaps; i++)
			destroy_image(&difference_image[i]);
	}
}

static char *get_file_type_text(int type) {
	switch (type) {
	case FILE_TYPE_PNG :
		return "PNG";
	case FILE_TYPE_KTX :
		return "KTX";
	case FILE_TYPE_DDS :
		return "DDS";
	case FILE_TYPE_PKM :
		return "PKM";
	case FILE_TYPE_ASTC :
		return "ASTC";
	case FILE_TYPE_UNDEFINED :
		return "TEX";
	case FILE_TYPE_IMAGE_UNKNOWN :
		return "IMG";
	}
}

void gui_draw_window() {
	cairo_t *cr;
	cr = cairo_create(window->surface);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);
	// Calculate the total size of vertical borders.
	double borders_x = 16 + SET_BORDER_WIDTH * (current_nu_image_sets - 1);
	// Draw the text at the top.
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 20);
	cairo_set_source_rgb(cr, 1, 1, 1);
	for (int i = 0 ; i < current_nu_image_sets; i++) {
		char *type_text = get_file_type_text(current_file_type[i]);
		char format_text[32];
		if ((current_file_type[i] & FILE_TYPE_TEXTURE_BIT) || current_file_type[i] == FILE_TYPE_UNDEFINED) {
			strcpy(format_text, texture_type_text(current_texture[i][0].type));
			strcat(format_text, " ");
		}
		else
			if (current_image[i][0].is_half_float) {
				if (current_image[i][0].nu_components == 2)
					strcpy(format_text, "rg_half_float");
				else
				if (current_image[i][0].nu_components == 1)
					strcpy(format_text, "r_half_float");
				else
				if (current_image[i][0].alpha_bits > 0)
					strcpy(format_text, "rgba_half_float");
				else
					strcpy(format_text, "rgb_half_float");
			}
			else
			if (current_image[i][0].bits_per_component == 16) {
				if (current_image[i][0].nu_components == 2)
					if (current_image[i][0].is_signed)
						strcpy(format_text, "signed rg16");
					else
						strcpy(format_text, "rg16");
				else
					if (current_image[i][0].is_signed)
						strcpy(format_text, "signed r16");
					else
						strcpy(format_text, "r16");
			}
			else
			if (current_image[i][0].nu_components == 2)
				if (current_image[i][0].is_signed)
					strcpy(format_text, "signed rg8");
				else
					strcpy(format_text, "rg8");
			else
			if (current_image[i][0].nu_components == 1)
				if (current_image[i][0].is_signed)
					strcpy(format_text, "signed r8");
				else
					strcpy(format_text, "r8");
			else
			if (current_image[i][0].alpha_bits > 0)
				strcpy(format_text, "rgba8");
			else
				strcpy(format_text, "rgb8");
		char mipmap_text[32];
		if (current_nu_mipmaps[i] > 1)
			sprintf(mipmap_text, "Levels: %d", current_nu_mipmaps[i]);
		else
			mipmap_text[0] = '\0';
		char *active_text;
		if (compression_active && i == 1)
			active_text = " Compressing...";
		else
			active_text = "";
		char rmse_text[32];
		if (i == 1 && !compression_active && current_rmse[0] >= 0)
			sprintf(rmse_text, " RMSE %.3lf", current_rmse[0]);
		else
			rmse_text[0] = '\0';
        	char s[80];
		sprintf(s, "%s %s %dx%d %s%s%s", type_text, format_text,
			current_image[i][0].width, current_image[i][0].height, mipmap_text, active_text, rmse_text);
		cairo_move_to(cr, 8 + ((window->area_width - borders_x) / current_nu_image_sets + SET_BORDER_WIDTH) * i,
			26);
	        cairo_show_text(cr, s);
	}

	double x = 8;
	for (int j = 0; j < current_nu_image_sets; j++) {
		cairo_identity_matrix(cr);
		// Draw from the base surface image, leaving some borders.
		cairo_rectangle(cr, x, 32, (window->area_width - borders_x) / current_nu_image_sets,
			window->area_height - 32 - 8);
		cairo_scale(cr, zoom, zoom);
		cairo_set_source_surface(cr, window->base_surface[j], x / zoom + zoom_origin_x / zoom,
			32 / zoom + zoom_origin_y / zoom);
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
		cairo_fill(cr);
		// Draw crosshairs
		cairo_identity_matrix(cr);
		cairo_set_source_rgb(cr, 1, 1, 1);
		double center_x = x + ((window->area_width - borders_x) / current_nu_image_sets) / 2;
		double center_y = 32 + (window->area_height - 32 - 8) / 2;
		cairo_move_to(cr, center_x + 8, center_y);
		cairo_arc(cr, center_x, center_y, 8, 0, 2 * M_PI);
		cairo_stroke(cr);
		x += (window->area_width - borders_x) / current_nu_image_sets + SET_BORDER_WIDTH;
		// Draw partitioning line 
		if (j < current_nu_image_sets - 1) {
			cairo_move_to(cr, x - SET_BORDER_WIDTH / 2, 32);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_line_to(cr, x - SET_BORDER_WIDTH / 2, window->area_height);
			cairo_stroke(cr);
		}
		// Display ETA when compressing.
		if (compression_active && j == 1 && compress_progress > 0.000001f) {
			cairo_move_to(cr, 8 + ((window->area_width - borders_x) / current_nu_image_sets
				+ SET_BORDER_WIDTH) * j, window->area_height - 6);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			double current_time = tv.tv_sec + tv.tv_usec * 0.000001;
			int eta = (int)((1 - compress_progress) *
				(current_time - compress_start_time) / compress_progress);
			int hours = eta / 3600;
			int minutes = eta / 60 - hours * 60;
			int seconds = eta - hours * 3600 - minutes * 60;
			char s[64];
			if (hours >= 1.0)
				sprintf(s, "ETA %dh %dm %ds", hours, minutes, seconds);
			else if (minutes >= 1.0)
				sprintf(s, "ETA %dm %ds", minutes, seconds);
			else
				sprintf(s, "ETA %ds", seconds);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_show_text(cr, s);
                }
	}

	cairo_destroy(cr);
}

void gui_draw_and_show_window() {
    gui_draw_window();
    gui_update_drawing_area(window);
}

void gui_run() {
    gtk_main();
}

void gui_handle_events() {
    while (gtk_events_pending())
        gtk_main_iteration();
}


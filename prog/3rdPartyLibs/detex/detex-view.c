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

/* Texture file viewer. */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "detex.h"

#define DEFAULT_WINDOW_WIDTH 512
#define DEFAULT_WINDOW_HEIGHT 540

static GtkWidget *gtk_window;
static cairo_surface_t *area_surface = NULL;
static int area_width, area_height;
static GtkWidget *texture_label;
static detexTexture *texture = NULL;
static uint8_t *pixel_buffer;	// Decompressed pixel buffer.

static void DrawTexture(detexTexture *texture, uint8_t *pixel_buffer);

static gboolean delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
	return FALSE;
}

static void destroy_cb(GtkWidget *widget, gpointer data) {
	gtk_main_quit();
	exit(0);
}

static gboolean window_key_press_cb(GtkWidget *widget, GdkEventKey *event, void *user_data) {
	if (event->keyval == GDK_KEY_Q || event->keyval == GDK_KEY_q)
		exit(0);
	return TRUE;
}

static gboolean area_configure_event_cb(GtkWidget *widget, GdkEventConfigure *event,
GtkWidget *image_drawing_area) {
	if (area_surface != NULL)
		cairo_surface_destroy(area_surface);
	area_width = event->width;
	area_height = event->height;
	area_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		area_width, area_height);
    	DrawTexture(texture, pixel_buffer);
	gtk_widget_show_all(gtk_window);
	// We've handled the configure event, no need for further processing.
	return TRUE;
}

static gboolean area_draw_cb(GtkWidget *widget, cairo_t *cr, void *user_data) {
	cairo_set_source_surface(cr, area_surface, 0, 0);
	cairo_paint(cr);
	return TRUE;
}

static void CreateWindowLayout() {
	gtk_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gtk_window),
		"Detex Library Texture Viewer");
	// Figure out the screen dimensions.
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(gtk_window));
	gint monitor = gdk_screen_get_monitor_at_window(screen, gdk_screen_get_active_window(screen));
	GdkRectangle monitor_rect;
	gdk_screen_get_monitor_geometry(screen, monitor, &monitor_rect);
	int width = DEFAULT_WINDOW_WIDTH;
	int height = DEFAULT_WINDOW_HEIGHT;
	if (texture->width > DEFAULT_WINDOW_WIDTH && texture->width + 16 <= monitor_rect.width)
		width = texture->width;
	if (texture->height > DEFAULT_WINDOW_HEIGHT && texture->height + 32 <= monitor_rect.height)
		height = texture->height;
	gtk_window_set_default_size(GTK_WINDOW(gtk_window), width, height);
	g_signal_connect(G_OBJECT(gtk_window), "delete_event",
		G_CALLBACK(delete_event_cb), NULL);
	g_signal_connect(G_OBJECT(gtk_window), "destroy",
		G_CALLBACK(destroy_cb), NULL);
	g_signal_connect(G_OBJECT(gtk_window), "key-press-event",
		G_CALLBACK(window_key_press_cb), NULL);
	gtk_widget_add_events(gtk_window, GDK_KEY_PRESS_MASK);
	gtk_container_set_border_width(GTK_CONTAINER(gtk_window), 0);
	GtkWidget *image_drawing_area = gtk_drawing_area_new();
	// Create a vbox for texture image and label.
	GtkWidget *texture_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(texture_vbox), FALSE);
	gtk_box_pack_start(GTK_BOX(texture_vbox), image_drawing_area, TRUE, TRUE, 0);
	texture_label = gtk_label_new("Unknown");
	gtk_box_pack_start(GTK_BOX(texture_vbox), texture_label, FALSE, FALSE, 0);
	// Add the texture vbox to the lay-out.
	gtk_container_add(GTK_CONTAINER(gtk_window), texture_vbox);
	g_signal_connect(image_drawing_area, "draw",
		G_CALLBACK(area_draw_cb), NULL);
	g_signal_connect(image_drawing_area, "configure-event",
		G_CALLBACK(area_configure_event_cb), image_drawing_area);
	gtk_widget_show_all(gtk_window);
}

static double CalculateZoomFactor() {
	double factorx = (double)area_width / texture->width;
	double factory = (double)area_height / texture->height;
	if (factorx < factory)
		return factorx;
	else
		return factory;
}

static void DrawTexture(detexTexture *texture, uint8_t *pixel_buffer) {
	cairo_t *cr = cairo_create(area_surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, 0.0d, 0.0d, 0.0d, 1.0d);
	cairo_paint(cr);
	if (texture == NULL) {
		cairo_destroy(cr);
		return;
	}
	cairo_surface_t *image_surface = cairo_image_surface_create_for_data(
		(unsigned char *)pixel_buffer, CAIRO_FORMAT_ARGB32,
		texture->width, texture->height, texture->width * 4);
	double zoom_factor = CalculateZoomFactor();
	cairo_rectangle(cr, 0, 0, zoom_factor * texture->width, zoom_factor * texture->height);
	if (detexFormatHasAlpha(texture->format)) {
		cairo_set_source_rgba(cr, 0.0d, 0.0d, 0.5d, 1.0d);
		cairo_fill(cr);
	}
        cairo_scale(cr, zoom_factor, zoom_factor);
	cairo_set_source_surface(cr, image_surface, 0.0d, 0.0d);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	cairo_mask(cr, cairo_get_source(cr));
	cairo_destroy(cr);
	cairo_surface_destroy(image_surface);
}

static bool LoadCompressedTexture(const char *filename) {
	return detexLoadTextureFile(filename, &texture);
}

int main(int argc, char **argv) {
	gtk_init(&argc, &argv);
	if (argc < 2) {
		printf("Syntax: detex-view <texture filename>\n");
		exit(0);
	}
	const char *filename = argv[1];
	bool r = LoadCompressedTexture(filename);
	if (!r) {
		printf("%s\n", detexGetErrorMessage());
		exit(1);
	}
	CreateWindowLayout();
	char *label_text;
	asprintf(&label_text, "File: %s  Size: %dx%d  Format: %s",
		filename, texture->width, texture->height,
		detexGetTextureFormatText(texture->format));
	gtk_label_set_text(GTK_LABEL(texture_label), label_text);
	uint32_t pixel_format;
	// Convert to a format suitable for Cairo.
	if (detexFormatHasAlpha(texture->format))
		pixel_format = DETEX_PIXEL_FORMAT_BGRA8;
	else
		pixel_format = DETEX_PIXEL_FORMAT_BGRX8;
	pixel_buffer = (uint8_t *)malloc(texture->width * texture->height *
		detexGetPixelSize(pixel_format));
	r = detexDecompressTextureLinear(texture, pixel_buffer,
		pixel_format);
	if (!r) {
		printf("Decompression of %s returned error:\n%s\n",
			filename, detexGetErrorMessage());
		exit(1);
	}
	DrawTexture(texture, pixel_buffer);
	gtk_main();
}


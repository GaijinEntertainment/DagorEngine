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

/* Test/validation program. */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "detex.h"

#define TEXTURE_WIDTH 64
#define TEXTURE_HEIGHT 64

static const char *texture_file[] = {
	"test-texture-BC1.ktx",
	"test-texture-BC1A.ktx",
	"test-texture-BC2.ktx",
	"test-texture-BC3.ktx",
	"test-texture-RGTC1.ktx",
	"test-texture-RGTC2.ktx",
	"test-texture-SIGNED_RGTC1.ktx",
	"test-texture-SIGNED_RGTC2.ktx",
	"test-texture-BPTC.ktx",
	"test-texture-BPTC_FLOAT.ktx",
	"test-texture-BPTC_SIGNED_FLOAT.ktx",
	"test-texture-ETC1.ktx",
	"test-texture-ETC2.ktx",
	"test-texture-ETC2_PUNCHTHROUGH.ktx",
	"test-texture-ETC2_EAC.ktx",
	"test-texture-EAC_R11.ktx",
	"test-texture-EAC_RG11.ktx",
	"test-texture-EAC_SIGNED_R11.ktx",
	"test-texture-EAC_SIGNED_RG11.ktx",
	"test-texture-RGB8.ktx",
	"test-texture-RGBA8.ktx",
	"test-texture-FLOAT_RGB16.ktx",
	"test-texture-FLOAT_RGBA16.ktx",
	"test-texture-RGB8.dds",
	"test-texture-RGBA8.dds",
};

#define NU_TEXTURE_FORMATS_FROM_FILE (sizeof(texture_file) / sizeof(texture_file[0]))
#define NU_TEXTURES (NU_TEXTURE_FORMATS_FROM_FILE + 4)

static GtkWidget *gtk_window;
cairo_surface_t *surface[NU_TEXTURES];
GtkWidget *texture_label[NU_TEXTURES];
uint8_t *pixel_buffer[NU_TEXTURES];
detexTexture *texture[NU_TEXTURES];

static gboolean delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data) {
    return FALSE;
}

static void destroy_cb(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
    exit(0);
}

static gboolean area_draw_cb(GtkWidget *widget, cairo_t *cr, cairo_surface_t *surface) {
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    return TRUE;
}

static void CreateWindowLayout() {
	gtk_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(gtk_window),
		"Detex Library Validation Program");
	g_signal_connect(G_OBJECT(gtk_window), "delete_event",
		G_CALLBACK(delete_event_cb), NULL);
	g_signal_connect(G_OBJECT(gtk_window), "destroy",
		G_CALLBACK(destroy_cb), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(gtk_window), 0);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(gtk_window), vbox);
	GtkWidget *hbox;
	for (int i = 0; i < NU_TEXTURES; i++) {
		if (i % 8 == 0) {
			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_container_add(GTK_CONTAINER(vbox), hbox);
		}
		GtkWidget *image_drawing_area = gtk_drawing_area_new();
		gtk_widget_set_size_request(image_drawing_area,
			TEXTURE_WIDTH, TEXTURE_HEIGHT);
		// Create a vbox for texture image and label.
		GtkWidget *texture_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start(GTK_BOX(texture_vbox), image_drawing_area, TRUE, TRUE, 0);
		texture_label[i] = gtk_label_new("Unknown");
		gtk_box_pack_start(GTK_BOX(texture_vbox), texture_label[i], TRUE, TRUE, 0);
		// Add the texture vbox to the lay-out.
		gtk_box_pack_start(GTK_BOX(hbox), texture_vbox, TRUE, FALSE, 8);
		surface[i] = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			TEXTURE_WIDTH, TEXTURE_HEIGHT);
		g_signal_connect(image_drawing_area, "draw",
			G_CALLBACK(area_draw_cb), surface[i]);
	}

	gtk_widget_show_all(gtk_window);
}

static void DrawTexture(int i) {
	cairo_surface_t *image_surface = cairo_image_surface_create_for_data(
		(unsigned char *)pixel_buffer[i], CAIRO_FORMAT_ARGB32,
		TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_WIDTH * 4);
	cairo_t *cr = cairo_create(surface[i]);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, 0.0d, 0.0d, 0.5d, 1.0d);
	cairo_paint(cr);
	cairo_set_source_surface(cr, image_surface, 0.0d, 0.0d);
	cairo_mask_surface(cr, image_surface, 0.0d, 0.0d);
	cairo_destroy(cr);
	cairo_surface_destroy(image_surface);
}

static bool LoadTexture(int i) {
	return detexLoadTextureFile(texture_file[i], &texture[i]);
}

static void CreateHDRTextures() {
	int i = NU_TEXTURE_FORMATS_FROM_FILE;
	texture[i] = (detexTexture *)malloc(sizeof(detexTexture));
	texture[i]->format = DETEX_PIXEL_FORMAT_FLOAT_RGB16;
	texture[i]->data = (uint8_t *)malloc(detexGetPixelSize(DETEX_PIXEL_FORMAT_FLOAT_RGB16) *
		TEXTURE_WIDTH * TEXTURE_HEIGHT);
	texture[i]->width = TEXTURE_WIDTH;
	texture[i]->height = TEXTURE_HEIGHT;
	texture[i]->width_in_blocks = TEXTURE_WIDTH;
	texture[i]->height_in_blocks = TEXTURE_HEIGHT;
	float *float_buffer = (float *)malloc(sizeof(float) * 3 * TEXTURE_WIDTH * TEXTURE_HEIGHT);
	for (int y = 0; y < TEXTURE_HEIGHT; y++)
		for (int x = 0; x < TEXTURE_WIDTH; x++) {
			float c = (x + y) / (float)(TEXTURE_WIDTH + TEXTURE_HEIGHT - 2);
			float_buffer[(y * TEXTURE_WIDTH + x) * 3] = c;
			float_buffer[(y * TEXTURE_WIDTH + x) * 3 + 1] = c;
			float_buffer[(y * TEXTURE_WIDTH + x) * 3 + 2] = c;
		}
	bool r = detexConvertPixels((uint8_t *)float_buffer, TEXTURE_WIDTH * TEXTURE_HEIGHT,
		DETEX_PIXEL_FORMAT_FLOAT_RGB32,	texture[i]->data, DETEX_PIXEL_FORMAT_FLOAT_RGB16);
	if (!r)
		printf("Error converting float to half-float:\n%s\n", detexGetErrorMessage());
	texture[i + 1] = (detexTexture *)malloc(sizeof(detexTexture));
	texture[i + 1]->format = DETEX_PIXEL_FORMAT_FLOAT_RGB32;
	texture[i + 1]->data = (uint8_t *)float_buffer;
	texture[i + 1]->width = TEXTURE_WIDTH;
	texture[i + 1]->height = TEXTURE_HEIGHT;
	texture[i + 1]->width_in_blocks = TEXTURE_WIDTH;
	texture[i + 1]->height_in_blocks = TEXTURE_HEIGHT;
	// Create copies with HDR format (sharing the pixel buffer);
	texture[i + 2] = (detexTexture *)malloc(sizeof(detexTexture));
	*texture[i + 2] = *texture[i];
	texture[i + 2]->format = DETEX_PIXEL_FORMAT_FLOAT_RGB16_HDR;
	texture[i + 3] = (detexTexture *)malloc(sizeof(detexTexture));
	*texture[i + 3] = *texture[i + 1];
	texture[i + 3]->format = DETEX_PIXEL_FORMAT_FLOAT_RGB32_HDR;
}

static void ConvertHDRTextures() {
	detexSetHDRParameters(1.0f, 0.0f, 2.0f);
	for (int i = NU_TEXTURE_FORMATS_FROM_FILE; i < NU_TEXTURES; i++) {
		pixel_buffer[i] = (uint8_t *)malloc(4 * TEXTURE_WIDTH * TEXTURE_HEIGHT);
		bool r = detexDecompressTextureLinear(texture[i], pixel_buffer[i], DETEX_PIXEL_FORMAT_BGRX8);
		if (!r) {
			printf("Decompression of format %s returned error:\n%s\n",
				detexGetTextureFormatText(texture[i]->format), detexGetErrorMessage());
		}
	}
}

int main(int argc, char **argv) {
	gtk_init(&argc, &argv);
	CreateWindowLayout();
	for (int i = 0; i < NU_TEXTURE_FORMATS_FROM_FILE; i++) {
		pixel_buffer[i] = (uint8_t *)malloc(16 * 4 *
			(TEXTURE_WIDTH / 4) * (TEXTURE_HEIGHT / 4));
		bool r = LoadTexture(i);
		if (!r) {
			printf("%s\n", detexGetErrorMessage());
			memset(pixel_buffer[i], 0, 16 * 4 *
				(TEXTURE_WIDTH / 4) * (TEXTURE_HEIGHT / 4));
			continue;
		}
		gtk_label_set_text(GTK_LABEL(texture_label[i]), detexGetTextureFormatText(texture[i]->format));
		uint32_t pixel_format;
		// Convert to a format suitable for Cairo.
		if (detexFormatHasAlpha(texture[i]->format))
			pixel_format = DETEX_PIXEL_FORMAT_BGRA8;
		else
			pixel_format = DETEX_PIXEL_FORMAT_BGRX8;
		r = detexDecompressTextureLinear(texture[i], pixel_buffer[i],
			pixel_format);
		if (!r) {
			printf("Decompression of %s returned error:\n%s\n",
				texture_file[i], detexGetErrorMessage());
			continue;
		}
	}
	CreateHDRTextures();
	ConvertHDRTextures();
	for (int i = NU_TEXTURE_FORMATS_FROM_FILE; i < NU_TEXTURES; i++)
		gtk_label_set_text(GTK_LABEL(texture_label[i]), detexGetTextureFormatText(texture[i]->format));
	for (int i = 0; i < NU_TEXTURES; i++)
		DrawTexture(i);
	gtk_main();
}


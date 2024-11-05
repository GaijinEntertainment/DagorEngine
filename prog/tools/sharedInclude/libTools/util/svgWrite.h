//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifndef __GAIJIN_SVG_DEBUG_H__
#define __GAIJIN_SVG_DEBUG_H__

#include <stdio.h>

class Point2;

FILE *svg_open(const char *fname, int w = -1, int h = -1);
void svg_begin_group(FILE *fp, const char *attr);
void svg_end_group(FILE *fp);
void svg_draw_face(FILE *fp, const Point2 &v0, const Point2 &v1, const Point2 &v2);
void svg_draw_line(FILE *fp, const Point2 &v0, const Point2 &v1, const char *attr = NULL);
void svg_draw_circle(FILE *fp, const Point2 &c, float rad = 1.0f);
void svg_draw_number(FILE *fp, const Point2 &c, int num);

void svg_start_poly(FILE *fp, const Point2 &v);
void svg_add_poly_point(FILE *fp, const Point2 &v);
void svg_end_poly(FILE *fp);

void svg_close(FILE *fp);

#endif

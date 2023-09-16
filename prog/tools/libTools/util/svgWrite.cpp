#include <libTools/util/svgWrite.h>
#include <math/dag_math3d.h>

//
// Svg writer stuff
//
FILE *svg_open(const char *fname, int w, int h)
{
  FILE *fp = fopen(fname, "wt");
  if (!fp)
    return fp;
  if (w > 0 && h > 0)
    fprintf(fp, "<svg width=\"%dpx\" height=\"%dpx\">\n", w, h);
  else
    fprintf(fp, "<svg>\n");

  return fp;
}
void svg_begin_group(FILE *fp, const char *attr)
{
  if (fp)
    fprintf(fp, "<g %s>\n", attr);
}
void svg_end_group(FILE *fp)
{
  if (fp)
    fprintf(fp, "</g>\n");
}
void svg_draw_face(FILE *fp, const Point2 &v0, const Point2 &v1, const Point2 &v2)
{
  if (fp)
    fprintf(fp, "<polyline points=\"%.3f,%.3f %.3f,%.3f %.3f,%.3f %.3f,%.3f\" />\n", v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, v0.x, v0.y);
}
void svg_draw_line(FILE *fp, const Point2 &v0, const Point2 &v1, const char *attr)
{
  if (fp)
    fprintf(fp, "<line x1=\"%.3f\" y1=\"%.3f\" x2=\"%.3f\" y2=\"%.3f\" %s />\n", v0.x, v0.y, v1.x, v1.y, attr ? attr : "");
}
void svg_draw_number(FILE *fp, const Point2 &c, int num)
{
  if (fp)
    fprintf(fp,
      "<text style=\"fill:black; font-family:Verdana; font-size:9; text-anchor:middle;\""
      " x=\"%.3f\" y=\"%.3f\" > %d </text>\n",
      c.x, c.y, num);
}

void svg_start_poly(FILE *fp, const Point2 &v)
{
  if (fp)
    fprintf(fp, "<polyline points=\"%.3f,%.3f", v.x, v.y);
}
void svg_add_poly_point(FILE *fp, const Point2 &v)
{
  if (fp)
    fprintf(fp, " %.3f,%.3f", v.x, v.y);
}
void svg_end_poly(FILE *fp)
{
  if (fp)
    fprintf(fp, "\" />\n");
}

void svg_close(FILE *fp)
{
  if (!fp)
    return;
  fprintf(fp, "</svg>");
  fclose(fp);
}

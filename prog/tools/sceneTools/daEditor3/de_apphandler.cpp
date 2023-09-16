#include "de_apphandler.h"
#include "de_plugindata.h"
#include <oldEditor/de_cm.h>
#include <sepGui/wndGlobal.h>
#include <libTools/util/strUtil.h>

#include <gui/dag_stdGuiRenderEx.h>
#include <3d/dag_render.h>
#include <math/dag_mathAng.h>

#include <debug/dag_debug.h>

#include <stdio.h>

static TEXTUREID compass_tid = BAD_TEXTUREID, compass_nesw_tid = BAD_TEXTUREID;

static Point2 getCameraAngles()
{
  Point2 ang = dir_to_angles(::grs_cur_view.itm.getcol(2));
  if (ang.y > DegToRad(85))
    ang.x = dir_to_angles(-::grs_cur_view.itm.getcol(1)).x;
  else if (ang.y < -DegToRad(85))
    ang.x = dir_to_angles(::grs_cur_view.itm.getcol(1)).x;
  return ang;
}

DagorEdAppEventHandler::DagorEdAppEventHandler(GenericEditorAppWindow &m) :
  GenericEditorAppWindow::AppEventHandler(m), showCompass(false)
{}
DagorEdAppEventHandler::~DagorEdAppEventHandler()
{
  release_managed_tex(compass_tid);
  release_managed_tex(compass_nesw_tid);
}

//==============================================================================
void DagorEdAppEventHandler::handleViewportPaint(IGenViewportWnd *wnd)
{
  if (compass_tid == BAD_TEXTUREID)
  {
    compass_tid = add_managed_texture(::make_full_path(sgg::get_exe_path_full(), "../commonData/tex/compass.tga"));
    compass_nesw_tid = add_managed_texture(::make_full_path(sgg::get_exe_path_full(), "../commonData/tex/compass_nesw.tga"));
    acquire_managed_tex(compass_tid);
    acquire_managed_tex(compass_nesw_tid);
  }

  __super::handleViewportPaint(wnd);


  IGenEditorPlugin *plug = DAGORED2->curPlugin();
  int vpW, vpH;
  wnd->getViewportSize(vpW, vpH);

  if (plug && !plug->getVisible())
  {
    char mess1[100];
    _snprintf(mess1, 99, " Caution: \"%s\" visibility is off \n", plug->getMenuCommandName());
    char mess2[100] = " Press F12 key to change plugin visibility ";

    StdGuiRender::set_font(0);
    Point2 ts1 = StdGuiRender::get_str_bbox(mess1).size();
    Point2 ts2 = StdGuiRender::get_str_bbox(mess2).size();

    int width = ts1.x > ts2.x ? ts1.x : ts2.x;
    int left = vpW / 2 - width / 2;
    int top = vpH / 2 - ts2.y / 2;

    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::render_box(left, top + ts2.y + 8, left + width, top - ts1.y);

    StdGuiRender::set_color(COLOR_WHITE);
    StdGuiRender::draw_strf_to(left + (width - ts1.x) / 2, top, mess1);
    StdGuiRender::draw_strf_to(left + (width - ts2.x) / 2, top + ts2.y + 2, mess2);
  }

  if (showCompass)
  {
    Point3 toNorth = ::grs_cur_view.pos + Point3(0, 0, 50);
    Point2 cp;

    if (!wnd->isOrthogonal() && wnd->worldToClient(toNorth, cp))
    {
      // show NORTH marker
      StdGuiRender::set_color(COLOR_WHITE);
      StdGuiRender::draw_line(cp.x - 20, cp.y - 20, cp.x + 20, cp.y + 20);
      StdGuiRender::draw_line(cp.x + 20, cp.y - 20, cp.x - 20, cp.y + 20);

      cp += Point2(2, 1);
      StdGuiRender::set_color(COLOR_BLACK);
      StdGuiRender::draw_line(cp.x - 20, cp.y - 20, cp.x + 20, cp.y + 20);
      cp.y -= 1;
      StdGuiRender::draw_line(cp.x + 20, cp.y - 20, cp.x - 20, cp.y + 20);
    }

    if (!wnd->isOrthogonal())
    {
      // show CENTER-of-SCREEN marker
      cp.set(vpW / 2, vpH / 2);
      StdGuiRender::set_color(COLOR_BLACK);
      StdGuiRender::draw_line(cp.x + 1 - 6, cp.y + 1, cp.x + 1 + 5, cp.y + 1);
      StdGuiRender::draw_line(cp.x + 1, cp.y + 1 - 6, cp.x + 1, cp.y + 1 + 5);

      StdGuiRender::set_color(COLOR_WHITE);
      StdGuiRender::draw_line(cp.x - 6, cp.y, cp.x + 5, cp.y);
      StdGuiRender::draw_line(cp.x, cp.y - 6, cp.x, cp.y + 5);
    }


    // render compass widget
    Point2 ang = getCameraAngles();
    float alpha = -ang.x - HALFPI;
    Point2 c(vpW - 132 + 64, vpH - 132 + 64), c2, ts;
    float r = 64;
    float sa, ca;
    sincos(alpha, sa, ca);

    //  rotatable compass core
    StdGuiRender::set_color(COLOR_WHITE);
    StdGuiRender::set_texture(compass_tid);
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);
    StdGuiRender::render_quad(c + r * Point2(+sa - ca, -ca - sa), c + r * Point2(-sa - ca, +ca - sa),
      c + r * Point2(-sa + ca, +ca + sa), c + r * Point2(+sa + ca, -ca + sa));

    StdGuiRender::set_texture(compass_nesw_tid);
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);

    //  world directions
    r = 54;
    sincos(-alpha + PI, sa, ca);
    c2 = c + Point2(r * sa, r * ca);
    StdGuiRender::render_rect(c2.x - 10, c2.y - 10, c2.x + 10, c2.y + 10, Point2(0.00, 0), Point2(0.25, 0), Point2(0, 1)); // North
    sincos(-alpha + HALFPI, sa, ca);
    c2 = c + Point2(r * sa, r * ca);
    StdGuiRender::render_rect(c2.x - 10, c2.y - 10, c2.x + 10, c2.y + 10, Point2(0.25, 0), Point2(0.25, 0), Point2(0, 1)); // East
    sincos(-alpha, sa, ca);
    c2 = c + Point2(r * sa, r * ca);
    StdGuiRender::render_rect(c2.x - 10, c2.y - 10, c2.x + 10, c2.y + 10, Point2(0.50, 0), Point2(0.25, 0), Point2(0, 1)); // South
    sincos(-alpha - HALFPI, sa, ca);
    c2 = c + Point2(r * sa, r * ca);
    StdGuiRender::render_rect(c2.x - 10, c2.y - 10, c2.x + 10, c2.y + 10, Point2(0.75, 0), Point2(0.25, 0), Point2(0, 1)); // West

    StdGuiRender::set_texture(BAD_TEXTUREID);

    // render course and pitch
    StdGuiRender::set_font(0);
    String strPsi(0, "%.1f°", fmodf(RadToDeg(ang.x + 5 * HALFPI), 360));
    String strTeta(0, "%.1f°", RadToDeg(ang.y));

    //  course with moving scale
    StdGuiRender::set_color(COLOR_YELLOW);
    StdGuiRender::render_box(c.x - 35, c.y - 115, c.x + 35, c.y - 92);
    StdGuiRender::set_color(COLOR_BLACK);
    for (float a = -90, n = fmodf(RadToDeg(ang.x + 5 * HALFPI), 360); a <= 360 + 90; a += 10)
    {
      float x = (a - n) * 1.0f;
      bool bold = fmodf(a, 90) == 0;
      if (fabsf(x) <= 35)
        StdGuiRender::draw_line(c.x + x, c.y - 115, c.x + x, c.y - (bold ? 107 : 111), bold ? 2 : 1);
    }

    //  pitch with moving scale
    StdGuiRender::set_color(COLOR_GREEN);
    StdGuiRender::render_box(c.x - 35, c.y - 90, c.x + 35, c.y - 67);
    StdGuiRender::set_color(COLOR_WHITE);
    for (float a = -90, n = RadToDeg(ang.y); a <= 90; a += 10)
    {
      float y = (a - n) * 0.5f;
      bool bold = a == 0;
      if (fabsf(y) <= 11)
        StdGuiRender::draw_line(c.x - 35, c.y - 78 + y, c.x - (bold ? 20 : 28), c.y - 78 + y, bold ? 2 : 1);
    }

    //  draw numerical course
    StdGuiRender::set_color(COLOR_BLACK);
    ts = StdGuiRender::get_str_bbox(strPsi).size();
    StdGuiRender::draw_strf_to(c.x + 30 - ts.x, c.y - 102 + ts.y / 2, strPsi);

    //  draw numerical pitch
    StdGuiRender::set_color(COLOR_WHITE);
    ts = StdGuiRender::get_str_bbox(strTeta).size();
    StdGuiRender::draw_strf_to(c.x + 30 - ts.x, c.y - 81 + ts.y / 2, strTeta);
  }
}

bool DagorEdAppEventHandler::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (showCompass)
  {
    int vpW, vpH;
    wnd->getViewportSize(vpW, vpH);

    // handle click to world directions on compass to set proper camera
    Point2 ang = getCameraAngles();
    Point2 c(vpW - 132 + 64 - x, vpH - 132 + 64 - y), c2;
    for (int i = 0; i < 4; i++)
    {
      float r = 54, sa, ca;
      sincos(ang.x - HALFPI * (i + 1), sa, ca);
      if (lengthSq(c + Point2(r * sa, r * ca)) < 100)
      {
        ang.x = (i - 1) * HALFPI;
        wnd->setCameraDirection(angles_to_dir(ang), angles_to_dir(ang + Point2(0, HALFPI)));
        wnd->setCameraPos(::grs_cur_view.itm.getcol(3));
        return true;
      }
    }
  }

  return __super::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
}

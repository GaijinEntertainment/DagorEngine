// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_baseCursor.h>
#include <gui/dag_stdGuiRender.h>
#include <drv/3d/dag_sampler.h>

class GuiCursorLite : public IGenGuiCursor
{
  struct Mode
  {
    GuiNameId name;
    TEXTUREID texId;
    d3d::SamplerHandle sampler;
    float w, h;
    Point2 hotspot;

    Mode() : texId(BAD_TEXTUREID), sampler(d3d::INVALID_SAMPLER_HANDLE) {}

    ~Mode()
    {
      if (texId != BAD_TEXTUREID)
      {
        release_managed_tex(texId);
        texId = BAD_TEXTUREID;
      }
    }

    void set(float _w, float _h, float hotspotx, float hotspoty, const char *texfname)
    {
      w = _w;
      h = _h;
      hotspot = Point2(hotspotx, hotspoty);

      if (texId != BAD_TEXTUREID)
        release_managed_tex(texId);

      texId = add_managed_texture(texfname);
      if (texId != BAD_TEXTUREID)
      {
        acquire_managed_tex(texId);
        sampler = get_texture_separate_sampler(texId);
        if (sampler == d3d::INVALID_SAMPLER_HANDLE)
        {
          logerr("Failed to get sampler for texture %s", texfname);
          sampler = d3d::request_sampler({});
        }
      }
    }
  };

  Tab<Mode> modes;
  int curModeId;
  bool visible;
  GuiNameId curMode;

public:
  GuiCursorLite() : modes(uimem)
  {
    curModeId = -1;
    visible = true;
  }
  virtual void render(const Point2 &_pt)
  {
    if (curModeId == -1 || modes[curModeId].texId == BAD_TEXTUREID)
      return;

    StdGuiRender::set_ablend(true);
    StdGuiRender::set_color(255, 255, 255, 255);
    StdGuiRender::set_texture(modes[curModeId].texId, modes[curModeId].sampler);

    Point2 pt = _pt - modes[curModeId].hotspot;
    StdGuiRender::render_rect_t(pt.x, pt.y, pt.x + modes[curModeId].w, pt.y + modes[curModeId].h);
  }
  virtual void timeStep(real /*dt*/) {}

  virtual void setMode(const GuiNameId &mode_name)
  {
    if (curMode == mode_name)
      return;

    curMode = mode_name;
    for (int i = 0; i < modes.size(); ++i)
      if (modes[i].name == mode_name)
      {
        curModeId = i;
        return;
      }
    curModeId = -1;
  }

  virtual void setVisible(bool vis) { visible = vis; }
  virtual bool getVisible() const { return visible && (curModeId != -1); }

  virtual bool setModeCursor(const GuiNameId &mode_name, float w, float h, float hotspotx, float hotspoty, const char *texfname)
  {
    int i;
    for (i = 0; i < modes.size(); ++i)
      if (modes[i].name == mode_name)
        break;

    if (i >= modes.size())
    {
      i = append_items(modes, 1);
      modes[i].name = mode_name;
    }

    modes[i].set(w, h, hotspotx, hotspoty, texfname);
    return true;
  }
};
DAG_DECLARE_RELOCATABLE(GuiCursorLite::Mode);

IGenGuiCursor *create_gui_cursor_lite() { return new (uimem) GuiCursorLite; }

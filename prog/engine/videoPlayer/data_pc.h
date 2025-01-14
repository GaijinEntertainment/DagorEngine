// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ringbuf.h"
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaderVar.h>

struct VideoPlaybackData
{
  static constexpr int VIDEO_BUFFER_SIZE = 2;

  struct OneFrame
  {
    int time;
    TEXTUREID texIdY, texIdU, texIdV;
    Texture *texY, *texU, *texV; // system memory textures
    d3d::EventQuery *ev;
  };
  GenRingBuffer<OneFrame, 32> vBuf;

  struct OneVideoFrame
  {
    Texture *texY, *texU, *texV;
    TEXTUREID texIdY, texIdU, texIdV;
  };
  OneVideoFrame useVideo[VIDEO_BUFFER_SIZE];
  d3d::SamplerHandle sampler;
  int useVideoFramePos[VIDEO_BUFFER_SIZE];
  int currentVideoFrame;

public:
  void initBuffers(int q_depth)
  {
    const bool avoid_tex_update = d3d::get_driver_code().is(d3d::metal);
    if (avoid_tex_update)
      q_depth += VIDEO_BUFFER_SIZE; //== the same memory consumption as with DX, but use longer queue to avoid overrun
    vBuf.clear(q_depth);
    currentVideoFrame = 0;
    memset(useVideo, 0, sizeof(useVideo));
    memset(useVideoFramePos, 0xff, sizeof(useVideoFramePos));
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    sampler = d3d::request_sampler(smpInfo);
  }
  void termBuffers() { vBuf.clear(0); }
  static inline bool memsetTexture(Texture *tex)
  {
    if (!tex)
      return false;
    TextureInfo tinfo;
    int stride;
    uint8_t *p;
    if (tex->lockimg((void **)&p, stride, 0, TEXLOCK_WRITE))
    {
      tex->getinfo(tinfo);
      memset(p, 0xFF, stride * tinfo.h); // fixme: not correct for DXT
      return tex->unlockimg();
    }
    tex->unlockimg();
    return false;
  }
  bool initVideoBuffers(int wd, int ht)
  {
    TextureInfo ti;
    int w = wd, h = ht;
    int tex_flags;
    static int counter = 0;
    static char texName[20];
    const bool avoid_tex_update = d3d::get_driver_code().is(d3d::metal);

    tex_flags = TEXCF_DYNAMIC | TEXFMT_L8 | TEXCF_SYSMEM;
    if (avoid_tex_update)
    {
      tex_flags &= ~TEXCF_SYSMEM;
      DEBUG_CTX("avoid using tex->update() - use direct buffering");
    }

    for (int i = 0; i < vBuf.getDepth(); i++)
    {
      OneFrame &b = vBuf.buf[i];
      b.texY = d3d::create_tex(NULL, w, h, tex_flags, 1, "videoSysTexY");
      if (!b.texY)
      {
        DEBUG_CTX("can't create Y tex %d: w=%d, h=%d, tex_flags=0x%p", i, w, h, tex_flags);
        return false;
      }
      d3d_err(b.texY->getinfo(ti));
      if (ti.w < wd || ti.h < ht)
      {
        DEBUG_CTX("bad tex sz %dx%d < %dx%d", ti.w, ti.h, wd, ht);
        return false;
      }

      b.texU = d3d::create_tex(NULL, w / 2, h / 2, tex_flags, 1, "videoSysTexU");
      b.texV = d3d::create_tex(NULL, w / 2, h / 2, tex_flags, 1, "videoSysTexV");
      if (!b.texU || !b.texV)
      {
        DEBUG_CTX("can't create UV tex %d: w=%d, h=%d, tex_flags=0x%p", i, w / 2, h / 2, tex_flags);
        return false;
      }
      if (avoid_tex_update)
      {
        b.texY->disableSampler();
        b.texU->disableSampler();
        b.texV->disableSampler();

        snprintf(texName, sizeof(texName), "y%04d_ogv", ++counter);
        b.texIdY = register_managed_tex(texName, b.texY);

        texName[0] = 'u';
        b.texIdU = register_managed_tex(texName, b.texU);

        texName[0] = 'v';
        b.texIdV = register_managed_tex(texName, b.texV);
      }
      else
        b.texIdY = b.texIdU = b.texIdV = BAD_TEXTUREID;
    }
    if (avoid_tex_update)
      return true;

    tex_flags &= ~TEXCF_SYSMEM;
    tex_flags |= TEXCF_UPDATE_DESTINATION;

    for (int i = 0; i < VIDEO_BUFFER_SIZE; i++)
    {
      OneVideoFrame &b = useVideo[i];
      b.texY = d3d::create_tex(NULL, w, h, tex_flags, 1, "videoBufTexY");
      if (!b.texY)
      {
        DEBUG_CTX("can't create tex: w=%d, h=%d, tex_flags=0x%p", w, h, tex_flags);
        return false;
      }

      d3d_err(b.texY->getinfo(ti));
      if (ti.w < wd || ti.h < ht)
      {
        DEBUG_CTX("bad tex sz %dx%d < %dx%d", ti.w, ti.h, wd, ht);
        return false;
      }

      snprintf(texName, sizeof(texName), "y%04d_ogv", ++counter);
      b.texIdY = register_managed_tex(texName, b.texY);

      b.texU = d3d::create_tex(NULL, w / 2, h / 2, tex_flags, 1, "videoBufTexU");
      b.texV = d3d::create_tex(NULL, w / 2, h / 2, tex_flags, 1, "videoBufTexV");
      if (!b.texU || !b.texV)
      {
        DEBUG_CTX("can't create uv tex: w=%d, h=%d, tex_flags=0x%p", w / 2, h / 2, tex_flags);
        return false;
      }

      texName[0] = 'u';
      b.texIdU = register_managed_tex(texName, b.texU);

      texName[0] = 'v';
      b.texIdV = register_managed_tex(texName, b.texV);

      b.texY->disableSampler();
      b.texU->disableSampler();
      b.texV->disableSampler();
      memsetTexture(b.texY);
      memsetTexture(b.texU);
      memsetTexture(b.texV);
    }

    return true;
  }

  void termVideoBuffers()
  {
    for (int i = 0; i < VIDEO_BUFFER_SIZE; i++)
    {
      OneVideoFrame &b = useVideo[i];
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.texIdY, b.texY);
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.texIdU, b.texU);
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.texIdV, b.texV);
      del_d3dres(b.texY);
      del_d3dres(b.texU);
      del_d3dres(b.texV);
    }

    for (int i = 0; i < vBuf.getDepth(); i++)
    {
      OneFrame &b = vBuf.buf[i];
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.texIdY, b.texY);
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.texIdU, b.texU);
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(b.texIdV, b.texV);
      del_d3dres(b.texY);
      del_d3dres(b.texU);
      del_d3dres(b.texV);
      if (b.ev)
        d3d::release_event_query(b.ev);
    }

    currentVideoFrame = 0;
    memset(useVideoFramePos, 0xff, sizeof(useVideoFramePos));
    memset(vBuf.buf, 0, sizeof(vBuf.buf));
  }

  void getCurrentTex(TEXTUREID &idY, TEXTUREID &idU, TEXTUREID &idV)
  {
    if (vBuf.buf[0].texIdY != BAD_TEXTUREID)
    {
      idY = vBuf.getRd().texIdY;
      idU = vBuf.getRd().texIdU;
      idV = vBuf.getRd().texIdV;
      return;
    }

    if (useVideoFramePos[currentVideoFrame] != vBuf.getRd().time)
    {
      currentVideoFrame++;
      currentVideoFrame %= VIDEO_BUFFER_SIZE;
      useVideo[currentVideoFrame].texY->update(vBuf.getRd().texY);
      useVideo[currentVideoFrame].texU->update(vBuf.getRd().texU);
      useVideo[currentVideoFrame].texV->update(vBuf.getRd().texV);
      useVideoFramePos[currentVideoFrame] = vBuf.getRd().time;
    }
    idY = useVideo[currentVideoFrame].texIdY;
    idU = useVideo[currentVideoFrame].texIdU;
    idV = useVideo[currentVideoFrame].texIdV;
  }
};

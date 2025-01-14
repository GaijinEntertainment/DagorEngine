// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ringbuf.h"
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_query.h>
#include <util/dag_delayedAction.h>

struct VideoPlaybackData
{
  struct OneFrame
  {
    int time;
    Texture *texY, *texU, *texV;
    TEXTUREID texIdY, texIdU, texIdV;
    d3d::EventQuery *ev;
  };
  GenRingBuffer<OneFrame, 32> vBuf;
  d3d::SamplerHandle sampler;

public:
  void initBuffers(int q_depth) { vBuf.clear(q_depth); }
  void termBuffers() { vBuf.clear(0); }

  bool initVideoBuffers(int wd, int ht)
  {
    int w = wd, h = ht;
    int tex_flags;

    tex_flags = TEXFMT_L8;
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC
    tex_flags |= TEXCF_DYNAMIC;
#endif

    for (int i = 0; i < vBuf.getDepth(); i++)
    {
      OneFrame &b = vBuf.buf[i];
      b.texIdY = b.texIdU = b.texIdV = BAD_TEXTUREID;

      b.texY = d3d::create_tex(NULL, w, h, tex_flags, 1);
      if (!b.texY)
      {
        DEBUG_CTX("can't create tex %d: w=%d, h=%d, tex_flags=0x%08X", i, w, h, tex_flags);
        return false;
      }

      TextureInfo ti;
      d3d_err(b.texY->getinfo(ti));
      if (ti.w < wd || ti.h < ht)
        return false;

      b.texU = d3d::create_tex(NULL, w / 2, h / 2, tex_flags, 1);
      b.texV = d3d::create_tex(NULL, w / 2, h / 2, tex_flags, 1);
      if (!b.texU || !b.texV)
      {
        DEBUG_CTX("can't create uv tex %d: w=%d, h=%d, tex_flags=0x%08X", i, w / 2, h / 2, tex_flags);
        return false;
      }

      static int counter = 0;
      static char texName[20];

      snprintf(texName, sizeof(texName), "y%04d_ogv", ++counter);
      b.texIdY = register_managed_tex(texName, b.texY);

      texName[0] = 'u';
      b.texIdU = register_managed_tex(texName, b.texU);

      texName[0] = 'v';
      b.texIdV = register_managed_tex(texName, b.texV);

      b.texY->disableSampler();
      b.texU->disableSampler();
      b.texV->disableSampler();
    }
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    sampler = d3d::request_sampler(smpInfo);
    return true;
  }

  void termVideoBuffers()
  {
    for (int i = 0; i < vBuf.getDepth(); i++)
    {
      OneFrame &b = vBuf.buf[i];
      release_managed_tex_verified(b.texIdY, b.texY);
      release_managed_tex_verified(b.texIdU, b.texU);
      release_managed_tex_verified(b.texIdV, b.texV);
      if (auto ev = b.ev) // Note: ATTOW not all drivers can safely destroy event query on thread
        run_action_on_main_thread([=]() { d3d::release_event_query(ev); });
    }
    memset(vBuf.buf, 0, sizeof(vBuf.buf));
  }

  void getCurrentTex(TEXTUREID &idY, TEXTUREID &idU, TEXTUREID &idV)
  {
    idY = vBuf.getRd().texIdY;
    idU = vBuf.getRd().texIdU;
    idV = vBuf.getRd().texIdV;
  }
};

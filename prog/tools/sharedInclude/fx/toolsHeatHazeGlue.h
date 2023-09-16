#pragma once

#include <3d/dag_tex3d.h>
#include <perfMon/dag_cpuFreq.h>
#include <render/heatHazeRenderer.h>
#include <shaders/dag_overrideStates.h>
#include <de3_dynRenderService.h>

struct ToolsHeatHazeRendererGlue
{
  void setFunctions(eastl::function<void()> render_particles, eastl::function<void()> render_ri)
  {
    renderParticles = render_particles;
    renderRI = render_ri;
  }

  void render()
  {
    if (IDynRenderService *renderService = EDITORCORE->queryEditorInterface<IDynRenderService>())
    {
      Texture *renderBuffer = renderService->getRenderBuffer();
      TEXTUREID renderBufferId = renderService->getRenderBufferId();
      TEXTUREID depthBufferId = renderService->getDepthBufferId();

      if (renderBuffer && renderBufferId != BAD_TEXTUREID)
      {
        TextureInfo info;
        renderBuffer->getinfo(info);

        int hazeDivisor = max(::dgs_get_game_params()->getInt("hazeDivisor", 4), 1);
        int texWidth = max(info.w / hazeDivisor, 1);
        int texHeight = max(info.h / hazeDivisor, 1);

        if (!heatHazeRenderer || offsetTextureWidth != texWidth || offsetTextureHeight != texHeight)
        {
          offsetTextureWidth = texWidth;
          offsetTextureHeight = texHeight;

          targetHazeOffset.close();
          targetHazeDepth.close();
          targetHazeColor.close();
          targetHazeTemp.close();

          heatHazeRenderer.reset();
          heatHazeRenderer = eastl::make_unique<HeatHazeRenderer>(hazeDivisor);

          targetHazeOffset = dag::create_tex(nullptr, offsetTextureWidth, offsetTextureHeight, TEXFMT_A16B16G16R16F | TEXCF_RTARGET, 1,
            "haze_offset_tex");

          uint32_t flags = 0;
          for (auto format : {TEXFMT_DEPTH24, TEXFMT_DEPTH16})
            if (d3d::check_texformat(format | TEXCF_RTARGET))
            {
              flags = format | TEXCF_RTARGET;
              break;
            }
          G_ASSERT(flags);

          targetHazeDepth = dag::create_tex(nullptr, offsetTextureWidth, offsetTextureHeight, flags, 1, "haze_depth_tex");
          targetHazeColor =
            dag::create_tex(nullptr, offsetTextureWidth, offsetTextureHeight, TEXFMT_DEFAULT | TEXCF_RTARGET, 1, "haze_color_tex");

          if (heatHazeRenderer->isHazeAppliedManual())
          {
            int backBufferWidth, backBufferHeight;
            uint32_t backBufferFormat;
            TextureInfo backBufferInfo;
            renderBuffer->getinfo(backBufferInfo);
            backBufferWidth = backBufferInfo.w;
            backBufferHeight = backBufferInfo.h;
            backBufferFormat = backBufferInfo.cflg & TEXFMT_MASK;

            targetHazeTemp = dag::create_tex(nullptr, backBufferWidth / 2, backBufferHeight / 2, backBufferFormat | TEXCF_RTARGET, 1,
              "rTargetHazeTemp");
          }

          shaders::OverrideState state;
          state.set(shaders::OverrideState::Z_FUNC);
          state.zFunc = CMPF_ALWAYS;
          zFuncAlwaysStateId = shaders::overrides::create(state);
        }

        HeatHazeRenderer::RenderTargets targets;
        targets.backBufferId = renderBufferId;
        targets.backBuffer = renderBuffer;
        targets.depthId = depthBufferId;
        targets.resolvedDepthId = depthBufferId;
        targets.hazeOffset = targetHazeOffset.getTex2D();
        targets.hazeDepth = targetHazeDepth.getTex2D();
        targets.hazeColor = targetHazeColor.getTex2D();
        targets.hazeTemp = targetHazeTemp.getTex2D();
        targets.hazeTempId = targetHazeTemp.getTexId();

        heatHazeRenderer->render(float(get_time_msec()) / 1000, targets, {info.w, info.h}, 0, renderParticles, nullptr, renderRI);
      }
    }
  }

  eastl::unique_ptr<HeatHazeRenderer> heatHazeRenderer;

  UniqueTexHolder targetHazeOffset;
  UniqueTexHolder targetHazeDepth;
  UniqueTexHolder targetHazeColor;
  UniqueTex targetHazeTemp;

  shaders::UniqueOverrideStateId zFuncAlwaysStateId;

  int offsetTextureWidth = 0;
  int offsetTextureHeight = 0;

  eastl::function<void()> renderParticles;
  eastl::function<void()> renderRI;
};
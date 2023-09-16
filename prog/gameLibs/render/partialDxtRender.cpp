#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3dCmd.h>
#include <render/dxtcompress.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_mathUtils.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_watchdog.h>
#include <math/dag_imageFunctions.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_span.h>
#include <startup/dag_globalSettings.h>
#include <image/dag_dxtCompress.h>

#define DEBUG_SAVE_RT     0
#define DEBUG_SAVE_RT_TGA 0

#if DEBUG_SAVE_RT
#include <debug/dag_debug3d.h>
#include <image/dag_jpeg.h>
#include <image/dag_tga.h>
#include <stdio.h>
#endif

const int MAX_LOD = 12;
const int DXT_GRANULARITY = 4;
const int BPP = 4;

#define SKIP_COLOR 0

// todo gamma mip
void PartialDxtRender(Texture *rt, Texture *rtn, int linesPerPart, int picWidth, int picHeight, int numMips, bool dxt5, bool dxt5n,
  void (*renderFunc)(int lineNo, int linesCount, int totalLines, void *user_data), void *user_data, bool gamma_mips,
  bool update_game_screen)
{
  G_ASSERT(picHeight % linesPerPart == 0);

  unsigned int linearSliceSize = BPP * picWidth * linesPerPart;


  uint8_t *linearSliceData = (uint8_t *)memalloc(linearSliceSize);
  uint8_t *linearSliceNData = NULL;
  if (rtn)
    linearSliceNData = (uint8_t *)memalloc(linearSliceSize);

  unsigned flags =
#if _TARGET_C1 | _TARGET_C2

#endif
    TEXFMT_A8R8G8B8 | TEXCF_RTARGET; // | TEXCF_SRGBREAD

  //  unsigned flags = TEXFMT_A8R8G8B8 | TEXCF_RTARGET; //???

  Texture *localRt1 = d3d::create_tex(NULL, picWidth, linesPerPart, flags | TEXCF_SRGBWRITE, 1, "temp_part_dxt_tex1");
  Texture *localRt1n = d3d::create_tex(NULL, picWidth, linesPerPart, flags, 1, "temp_part_dxt_tex1n");
  Texture *localRt1ao = d3d::create_tex(NULL, picWidth, linesPerPart, flags, 1, "temp_part_dxt_tex1ao");

#if !_TARGET_PC
  Texture *localRt2 = d3d::create_tex(NULL, picWidth, linesPerPart, flags | TEXCF_SRGBWRITE, 1, "temp_part_dxt_tex2");
  Texture *localRt2n = NULL;
  if (rtn)
    localRt2n = d3d::create_tex(NULL, picWidth, linesPerPart, flags, 1, "temp_part_dxt_tex2n");
#endif

    // For PS3 we don't want to allocate 16 Mb of memory for whole image.
    // So we render image and pack it to DXT by slices.

#if DEBUG_SAVE_RT
  uint8_t *copyData = (uint8_t *)memalloc(picWidth * picHeight * BPP, tmpmem);
#endif


  uint8_t *downsampledSlices[MAX_LOD];
  uint8_t *downsampledSlicesN[MAX_LOD];
  uint32_t downsampledSlicesSize[MAX_LOD];

  uint32_t mipLevelsCnt = 0;
  uint16_t mipCompletenessMask = 0;

  const uint32_t widthLimit = DXT_GRANULARITY;

  // count mipmaps and allocate memory for one slice of every mipmap
  // slice is picWidth x linesPerPart rectangle.
  for (uint32_t w = picWidth, h = picHeight; w >= widthLimit && h >= DXT_GRANULARITY && mipLevelsCnt < numMips;
       mipLevelsCnt++, w /= 2, h /= 2)
  {
    G_ASSERT(mipLevelsCnt < MAX_LOD);

    if (mipLevelsCnt > 0)
    {
      uint32_t sz = w * linesPerPart * BPP;
      downsampledSlices[mipLevelsCnt] = (uint8_t *)memalloc(sz);
      if (rtn)
        downsampledSlicesN[mipLevelsCnt] = (uint8_t *)memalloc(sz);
      downsampledSlicesSize[mipLevelsCnt] = sz;
    }
  }
  downsampledSlices[0] = linearSliceData;
  if (rtn)
    downsampledSlicesN[0] = linearSliceNData;
  SmallTab<float, TmpmemAlloc> floatTemp;
  if (gamma_mips)
    clear_and_resize(floatTemp, picWidth * linesPerPart * 4);

#define EXTERNAL_START_RENDER ((void)0)
#define EXTERNAL_END_RENDER   ((void)0)

#define INTERNAL_START_RENDER                                                                                         \
  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);                                             \
  if (update_game_screen && is_main_thread() && get_time_usec_qpc(last_draw_time) > 30000 && !d3d::device_lost(NULL)) \
  {                                                                                                                   \
    d3d::setwire(0);                                                                                                  \
    d3d::set_render_target();                                                                                         \
    dagor_work_cycle_flush_pending_frame();                                                                           \
    dagor_draw_scene_and_gui(true, true);                                                                             \
    d3d::update_screen();                                                                                             \
    dagor_idle_cycle();                                                                                               \
    last_draw_time = ref_time_ticks_qpc();                                                                            \
    watchdog_kick();                                                                                                  \
  }                                                                                                                   \
  d3d::get_render_target(prevRt);                                                                                     \
  d3d::set_render_target()

#define INTERNAL_END_RENDER                                               \
  d3d::set_render_target(prevRt);                                         \
  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL); \
  if (is_main_thread())                                                   \
  watchdog_kick()

  Driver3dRenderTarget prevRt;
  int64_t last_draw_time = ref_time_ticks_qpc();

  // process image by slices
  EXTERNAL_START_RENDER;
  for (uint32_t y = 0; y < picHeight; y += linesPerPart)
  {
    d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, NULL, NULL, NULL);
    uint32_t slice = y / linesPerPart;
    bool is_pc = false;
#if _TARGET_PC
    is_pc = true;
#endif
    if (!y || is_pc)
    {
      INTERNAL_START_RENDER;

      d3d::set_render_target(localRt1, 0);
      d3d::set_render_target(1, localRt1n, 0);
      d3d::set_render_target(2, localRt1ao, 0);
      renderFunc(y, linesPerPart, picHeight, user_data);

      INTERNAL_END_RENDER;
    }

#if !_TARGET_PC
    if (y < picHeight - linesPerPart)
    {
      // render next slice picWidth x linesPerPart.
      INTERNAL_START_RENDER;

      d3d::set_render_target(localRt2, 0);
      if (rtn)
        d3d::set_render_target(1, localRt2n, 0);

      renderFunc(y + linesPerPart, linesPerPart, picHeight, user_data);

      INTERNAL_END_RENDER;
    }
#endif

#if SAVE_MIPS_RT && !_TARGET_PC
    char path[80];
    snprintf(path, sizeof(path), "slice%04d.jpg", y);
    save_rt_image_as_jpeg(localRt2, 0, path);
#endif

    void *renderedData = NULL;
    int renderedDataStride;

    void *renderedNData = NULL;
    int renderedNDataStride = 0;
    bool lock_fail_logged = false;
    while (!localRt1->lockimg(&renderedData, renderedDataStride, 0, TEXLOCK_READ) || !renderedData ||
           (rtn && (!localRt1n->lockimg(&renderedNData, renderedNDataStride, 0, TEXLOCK_READ) || !renderedNData)))
    {
      if (renderedData)
      {
        localRt1->unlockimg();
        renderedData = NULL;
      }

      if (renderedNData)
      {
        localRt1n->unlockimg();
        renderedNData = NULL;
      }

      if (!lock_fail_logged)
      {
        if (!d3d::is_in_device_reset_now())
          logerr("unable to lock image - device lost?");
        lock_fail_logged = true;
      }
      if (is_main_thread())
      {
        return; // Will be re-rendered in afterReset. Cannot reset here because current RT will be deleted during that.
      }
      else
      {
        // Give the main thread a chance to reset the device.

        EXTERNAL_END_RENDER;
        sleep_msec(1000);
        EXTERNAL_START_RENDER;
      }

      d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
      d3d::get_render_target(prevRt);

      d3d::set_render_target(localRt1, 0);
      d3d::set_render_target(1, localRt1n, 0);
      d3d::set_render_target(2, localRt1ao, 0);
      renderFunc(y, linesPerPart, picHeight, user_data);

      d3d::set_render_target(prevRt);
      d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
    }

    {
      int linearSlicePitch = BPP * picWidth;
      uint8_t *src = (uint8_t *)renderedData;
      uint8_t *dst = (uint8_t *)linearSliceData;
      for (int i = 0; i < linesPerPart; ++i, src += renderedDataStride, dst += linearSlicePitch)
        memcpy(dst, src, linearSlicePitch);
      if (rtn)
      {
        src = (uint8_t *)renderedNData;
        dst = (uint8_t *)linearSliceNData;
        for (int i = 0; i < linesPerPart; ++i, src += renderedNDataStride, dst += linearSlicePitch)
          memcpy(dst, src, linearSlicePitch);
      }
    }
    d3d_err(localRt1->unlockimg());
    if (rtn)
    {
      d3d_err(localRt1n->unlockimg());
    }


#if DEBUG_SAVE_RT
    memcpy(copyData + y * picWidth * BPP, linearSliceData, linearSliceSize);
#endif

    // upload ready mip-levels
    for (uint32_t mip = 0; mip < mipLevelsCnt; mip++)
    {
      uint32_t sliceWidth = picWidth >> mip;
      uint32_t sliceHeight = linesPerPart;
      uint32_t sliceSize = sliceWidth * sliceHeight * BPP;
      uint32_t mipSlice = slice >> mip;

      if (mip > 0)
      {
        // we store only upper half of the slice
        G_ASSERT(downsampledSlicesSize[mip] == sliceSize);

        if ((mipCompletenessMask & (1 << mip)) == 0)
        {
          // store first half of downsampled slice
          mipCompletenessMask |= (1 << mip);
          if (gamma_mips)
          {
            int numPixels = sliceWidth * 2 * sliceHeight;
            imagefunctions::exponentiate4_c(downsampledSlices[mip - 1], floatTemp.data(), numPixels, 2.2f);
            imagefunctions::downsample4x_simda(floatTemp.data(), floatTemp.data(), sliceWidth, sliceHeight / 2);
            imagefunctions::convert_to_linear_simda(floatTemp.data(), downsampledSlices[mip], numPixels / 4, 1.f / 2.2f);
          }
          else
            software_downsample_2x(downsampledSlices[mip], downsampledSlices[mip - 1], sliceWidth, sliceHeight / 2);
          if (rtn)
            software_downsample_2x(downsampledSlicesN[mip], downsampledSlicesN[mip - 1], sliceWidth, sliceHeight / 2);
          break;
        }
        else
        {
          // concate with first half, downsample and compress the slice
          mipCompletenessMask &= ~(1 << mip);
          if (gamma_mips)
          {
            int numPixels = sliceWidth * 2 * sliceHeight;
            imagefunctions::exponentiate4_c(downsampledSlices[mip - 1], floatTemp.data(), numPixels, 2.2f);
            imagefunctions::downsample4x_simda(floatTemp.data(), floatTemp.data(), sliceWidth, sliceHeight / 2);
            imagefunctions::convert_to_linear_simda(floatTemp.data(), downsampledSlices[mip] + sliceSize / 2, numPixels / 4,
              1.f / 2.2f);
          }
          else
            software_downsample_2x(downsampledSlices[mip] + sliceSize / 2, downsampledSlices[mip - 1], sliceWidth, sliceHeight / 2);
          if (rtn)
            software_downsample_2x(downsampledSlicesN[mip] + sliceSize / 2, downsampledSlicesN[mip - 1], sliceWidth, sliceHeight / 2);
        }
      }

      // compress mip slice sliceWidth x linesPerPart to DXT
      void *rtData = NULL;
      int rtStride;
      unsigned lockFlags = TEXLOCK_WRITE | TEXLOCK_DONOTUPDATEON9EXBYDEFAULT;
      if (!rt->lockimg(&rtData, rtStride, mip, lockFlags))
      {
        debug("%s lockimg failed with '%s' (%d)", __FUNCTION__, d3d::get_last_error(), 1);
        continue; // not sure what to do from here
      }
      G_ASSERT(rtData);
      uint32_t sliceSizeDXT = rtStride * linesPerPart / DXT_GRANULARITY;
      char *outData = (char *)rtData + sliceSizeDXT * mipSlice;
      ManualDXT(dxt5 ? MODE_DXT5 : MODE_DXT1, (TexPixel32 *)downsampledSlices[mip], sliceWidth, linesPerPart, rtStride, outData,
        picWidth > 4096 ? DXT_ALGORITHM_PRODUCTION : DXT_ALGORITHM_EXCELLENT);
      rt->unlockimg();
      if (rtn)
      {
        rtData = NULL;
        if (!rtn->lockimg(&rtData, rtStride, mip, lockFlags))
        {
          debug("%s lockimg failed with '%s' (%d)", __FUNCTION__, d3d::get_last_error(), 2);
          continue; // not sure what to do from here
        }
        G_ASSERT(rtData);
        uint32_t sliceSizeDXTN = rtStride * linesPerPart / DXT_GRANULARITY;
        outData = (char *)rtData + sliceSizeDXTN * mipSlice;
        ManualDXT(dxt5n ? MODE_DXT5 : MODE_DXT1, (TexPixel32 *)downsampledSlicesN[mip], sliceWidth, linesPerPart, rtStride, outData,
          picWidth > 4096 ? DXT_ALGORITHM_PRODUCTION : DXT_ALGORITHM_EXCELLENT);
        rtn->unlockimg();
      }
    }
#if !_TARGET_PC
    eastl::swap(localRt1, localRt2);
#endif
  }
  int stride;
  rt->lockimg(NULL, stride, 0, TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_DELSYSMEMCOPY);
  rt->unlockimg();
  if (rtn)
  {
    rtn->lockimg(NULL, stride, 0, TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_DELSYSMEMCOPY);
    rtn->unlockimg();
  }
  EXTERNAL_END_RENDER;

#undef EXTERNAL_START_RENDER
#undef EXTERNAL_END_RENDER
#undef INTERNAL_START_RENDER
#undef INTERNAL_END_RENDER

#if DEBUG_SAVE_RT
  static int number = 1;
  char buf[256];
#if DEBUG_SAVE_RT_TGA
  snprintf(buf, sizeof(buf), "debug_clip%d.tga", number++);
  save_tga32(buf, (TexPixel32 *)copyData, picWidth, picHeight, picWidth * 4);
#else
  snprintf(buf, sizeof(buf), "debug_clip%d.jpg", number++);
  save_jpeg32((TexPixel32 *)copyData, picWidth, picHeight, picWidth * 4, 50, buf);
#endif
  // save_rt_image_as_jpeg(copyTex, 0, "rt.jpg");
  memfree(copyData, tmpmem);
#endif

  //  G_ASSERT(mipCompletenessMask == 0);

  // free memory of slices (downsampledSlices[0] isn't allocated)
  for (uint32_t mip = mipLevelsCnt - 1; mip > 0; mip--)
    memfree_anywhere(downsampledSlices[mip]);

  if (rtn)
    for (uint32_t mip = mipLevelsCnt - 1; mip > 0; mip--)
      memfree_anywhere(downsampledSlicesN[mip]);

  memfree_anywhere(linearSliceData);
  if (rtn)
    memfree_anywhere(linearSliceNData);
#if !_TARGET_PC
  del_d3dres(localRt2);
  del_d3dres(localRt2n);
#endif
  del_d3dres(localRt1);
  del_d3dres(localRt1n);
  del_d3dres(localRt1ao);
}

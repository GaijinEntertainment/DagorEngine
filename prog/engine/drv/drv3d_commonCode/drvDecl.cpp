// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_carray.h>
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>
#include <osApiWrappers/dag_localConv.h>


// From https://github.com/WebKit/webkit/blob/main/Source/ThirdParty/ANGLE/src/libANGLE/renderer/driver_utils.h#L18 and
// https://gamedev.stackexchange.com/a/31626
static const eastl::pair<uint32_t, GpuVendor> vendor_id_table[] = //
  {
    {0x10DE, GpuVendor::NVIDIA},

    {0x1002, GpuVendor::AMD},
    {0x1022, GpuVendor::AMD},

    {0x8086, GpuVendor::INTEL},
    {0x8087, GpuVendor::INTEL},
    {0x163C, GpuVendor::INTEL},

    {0x5143, GpuVendor::QUALCOMM},
    {0x4D4F4351, GpuVendor::QUALCOMM},

    {0x1010, GpuVendor::IMGTEC},

    {0x13B5, GpuVendor::ARM},

    {0x05AC, GpuVendor::APPLE},

    {0x10005, GpuVendor::MESA},

    {0x0003, GpuVendor::SHIM_DRIVER},

    {0x144D, GpuVendor::SAMSUNG},

    {0x19E5, GpuVendor::HUAWEI},

    {0x0000, GpuVendor::UNKNOWN},
};

static const carray<const char *, GPU_VENDOR_COUNT> vendor_names = {
  "Unknown",     // GpuVendor::UNKNOWN
  "Mesa",        // GpuVendor::MESA
  "ImgTec",      // GpuVendor::IMGTEC
  "AMD",         // GpuVendor::AMD / GpuVendor::ATI
  "NVIDIA",      // GpuVendor::NVIDIA
  "Intel",       // GpuVendor::INTEL
  "Apple",       // GpuVendor::APPLE
  "Shim driver", // GpuVendor::SHIM_DRIVER
  "ARM",         // GpuVendor::ARM
  "Qualcomm",    // GpuVendor::QUALCOMM
  "Samsung",     // GpuVendor::SAMSUNG
  "Huawei",      // GpuVendor::HUAWEI
};

void d3d_get_render_target(Driver3dRenderTarget &rt) { d3d::get_render_target(rt); }
void d3d_set_render_target(Driver3dRenderTarget &rt) { d3d::set_render_target(rt); }

void d3d_get_view_proj(ViewProjMatrixContainer &vp)
{
  d3d::gettm(TM_VIEW, vp.savedView);
  vp.p_ok = d3d::getpersp(vp.p);
  // Get proj tm also even though persp is ok. Overally there is no good having
  // uninitialized proj whereas everyone can use it
  d3d::gettm(TM_PROJ, &vp.savedProj);
}

void d3d_set_view_proj(const ViewProjMatrixContainer &vp)
{
  d3d::settm(TM_VIEW, vp.savedView);
  if (!vp.p_ok)
    d3d::settm(TM_PROJ, &vp.savedProj);
  else
    d3d::setpersp(vp.p);
}

void d3d_get_view(int &viewX, int &viewY, int &viewW, int &viewH, float &viewN, float &viewF)
{
  d3d::getview(viewX, viewY, viewW, viewH, viewN, viewF);
}
void d3d_set_view(int viewX, int viewY, int viewW, int viewH, float viewN, float viewF)
{
  d3d::setview(viewX, viewY, viewW, viewH, viewN, viewF);
  d3d::setscissor(viewX, viewY, viewW, viewH);
}

const char *d3d_get_vendor_name(GpuVendor vendor)
{
  int vendorIdx = eastl::to_underlying(vendor);
  G_ASSERT(vendorIdx >= 0 && vendorIdx < vendor_names.size());
  return vendor_names[vendorIdx];
}

GpuVendor d3d_get_vendor(uint32_t vendor_id, const char *description)
{
  GpuVendor vendor = GpuVendor::UNKNOWN;
  auto ref =
    eastl::find_if(eastl::begin(vendor_id_table), eastl::end(vendor_id_table), [=](auto &ent) { return ent.first == vendor_id; });
  if (ref != eastl::end(vendor_id_table))
    vendor = ref->second;
  else if (description)
  {
    if (strstr(description, "ATI") || strstr(description, "AMD"))
      vendor = GpuVendor::AMD;
    else
    {
      if (char *lower = str_dup(description, tmpmem))
      {
        lower = dd_strlwr(lower);
        if (strstr(lower, "radeon"))
          vendor = GpuVendor::AMD;
        else if (strstr(lower, "geforce") || strstr(lower, "nvidia"))
          vendor = GpuVendor::NVIDIA;
        else if (strstr(lower, "intel") || strstr(lower, "rdpdd")) // Assume the worst for the Microsoft Mirroring Driver - take it as
                                                                   // it mirrors to Intel driver with broken voltex compression.
          vendor = GpuVendor::INTEL;
        memfree(lower, tmpmem);
      }
    }
  }
  return vendor;
}

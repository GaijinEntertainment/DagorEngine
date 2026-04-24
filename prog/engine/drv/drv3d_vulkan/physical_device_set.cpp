// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physical_device_set.h"

using namespace drv3d_vulkan;

namespace
{

struct DeviceInfo
{
  const char *name;
  GpuVendor vendor;
  double tflops;     // Peak FP32 single-precision (TFLOPS)
  const char *uarch; // GPU micro-architecture
};

static const DeviceInfo deviceInfos[] = {
  // ══════════════════════════════════════════════════════════════════════════
  //  NVIDIA
  // ══════════════════════════════════════════════════════════════════════════

  // ── Fermi (Vulkan 1.0) ───────────────────────────────────────────────────
  {"geforce gtx 480", GpuVendor::NVIDIA, 1.345, "fermi"},
  {"geforce gtx 470", GpuVendor::NVIDIA, 1.089, "fermi"},
  {"geforce gtx 460", GpuVendor::NVIDIA, 0.907, "fermi"},
  {"geforce gtx 580", GpuVendor::NVIDIA, 1.581, "fermi"},
  {"geforce gtx 570", GpuVendor::NVIDIA, 1.405, "fermi"},
  {"geforce gtx 560 ti", GpuVendor::NVIDIA, 1.263, "fermi"},
  // ── Kepler (Vulkan 1.2) ──────────────────────────────────────────────────
  {"geforce gtx 680", GpuVendor::NVIDIA, 3.090, "kepler"},
  {"geforce gtx 670", GpuVendor::NVIDIA, 2.460, "kepler"},
  {"geforce gtx 660 ti", GpuVendor::NVIDIA, 2.460, "kepler"},
  {"geforce gtx 660", GpuVendor::NVIDIA, 1.882, "kepler"},
  {"geforce gtx 650", GpuVendor::NVIDIA, 0.931, "kepler"},
  {"geforce gtx 780 ti", GpuVendor::NVIDIA, 5.046, "kepler"},
  {"geforce gtx 780", GpuVendor::NVIDIA, 3.977, "kepler"},
  {"geforce gtx 770", GpuVendor::NVIDIA, 3.213, "kepler"},
  {"geforce gtx 760", GpuVendor::NVIDIA, 2.257, "kepler"},
  {"geforce gt 730", GpuVendor::NVIDIA, 0.693, "kepler"},
  {"geforce gtx titan", GpuVendor::NVIDIA, 4.500, "kepler"},
  {"geforce gtx titan black", GpuVendor::NVIDIA, 5.121, "kepler"},
  {"geforce gtx titan z", GpuVendor::NVIDIA, 8.122, "kepler"},
  // ── Maxwell (Vulkan 1.3) ─────────────────────────────────────────────────
  {"geforce gtx 750 ti", GpuVendor::NVIDIA, 1.389, "maxwell"},
  {"geforce gtx 750", GpuVendor::NVIDIA, 1.044, "maxwell"},
  {"geforce gtx 980 ti", GpuVendor::NVIDIA, 5.632, "maxwell"},
  {"geforce gtx 980", GpuVendor::NVIDIA, 4.612, "maxwell"},
  {"geforce gtx 970", GpuVendor::NVIDIA, 3.920, "maxwell"},
  {"geforce gtx 960", GpuVendor::NVIDIA, 2.308, "maxwell"},
  {"geforce gtx 950", GpuVendor::NVIDIA, 1.793, "maxwell"},
  {"geforce gtx titan x", GpuVendor::NVIDIA, 6.144, "maxwell"},

  // ── Pascal (Vulkan 1.3) ──────────────────────────────────────────────────
  {"geforce gtx 1080 ti", GpuVendor::NVIDIA, 11.340, "pascal"},
  {"geforce gtx 1080", GpuVendor::NVIDIA, 8.873, "pascal"},
  {"geforce gtx 1070 ti", GpuVendor::NVIDIA, 8.186, "pascal"},
  {"geforce gtx 1070", GpuVendor::NVIDIA, 6.463, "pascal"},
  {"geforce gtx 1060 6gb", GpuVendor::NVIDIA, 4.375, "pascal"},
  {"geforce gtx 1060 3gb", GpuVendor::NVIDIA, 3.935, "pascal"},
  {"geforce gtx 1050 ti", GpuVendor::NVIDIA, 2.138, "pascal"},
  {"geforce gtx 1050", GpuVendor::NVIDIA, 1.862, "pascal"},
  {"geforce gt 1030", GpuVendor::NVIDIA, 0.907, "pascal"},
  {"nvidia titan x (pascal)", GpuVendor::NVIDIA, 10.974, "pascal"},
  {"nvidia titan xp", GpuVendor::NVIDIA, 12.150, "pascal"},
  {"tesla p100 pcie 16gb", GpuVendor::NVIDIA, 9.300, "pascal"},
  {"tesla p40", GpuVendor::NVIDIA, 11.758, "pascal"},
  {"quadro gp100", GpuVendor::NVIDIA, 10.300, "pascal"},
  {"quadro p6000", GpuVendor::NVIDIA, 12.629, "pascal"},
  {"quadro p5000", GpuVendor::NVIDIA, 8.873, "pascal"},
  // ── Volta (Vulkan 1.3) ───────────────────────────────────────────────────
  {"nvidia titan v", GpuVendor::NVIDIA, 13.800, "volta"},
  {"tesla v100 sxm2 32gb", GpuVendor::NVIDIA, 15.700, "volta"},
  {"tesla v100 pcie 32gb", GpuVendor::NVIDIA, 14.000, "volta"},
  {"quadro gv100", GpuVendor::NVIDIA, 16.664, "volta"},

  // ── Turing (Vulkan 1.3) ──────────────────────────────────────────────────
  {"geforce rtx 2080 ti", GpuVendor::NVIDIA, 13.448, "turing"},
  {"geforce rtx 2080 super", GpuVendor::NVIDIA, 11.151, "turing"},
  {"geforce rtx 2080", GpuVendor::NVIDIA, 10.069, "turing"},
  {"geforce rtx 2070 super", GpuVendor::NVIDIA, 9.062, "turing"},
  {"geforce rtx 2070", GpuVendor::NVIDIA, 7.465, "turing"},
  {"geforce rtx 2060 super", GpuVendor::NVIDIA, 7.181, "turing"},
  {"geforce rtx 2060", GpuVendor::NVIDIA, 6.451, "turing"},
  {"geforce gtx 1660 ti", GpuVendor::NVIDIA, 5.437, "turing"},
  {"geforce gtx 1660 super", GpuVendor::NVIDIA, 5.027, "turing"},
  {"geforce gtx 1660", GpuVendor::NVIDIA, 4.997, "turing"},
  {"geforce gtx 1650 super", GpuVendor::NVIDIA, 4.416, "turing"},
  {"geforce gtx 1650", GpuVendor::NVIDIA, 2.984, "turing"},
  {"nvidia titan rtx", GpuVendor::NVIDIA, 16.312, "turing"},
  {"quadro rtx 8000", GpuVendor::NVIDIA, 16.312, "turing"},
  {"quadro rtx 6000", GpuVendor::NVIDIA, 16.312, "turing"},
  {"quadro rtx 5000", GpuVendor::NVIDIA, 11.151, "turing"},
  {"quadro rtx 4000", GpuVendor::NVIDIA, 7.119, "turing"},
  {"tesla t4", GpuVendor::NVIDIA, 8.141, "turing"},

  // ── Ampere (Vulkan 1.3) ──────────────────────────────────────────────────
  {"geforce rtx 3090 ti", GpuVendor::NVIDIA, 40.000, "ampere"},
  {"geforce rtx 3090", GpuVendor::NVIDIA, 35.580, "ampere"},
  {"geforce rtx 3080 ti", GpuVendor::NVIDIA, 34.100, "ampere"},
  {"geforce rtx 3080 12gb", GpuVendor::NVIDIA, 30.600, "ampere"},
  {"geforce rtx 3080", GpuVendor::NVIDIA, 29.768, "ampere"},
  {"geforce rtx 3070 ti", GpuVendor::NVIDIA, 21.750, "ampere"},
  {"geforce rtx 3070", GpuVendor::NVIDIA, 20.314, "ampere"},
  {"geforce rtx 3060 ti", GpuVendor::NVIDIA, 16.200, "ampere"},
  {"geforce rtx 3060", GpuVendor::NVIDIA, 12.742, "ampere"},
  {"geforce rtx 3050", GpuVendor::NVIDIA, 9.109, "ampere"},
  {"nvidia a100 sxm4 80gb", GpuVendor::NVIDIA, 77.600, "ampere"},
  {"nvidia a100 pcie 80gb", GpuVendor::NVIDIA, 77.600, "ampere"},
  {"nvidia a40", GpuVendor::NVIDIA, 37.418, "ampere"},
  {"nvidia a30", GpuVendor::NVIDIA, 10.320, "ampere"},
  {"nvidia a10", GpuVendor::NVIDIA, 31.240, "ampere"},
  {"rtx a6000", GpuVendor::NVIDIA, 38.710, "ampere"},
  {"rtx a5000", GpuVendor::NVIDIA, 27.768, "ampere"},
  {"rtx a4000", GpuVendor::NVIDIA, 19.173, "ampere"},
  {"rtx a2000", GpuVendor::NVIDIA, 15.866, "ampere"},

  // ── Ada Lovelace (Vulkan 1.3) ────────────────────────────────────────────
  {"geforce rtx 4090", GpuVendor::NVIDIA, 82.576, "ada"},
  {"geforce rtx 4080 super", GpuVendor::NVIDIA, 52.224, "ada"},
  {"geforce rtx 4080", GpuVendor::NVIDIA, 48.740, "ada"},
  {"geforce rtx 4070 ti super", GpuVendor::NVIDIA, 40.000, "ada"},
  {"geforce rtx 4070 ti", GpuVendor::NVIDIA, 40.092, "ada"},
  {"geforce rtx 4070 super", GpuVendor::NVIDIA, 35.490, "ada"},
  {"geforce rtx 4070", GpuVendor::NVIDIA, 29.151, "ada"},
  {"geforce rtx 4060 ti", GpuVendor::NVIDIA, 22.060, "ada"},
  {"geforce rtx 4060", GpuVendor::NVIDIA, 15.115, "ada"},
  {"geforce rtx 4050", GpuVendor::NVIDIA, 12.413, "ada"},
  {"nvidia l40s", GpuVendor::NVIDIA, 91.610, "ada"},
  {"nvidia l40", GpuVendor::NVIDIA, 90.520, "ada"},
  {"nvidia l4", GpuVendor::NVIDIA, 30.290, "ada"},
  {"rtx 6000 ada", GpuVendor::NVIDIA, 91.100, "ada"},
  {"rtx 5000 ada", GpuVendor::NVIDIA, 65.300, "ada"},
  {"rtx 4500 ada", GpuVendor::NVIDIA, 48.740, "ada"},
  {"rtx 4000 ada", GpuVendor::NVIDIA, 26.730, "ada"},

  // ── Blackwell (Vulkan 1.4) ───────────────────────────────────────────────
  {"geforce rtx 5090", GpuVendor::NVIDIA, 209.000, "blackwell"},
  {"geforce rtx 5080", GpuVendor::NVIDIA, 137.000, "blackwell"},
  {"geforce rtx 5070 ti", GpuVendor::NVIDIA, 107.000, "blackwell"},
  {"geforce rtx 5070", GpuVendor::NVIDIA, 74.000, "blackwell"},
  {"geforce rtx 5060 ti", GpuVendor::NVIDIA, 54.200, "blackwell"},
  {"geforce rtx 5060", GpuVendor::NVIDIA, 40.000, "blackwell"},
  {"nvidia b200", GpuVendor::NVIDIA, 144.000, "blackwell"},
  {"nvidia b100", GpuVendor::NVIDIA, 116.000, "blackwell"},
  {"nvidia b40", GpuVendor::NVIDIA, 90.000, "blackwell"},

  // ══════════════════════════════════════════════════════════════════════════
  //  AMD
  // ══════════════════════════════════════════════════════════════════════════

  // ── GCN 1st Gen / 2nd Gen (Vulkan 1.0) ──────────────────────────────────
  //   HD 7000-series (Southern Islands) and R9/R7 200-series
  {"radeon hd 7970", GpuVendor::AMD, 3.789, "gcn 1.0"},
  {"radeon hd 7950", GpuVendor::AMD, 3.072, "gcn 1.0"},
  {"radeon hd 7870", GpuVendor::AMD, 2.560, "gcn 1.0"},
  {"radeon hd 7850", GpuVendor::AMD, 1.761, "gcn 1.0"},
  {"radeon r9 290x", GpuVendor::AMD, 5.632, "gcn 2.0"},
  {"radeon r9 290", GpuVendor::AMD, 4.848, "gcn 2.0"},
  {"radeon r9 280x", GpuVendor::AMD, 3.789, "gcn 2.0"},
  {"radeon r9 280", GpuVendor::AMD, 3.195, "gcn 2.0"},
  {"radeon r9 270x", GpuVendor::AMD, 2.688, "gcn 2.0"},
  {"radeon r9 270", GpuVendor::AMD, 2.508, "gcn 2.0"},

  // ── GCN 3rd Gen (Vulkan 1.1) — Tonga / Fiji ──────────────────────────────
  {"radeon r9 390x", GpuVendor::AMD, 5.914, "gcn 3.0"},
  {"radeon r9 390", GpuVendor::AMD, 5.120, "gcn 3.0"},
  {"radeon r9 380x", GpuVendor::AMD, 3.482, "gcn 3.0"},
  {"radeon r9 380", GpuVendor::AMD, 3.072, "gcn 3.0"},
  {"radeon r9 nano", GpuVendor::AMD, 8.192, "gcn 3.0"},
  {"radeon r9 fury x", GpuVendor::AMD, 8.602, "gcn 3.0"},
  {"radeon r9 fury", GpuVendor::AMD, 7.168, "gcn 3.0"},
  {"radeon pro duo (fiji)", GpuVendor::AMD, 16.384, "gcn 3.0"},

  // ── GCN 4th Gen (Polaris) — Vulkan 1.2 ───────────────────────────────────
  {"radeon rx 480", GpuVendor::AMD, 5.834, "gcn 4.0"},
  {"radeon rx 470", GpuVendor::AMD, 4.940, "gcn 4.0"},
  {"radeon rx 460", GpuVendor::AMD, 2.200, "gcn 4.0"},
  {"radeon rx 580", GpuVendor::AMD, 6.175, "gcn 4.0"},
  {"radeon rx 570", GpuVendor::AMD, 5.095, "gcn 4.0"},
  {"radeon rx 560", GpuVendor::AMD, 2.611, "gcn 4.0"},
  {"radeon rx 550", GpuVendor::AMD, 1.211, "gcn 4.0"},
  {"radeon rx 590", GpuVendor::AMD, 7.119, "gcn 4.0"},

  // ── GCN 5th Gen (Vega) — Vulkan 1.2 ─────────────────────────────────────
  {"radeon rx vea 64", GpuVendor::AMD, 13.700, "gcn 5.0"},
  {"radeon rx vea 56", GpuVendor::AMD, 10.500, "gcn 5.0"},
  {"radeon vii", GpuVendor::AMD, 13.800, "gcn 5.0"},
  {"radeon instinct mi50", GpuVendor::AMD, 13.400, "gcn 5.0"},
  {"radeon instinct mi60", GpuVendor::AMD, 14.700, "gcn 5.0"},

  // ── RDNA 1 (Navi 10/14) — Vulkan 1.3 ────────────────────────────────────
  {"radeon rx 5700 xt", GpuVendor::AMD, 9.754, "rdna 1"},
  {"radeon rx 5700", GpuVendor::AMD, 7.949, "rdna 1"},
  {"radeon rx 5600 xt", GpuVendor::AMD, 7.194, "rdna 1"},
  {"radeon rx 5600", GpuVendor::AMD, 6.656, "rdna 1"},
  {"radeon rx 5500 xt", GpuVendor::AMD, 5.196, "rdna 1"},
  {"radeon rx 5500", GpuVendor::AMD, 5.196, "rdna 1"},
  {"radeon rx 5300", GpuVendor::AMD, 3.481, "rdna 1"},

  // ── RDNA 2 (Navi 21/22/23/24) — Vulkan 1.3 ──────────────────────────────
  {"radeon rx 6950 xt", GpuVendor::AMD, 23.650, "rdna 2"},
  {"radeon rx 6900 xt", GpuVendor::AMD, 23.040, "rdna 2"},
  {"radeon rx 6800 xt", GpuVendor::AMD, 20.740, "rdna 2"},
  {"radeon rx 6800", GpuVendor::AMD, 16.170, "rdna 2"},
  {"radeon rx 6750 xt", GpuVendor::AMD, 13.850, "rdna 2"},
  {"radeon rx 6700 xt", GpuVendor::AMD, 13.210, "rdna 2"},
  {"radeon rx 6700", GpuVendor::AMD, 11.400, "rdna 2"},
  {"radeon rx 6650 xt", GpuVendor::AMD, 10.790, "rdna 2"},
  {"radeon rx 6600 xt", GpuVendor::AMD, 10.600, "rdna 2"},
  {"radeon rx 6600", GpuVendor::AMD, 8.928, "rdna 2"},
  {"radeon rx 6500 xt", GpuVendor::AMD, 5.765, "rdna 2"},
  {"radeon rx 6400", GpuVendor::AMD, 3.573, "rdna 2"},
  {"radeon pro w6800", GpuVendor::AMD, 17.830, "rdna 2"},
  {"radeon pro w6600", GpuVendor::AMD, 9.991, "rdna 2"},
  {"amd instinct mi210", GpuVendor::AMD, 45.260, "rdna 2"},

  // ── RDNA 3 (Navi 31/32/33) — Vulkan 1.3 ─────────────────────────────────
  {"radeon rx 7900 xtx", GpuVendor::AMD, 61.390, "rdna 3"},
  {"radeon rx 7900 xt", GpuVendor::AMD, 53.690, "rdna 3"},
  {"radeon rx 7900 gre", GpuVendor::AMD, 45.980, "rdna 3"},
  {"radeon rx 7800 xt", GpuVendor::AMD, 37.320, "rdna 3"},
  {"radeon rx 7700 xt", GpuVendor::AMD, 34.980, "rdna 3"},
  {"radeon rx 7700", GpuVendor::AMD, 28.990, "rdna 3"},
  {"radeon rx 7600 xt", GpuVendor::AMD, 24.390, "rdna 3"},
  {"radeon rx 7600", GpuVendor::AMD, 21.500, "rdna 3"},
  {"radeon rx 7500 xt", GpuVendor::AMD, 15.990, "rdna 3"},
  {"radeon pro w7900", GpuVendor::AMD, 61.300, "rdna 3"},
  {"radeon pro w7800", GpuVendor::AMD, 45.200, "rdna 3"},
  {"amd instinct mi300x", GpuVendor::AMD, 163.400, "rdna 3"},
  {"amd instinct mi300a", GpuVendor::AMD, 122.600, "rdna 3"},

  // ── RDNA 4 (Navi 48/44) — Vulkan 1.4 ────────────────────────────────────
  {"radeon rx 9070 xt", GpuVendor::AMD, 48.700, "rdna 4"},
  {"radeon rx 9070", GpuVendor::AMD, 38.700, "rdna 4"},
  {"radeon rx 9060 xt", GpuVendor::AMD, 28.400, "rdna 4"},

  // ══════════════════════════════════════════════════════════════════════════
  //  Intel Arc
  // ══════════════════════════════════════════════════════════════════════════

  // ── Alchemist (Vulkan 1.3) ───────────────────────────────────────────────
  {"intel arc a770", GpuVendor::INTEL, 19.660, "alchemist"},
  {"intel arc a750", GpuVendor::INTEL, 17.200, "alchemist"},
  {"intel arc a580", GpuVendor::INTEL, 12.400, "alchemist"},
  {"intel arc a380", GpuVendor::INTEL, 7.190, "alchemist"},
  {"intel arc a310", GpuVendor::INTEL, 3.500, "alchemist"},
  {"intel arc pro a60", GpuVendor::INTEL, 14.660, "alchemist"},
  {"intel arc pro a40", GpuVendor::INTEL, 8.790, "alchemist"},
  {"intel arc pro a30m", GpuVendor::INTEL, 7.320, "alchemist"},

  // ── Battlemage (Vulkan 1.4) ──────────────────────────────────────────────
  {"intel arc b580", GpuVendor::INTEL, 14.570, "battlemage"},
  {"intel arc b570", GpuVendor::INTEL, 11.340, "battlemage"},
};

} // namespace

void PhysicalDeviceSet::guessAdditionalInformation()
{
  String lowcaseDeviceName;
  lowcaseDeviceName.setStr(properties.deviceName);
  lowcaseDeviceName.toLower();
  const DeviceInfo *info = nullptr;
  uint32_t matchLength = 0;
  for (const DeviceInfo &i : deviceInfos)
  {
    if (i.vendor != vendor)
      continue;

    if (strstr(lowcaseDeviceName, i.name))
    {
      uint32_t len = strlen(i.name);
      if (len > matchLength)
      {
        info = &i;
        matchLength = len;
      }
    }
  }

  if (!info)
  {
    additional.uarch = "<unknown>";
    additional.tflops = 0;
    return;
  }

  additional.uarch = info->uarch;
  additional.tflops = info->tflops;
}

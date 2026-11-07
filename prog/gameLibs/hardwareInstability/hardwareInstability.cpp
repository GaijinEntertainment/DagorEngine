// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <hardwareInstability/hardwareInstability.h>

#include <ioSys/dag_dataBlock.h>
#include <folders/folders.h>
#include <osApiWrappers/dag_messageBox.h>
#include <util/dag_localization.h>
#include <util/dag_delayedAction.h>
#include <util/dag_watchdog.h>

#include <debug/dag_debug.h>

#include <EASTL/string.h>

// Bump to re-issue warnings
static constexpr int GENERATION = 0;

static constexpr uint32_t VMIN_FIXED_MICROCODE_REVISION = 0x12F;

#if _TARGET_PC_WIN && !defined(_M_ARM64) && !defined(_M_ARM64EC)
#include <windows.h>
#if defined(__clang__)
#include <cpuid.h>
#else
#include <intrin.h>
#endif

static uint32_t get_bios_microcode_revision()
{
  uint8_t value[8] = {};
  DWORD size = sizeof(value);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "Update Revision", RRF_RT_REG_BINARY,
        nullptr, value, &size) != ERROR_SUCCESS ||
      size != sizeof(value))
    return 0;
  // The revision is the high dword of the MSR value
  uint32_t revision;
  memcpy(&revision, value + 4, sizeof(revision));
  return revision;
}

static bool cpu_is_vmin_shift_instability_vulnerable()
{
#if defined(__clang__)
#define CPUID(op, eax, ebx, ecx, edx) __get_cpuid(op, &eax, &ebx, &ecx, &edx)
#else
#define CPUID(op, eax, ebx, ecx, edx) \
  ([&] {                              \
    int regs[4];                      \
    __cpuid(regs, op);                \
    eax = regs[0];                    \
    ebx = regs[1];                    \
    ecx = regs[2];                    \
    edx = regs[3];                    \
  }())
#endif
  uint32_t cpuVendorId[3];
  uint32_t cpuVersionInfo;
  uint32_t unused, unused1, unused2;
  CPUID(0, unused, cpuVendorId[0], cpuVendorId[2], cpuVendorId[1]);
  CPUID(1, cpuVersionInfo, unused, unused1, unused2);
  // Intel, version 0xb0671
  return cpuVersionInfo == 0xb0671 && cpuVendorId[0] == 0x756e6547 && cpuVendorId[1] == 0x49656e69 && cpuVendorId[2] == 0x6c65746e;
#undef CPUID
}
#else
static bool cpu_is_vmin_shift_instability_vulnerable() { return false; }
static uint32_t get_bios_microcode_revision() { return 0; }
#endif

namespace hardware_instability
{

static String get_state_path()
{
  String dir = folders::get_local_appdata_dir();
  return dir.empty() ? String() : String(0, "%s/hw_instability.blk", dir.c_str());
}

static bool load_state(DataBlock &blk)
{
  String path = get_state_path();
  return !path.empty() && dblk::load(blk, path.c_str(), dblk::ReadFlag::ROBUST);
}

static void save_state(const DataBlock &blk)
{
  String path = get_state_path();
  if (!path.empty())
    dblk::save_to_text_file(blk, path.c_str());
}

void schedule_detection_and_warning()
{
  bool vminVulnerable = cpu_is_vmin_shift_instability_vulnerable();

  bool anyVulnerable = vminVulnerable;

  if (!anyVulnerable)
    return;

  ::delayed_call([=] {
    DataBlock state;
    load_state(state);

    if (vminVulnerable)
    {
      DataBlock &vminBlk = *state.addBlock("vmin");

      uint32_t microcode = get_bios_microcode_revision();
      logwarn("[HW_INSTABILITY] vulnerable cpu family 0xb0671, microcode revision 0x%X", microcode);

      int lastIssuedGeneration = vminBlk.getInt("lastIssuedGeneration", -1);

      if (lastIssuedGeneration < GENERATION)
      {
        const char *link = "https://www.intel.com/content/www/us/en/support/articles/000102331.html";
        bool needBiosUpdate = microcode < VMIN_FIXED_MICROCODE_REVISION;

        const char *text = needBiosUpdate
                             ? get_localized_text("hardware_instability/textVminUpdateBios",
                                 "Your CPU model (13th or 14th generation Intel Core, CPUID 0xB0671) has known stability issues that "
                                 "can damage it over time. We strongly advise you to update your BIOS to the latest version as soon "
                                 "as possible. You may experience random crashes and issues. You can read more at:")
                             : get_localized_text("hardware_instability/textVminDamage",
                                 "Your CPU model (13th or 14th generation Intel Core, CPUID 0xB0671) has known stability issues that "
                                 "could have damaged it. If you experience random crashes and issues, your CPU can be the cause. You "
                                 "can read more at:");
        // The link is not in the translations, so translators can not break the markup
        eastl::string message{eastl::string::CtorSprintf{}, "%s\n<a href=\"%s\">%s</a>", text, link, link};

        ScopeSetWatchdogTimeout _wd(WATCHDOG_DISABLE);
        ::os_message_box(message.c_str(), get_localized_text("hardware_instability/caption", "Hardware instability warning"),
          GUI_MB_OK | GUI_MB_ICON_WARNING | GUI_MB_FOREGROUND);
      }
      vminBlk.setInt("lastIssuedGeneration", GENERATION);
    }

    save_state(state);
  });
}

} // namespace hardware_instability

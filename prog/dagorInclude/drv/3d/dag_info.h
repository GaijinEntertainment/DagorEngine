//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/initializer_list.h>

#include <util/dag_globDef.h>

#include <drv/3d/dag_driverCode.h>

struct DagorDateTime;
struct Driver3dDesc;

/**
 * @brief Callback class for initializing the 3D driver.
 */
class Driver3dInitCallback
{
public:
  /**
   * @brief Struct representing a range of driver versions.
   */
  struct VersionRange
  {
    uint64_t minVersion; ///< The minimum driver version
    uint64_t maxVersion; ///< The maximum driver version
  };

  /**
   * @brief Struct representing the size of the render area.
   */
  struct RenderSize
  {
    int width;  ///< The width of the render area
    int height; ///< The height of the render area
  };

  using NeedStereoRenderFunc = bool (*)();              ///< Function pointer type for checking if stereo rendering is needed
  using StereoRenderDimensionFunc = int (*)();          ///< Function pointer type for getting the dimension of stereo rendering
  using StereoRenderExtensionsFunc = const char *(*)(); ///< Function pointer type for getting the extensions for stereo rendering
  using StereoRenderVersionsFunc = VersionRange (*)();  ///< Function pointer type for getting the supported versions for stereo
                                                        ///< rendering
  using StereoRenderAdapterFunc = int64_t (*)();        ///< Function pointer type for getting the adapter for stereo rendering

  /**
   * @brief Verifies the resolution settings.
   * @param ref_scr_wdt The reference screen width
   * @param ref_scr_hgt The reference screen height
   * @param base_scr_wdt The base screen width
   * @param base_scr_hgt The base screen height
   * @param window_mode Flag indicating if the window mode is enabled
   */
  virtual void verifyResolutionSettings(int &ref_scr_wdt, int &ref_scr_hgt, int base_scr_wdt, int base_scr_hgt, bool window_mode) const
  {
    G_UNUSED(ref_scr_wdt);
    G_UNUSED(ref_scr_hgt);
    G_UNUSED(base_scr_wdt);
    G_UNUSED(base_scr_hgt);
    G_UNUSED(window_mode);
  }

  /**
   * @brief Validates the driver description.
   * @param desc The driver description to validate
   * @return something
   * @todo This fucntion is not used anywhere. Remove it?
   */
  virtual int validateDesc(Driver3dDesc &desc) const = 0;

  /**
   * @brief Compares two driver descriptions.
   * @param A The first driver description
   * @param B The second driver description
   * @return something
   * @todo This fucntion is not used anywhere. Remove it?
   */
  virtual int compareDesc(Driver3dDesc &A, Driver3dDesc &B) const = 0;

  /**
   * @brief Checks if stereo rendering is desired.
   * @return True if stereo rendering is desired, false otherwise
   */
  virtual bool desiredStereoRender() const { return false; }

  /**
   * @brief Gets the desired adapter
   * @return The desired adapter ID
   */
  virtual int64_t desiredAdapter() const { return 0; }

  /**
   * @brief Gets the desired size of the renderer.
   * @return The desired size of the renderer
   */
  virtual RenderSize desiredRendererSize() const { return {0, 0}; }

  /**
   * @brief Gets the desired device extensions for the renderer.
   * @return The desired device extensions for the renderer
   */
  virtual const char *desiredRendererDeviceExtensions() const { return nullptr; }

  /**
   * @brief Gets the desired instance extensions for the renderer.
   * @return The desired instance extensions for the renderer
   */
  virtual const char *desiredRendererInstanceExtensions() const { return nullptr; }

  /**
   * @brief Gets the desired version range for the renderer.
   * @return The desired version range for the renderer
   */
  virtual VersionRange desiredRendererVersionRange() const { return {0, 0}; }
};

/**
 * @brief Enum representing the level of API support for a current hardware.
 * Currently it is only used to check DX12 support with a fallback on DX11 if the support is not full.
 */
enum class APISupport
{
  FULL_SUPPORT,       ///< Full support for the API
  OUTDATED_OS,        ///< The Windows version is outdated
  OUTDATED_DRIVER,    ///< The driver is outdated
  BLACKLISTED_DRIVER, ///< The driver is blacklisted (we know about bugs in the driver)
  NO_DEVICE_FOUND     ///< No compatible device found
};

namespace d3d
{

/**
 * @brief Guesses and returns the GPU vendor ID.
 * @param out_gpu_desc Pointer to store the GPU description
 * @param out_drv_ver Pointer to store the driver version
 * @param out_drv_date Pointer to store the driver date
 * @param device_id Pointer to store the device ID
 * @return The GPU vendor ID for enum D3D_VENDOR_...
 */
int guess_gpu_vendor(String *out_gpu_desc = NULL, uint32_t *out_drv_ver = NULL, DagorDateTime *out_drv_date = NULL,
  uint32_t *device_id = nullptr);

/**
 * @brief Gets the driver date for the GPU vendor.
 * @param vendor The GPU vendor ID
 * @return The driver date for the GPU vendor
 */
DagorDateTime get_gpu_driver_date(int vendor);

/**
 * @brief Determines and returns the size of the dedicated GPU memory in KB.
 * @return The size of the dedicated GPU memory in KB
 */
unsigned get_dedicated_gpu_memory_size_kb();

/**
 * @brief Determines and returns the size of the free dedicated GPU memory in KB.
 * @return The size of the free dedicated GPU memory in KB
 */
unsigned get_free_dedicated_gpu_memory_size_kb();

/**
 * @brief Gets the current GPU memory during the game (supports only Nvidia GPUs).
 * @param dedicated_total Pointer to store the total dedicated GPU memory
 * @param dedicated_free Pointer to store the free dedicated GPU memory
 */
void get_current_gpu_memory_kb(uint32_t *dedicated_total, uint32_t *dedicated_free);

/**
 * @brief Gets the GPU frequency.
 * @param out_freq String to store the GPU frequency
 * @return True if the GPU frequency was successfully retrieved, false otherwise
 * @note This function works only for Nvidia GPUs
 */
bool get_gpu_freq(String &out_freq);

/**
 * @brief Gets the GPU temperature.
 * @return The GPU temperature
 * @note This function works only for Nvidia GPUs
 */
int get_gpu_temperature();

/**
 * @brief Gets the video vendor string.
 * @param out_str String to store the video vendor string
 */
void get_video_vendor_str(String &out_str);

/**
 * @brief Gets the display scale.
 * @return The display scale
 */
float get_display_scale();

/**
 * @brief Disables SLI settings for the profile.
 * @todo Remove. We don't support SLI anymore
 */
void disable_sli();

/**
 * @brief Gets the driver name.
 * @return The driver name
 */
const char *get_driver_name();

/**
 * @brief Gets the driver code.
 * @return The driver code
 */
#if _TARGET_XBOX
inline constexpr DriverCode get_driver_code() { return DriverCode::make(d3d::dx12); }
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_ANDROID | _TARGET_C3
inline constexpr DriverCode get_driver_code() { return DriverCode::make(d3d::vulkan); }
#elif _TARGET_IOS
inline constexpr DriverCode get_driver_code() { return DriverCode::make(d3d::metal); }
#else
DriverCode get_driver_code();
#endif

/**
 * @brief Checks if the d3d-stub driver is used.
 * @return True if the d3d-stub driver is used, false otherwise
 */
static inline bool is_stub_driver() { return get_driver_code().is(d3d::stub); }

/**
 * @brief Gets the device driver version.
 * @return The device driver version
 * @note Work only for Vulkan. On other platforms returns "1.0"
 */
const char *get_device_driver_version();

/**
 * @brief Gets the device name.
 * @return The device name
 */
const char *get_device_name();

/**
 * @brief Gets the last error message.
 * @return The last error message
 */
const char *get_last_error();

/**
 * @brief Gets the last error code.
 * @return The last error code
 */
uint32_t get_last_error_code();

/**
 * @brief Gets the raw pointer to the device interface (implementation and platform specific).
 * @return The raw pointer to the device interface
 */
void *get_device();

/**
 * @brief Gets the raw pointer to the device context (implementation and platform specific).
 * @return The raw pointer to the device context
 */
void *get_context();

/**
 * @brief Gets the driver description.
 * @return The driver description
 */
const Driver3dDesc &get_driver_desc();

/**
 * @brief Checks if the device is in device reset or being reset.
 * @return True if the device is in device reset or being reset, false otherwise
 */
bool is_in_device_reset_now();

/**
 * @brief Checks if the game rendering window is completely occluded.
 * @return True if the game rendering window is completely occluded, false otherwise
 */
bool is_window_occluded();

/**
 * @brief Checks if compute commands should be preferred for image processing.
 * @param formats The list of image formats
 * @return True if compute commands should be preferred, false otherwise
 * @note Implemented only for DX11
 */
bool should_use_compute_for_image_processing(std::initializer_list<unsigned> formats);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline const char *get_driver_name() { return d3di.driverName; }
inline const char *get_device_driver_version() { return d3di.driverVer; }
inline const char *get_device_name() { return d3di.deviceName; }
inline const char *get_last_error() { return d3di.get_last_error(); }
inline uint32_t get_last_error_code() { return d3di.get_last_error_code(); }

inline void *get_device() { return d3di.get_device(); }

inline const Driver3dDesc &get_driver_desc() { return d3di.drvDesc; }

inline bool is_in_device_reset_now() { return d3di.is_in_device_reset_now(); }
inline bool is_window_occluded() { return d3di.is_window_occluded(); }

inline bool should_use_compute_for_image_processing(std::initializer_list<unsigned> formats)
{
  return d3di.should_use_compute_for_image_processing(formats);
}
} // namespace d3d
#endif

# AMD AGS Library Changelog

### v5.2.1 - 2018-08-09
* Fix for app registration for DX12 UWP apps

### v5.2.0 - 2018-05-31
* App registration for DX12 apps
* Breaking API change for DX12 extensions
  * You now call `agsDriverExtensionsDX12_CreateDevice()`
  * With a corresponsding call to `agsDriverExtensionsDX12_DestroyDevice()` during teardown
* DX11 breadcrumb markers
  * `agsDriverExtensionsDX11_WriteBreadcrumb()`

### v5.1.1 - 2017-09-19
* Breaking API change for DX11 extensions
  * You now call `agsDriverExtensionsDX11_CreateDevice()` at creation time to access any DX11 AMD extensions
  * With a corresponding call to `agsDriverExtensionsDX11_DestroyDevice()` during teardown
* App registration extension for DX11 apps
* FreeSync 2 HDR support
* Scan and reduce wave-level shader intrinsics
* DX12 user markers!
* Static libs in various flavours!

### v5.0.6 - 2016-03-28
* Fix min/max/avg luminance in display info
* Clean up documentation
* Add WACK-compliant version of the library

### v5.0.5 - 2016-12-08
* Add function to set displays into HDR mode
* Implement full GPU enumeration with adapter string, device id, revision id, and vendor id
* Implement per-GPU display enumeration including information on display name, resolution, and HDR capabilities
* Remove `agsGetGPUMemorySize` and `agsGetEyefinityConfigInfo` in favor of including this information in the device and display enumeration
* Add optional user-supplied memory allocator
* Add DirectX11 shader compiler controls
* Add DirectX11 multiview extension
  * Requires Radeon Software Crimson ReLive Edition 16.12.1 (driver version 16.50.2001) or later
* Update DirectX11 Crossfire API to support using the API without needing a driver profile
* Update DirectX11 Crossfire API to allow specifying the transfer engine

### v4.0.3 - 2016-08-18
* Improve support for DirectX 11 and DirectX 12 GCN shader extensions
* Add support for Multidraw Indirect Count Indirect for DirectX 11
* Fix clock speed information for Polaris GPUs
* Requires Radeon Software Crimson Edition 16.9.2 (driver version 16.40.2311) or later

### v4.0.0 - 2016-05-24
* Add support for GCN shader extensions
  * Shader extensions are exposed for both DirectX 11 and DirectX 12
  * Requires Radeon Software Crimson Edition 16.5.2 or later
* Remove `RegisterApp` from the extension API
  * This extension is not currently supported in the driver

### v3.2.2 - 2016-05-23
* Add back `radeonSoftwareVersion` now that updated driver is public
  * Radeon Software Crimson Edition 16.5.2 or later
* Fix GPU info when primary adapter is > 0
* Update the implementation of agsDriverExtensions_NotifyResourceEndWrites

### v3.2.0 - 2016-02-12
* Add ability to disable Crossfire
  * This is in addition to the existing ability to enable the explicit Crossfire API
  * Desired Crossfire mode is now passed in to `agsInit`
  * Separate `SetCrossfireMode` function has been removed from the AGS API
  * The `agsInit` function should now be called **prior to device creation**
* Return library version number in the optional info parameter of `agsInit`
* Build amd_ags DLLs such that they do not depend on any Microsoft Visual C++ redistributable packages

### v3.1.1 - 2016-01-28
* Return null for the context when initialization fails
* Add version number defines to `amd_ags.h`
* Remove `radeonSoftwareVersion` until needed driver update is public

### v3.1.0 - 2016-01-26
* Initial release on GitHub

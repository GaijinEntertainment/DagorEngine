// include prog/engine/drv/drv3d_pc_multi/hlsl.cpp directly
#include <hlsl_dx.cpp>

// stubs for irrelevant code/data
DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::null); }
VPROG drv3d_dx11::create_vertex_shader_unpacked(const void *, uint32_t, uint32_t, bool) { return -1; }
FSHADER drv3d_dx11::create_pixel_shader_unpacked(const void *, uint32_t, uint32_t, bool) { return -1; }
__declspec(thread) HRESULT drv3d_dx11::last_hres = S_OK;

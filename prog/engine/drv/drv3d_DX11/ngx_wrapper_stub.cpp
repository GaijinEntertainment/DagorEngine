#include "ngx_wrapper.h"

using namespace drv3d_dx11;

#include "ngx_wrapper_impl_stub.inl.cpp"

drv3d_dx11::NgxWrapper drv3d_dx11::ngx_wrapper;

bool drv3d_dx11::init_ngx(const char *log_dir)
{
  shutdown_ngx();
  return ngx_wrapper.init(nullptr, log_dir);
}

void drv3d_dx11::shutdown_ngx() { ngx_wrapper.shutdown(nullptr); }
#include <3d/dag_drv3d.h>
#include <3d/dag_sbufferIDHolder.h>
#include <shaders/dag_shaders.h>

void SbufferIDHolder::set(Sbuffer *buf_, const char *name)
{
  close();
  if (buf_)
  {
    this->buf = buf_;
    bufId = register_managed_res(name, buf_);
  }
}

void SbufferIDHolder::close()
{
  if (bufId != BAD_D3DRESID)
    ShaderGlobal::reset_from_vars(bufId);
  release_managed_buf_verified(bufId, buf);
}

void SbufferIDHolderWithVar::set(Sbuffer *buf_, const char *name)
{
  close();
  if (buf_)
  {
    this->buf = buf_;
    bufId = register_managed_res(name, buf_);
  }
  varId = get_shader_variable_id(name, true);
}

void SbufferIDHolderWithVar::close()
{
  if (bufId != BAD_D3DRESID)
    ShaderGlobal::reset_from_vars(bufId);
  release_managed_buf_verified(bufId, buf);
  varId = -1;
}

void SbufferIDHolderWithVar::setVar() const
{
  if (varId > 0)
    ShaderGlobal::set_buffer(varId, bufId);
}

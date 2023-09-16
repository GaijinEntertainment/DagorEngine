#include "driver.h"

// #include "immediateConstStub.inc.cpp"

// this is a bit optimized version, basically less ifs and context locks
#include "immediateConstStub.h"

#include <3d/dag_drv3d_buffers.h>
#ifdef IMMEDIATE_CB_NAMESPACE
IMMEDIATE_CB_NAMESPACE
{
#endif
  static carray<Sbuffer *, STAGE_MAX> immediate_cb;
  bool init_immediate_cb()
  {
    if (immediate_cb[STAGE_CS] && immediate_cb[STAGE_PS] && immediate_cb[STAGE_VS])
      return true;
    immediate_cb[STAGE_CS] = d3d_buffers::create_persistent_cb(1, "_immediate_cb_cs");
    immediate_cb[STAGE_PS] = d3d_buffers::create_persistent_cb(1, "_immediate_cb_ps");
    immediate_cb[STAGE_VS] = d3d_buffers::create_persistent_cb(1, "_immediate_cb_vs");
    return immediate_cb[STAGE_CS] && immediate_cb[STAGE_PS] && immediate_cb[STAGE_VS];
  }

  void term_immediate_cb()
  {
    for (auto &cb : immediate_cb)
      del_d3dres(cb);
  }
#ifdef IMMEDIATE_CB_NAMESPACE
}
#endif

bool d3d::set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words)
{
  G_ASSERT(num_words <= 4);
  G_ASSERT(data || !num_words);
  if (num_words)
  {
    // d3d::set_const_buffer(stage, IMMEDAITE_CB_REGISTER, immediate_cb[stage]);//we assume shadow state is managed by driver, and API
    // call won't be initiated if not needed!
    const uint32_t slot = IMMEDAITE_CB_REGISTER;
    GenericBuffer *vb = (GenericBuffer *)immediate_cb[stage];
    if (g_render_state.constants.customBuffers[stage][slot] != vb->buffer)
    {
      g_render_state.constants.customBuffers[stage][slot] = vb->buffer;
      g_render_state.constants.constantsBufferChanged[stage] |= 1 << slot;
      g_render_state.constants.buffersFirstConstants[stage][slot] = 0;
      g_render_state.constants.buffersNumConstants[stage][slot] = 16;
      if (g_render_state.constants.customBuffersMaxSlotUsed[stage] < slot)
        g_render_state.constants.customBuffersMaxSlotUsed[stage] = slot;
    }
    ContextAutoLock contextLock;
    D3D11_MAPPED_SUBRESOURCE lockMsr;
    HRESULT hr = dx_context->Map(vb->buffer, NULL, D3D11_MAP_WRITE_DISCARD, 0, &lockMsr);
    if (FAILED(hr))
      return false;
    if (num_words == 1) // most typical usage
      *(uint32_t *)lockMsr.pData = *data;
    else
      memcpy(lockMsr.pData, data, num_words * 4);
    dx_context->Unmap(vb->buffer, NULL);
    return true;
  }
  else
    d3d::set_const_buffer(stage, IMMEDAITE_CB_REGISTER, nullptr); // we assume shadow state is managed by driver, and API call won't be
                                                                  // initiated if not needed!
  return true;
}

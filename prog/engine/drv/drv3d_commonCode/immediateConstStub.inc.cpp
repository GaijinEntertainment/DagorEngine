#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <generic/dag_carray.h>
#include "immediateConstStub.h"
#ifdef IMMEDIATE_CB_NAMESPACE
IMMEDIATE_CB_NAMESPACE
{
#endif
  static carray<Sbuffer *, STAGE_MAX> immediate_cb;
  bool init_immediate_cb()
  {
    immediate_cb[STAGE_CS] = d3d::buffers::create_persistent_cb(1, "_immediate_cb_cs");
    immediate_cb[STAGE_PS] = d3d::buffers::create_persistent_cb(1, "_immediate_cb_ps");
    immediate_cb[STAGE_VS] = d3d::buffers::create_persistent_cb(1, "_immediate_cb_vs");
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
  D3D_CONTRACT_ASSERT(num_words <= 4);
  D3D_CONTRACT_ASSERT(data || !num_words);
  if (num_words)
  {
    d3d::set_const_buffer(stage, IMMEDAITE_CB_REGISTER, immediate_cb[stage]); // we assume shadow state is managed by driver, and API
                                                                              // call won't be initiated if not needed!
    return immediate_cb[stage]->updateDataWithLock(0, num_words * 4, data, VBLOCK_DISCARD);
  }
  else
    d3d::set_const_buffer(stage, IMMEDAITE_CB_REGISTER, nullptr); // we assume shadow state is managed by driver, and API call won't be
                                                                  // initiated if not needed!
  return true;
}
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_materialData.h>
#include <shaders/dag_DynamicShaderHelper.h>


bool DynamicShaderHelper::init(const char *shader_name, CompiledShaderChannelId *channels, int num_channels, const char *module_info,
  bool optional /*= false*/)
{
  Ptr<MaterialData> matNull = new MaterialData;
  matNull->className = shader_name;
  material = new_shader_material(*matNull, false, !optional);
  matNull = NULL;
  if (!material)
  {
    if (!optional)
      DAG_FATAL("No shader for class = %s", shader_name);

    return false;
  }

  if (channels && !material->checkChannels(channels, num_channels))
  {
    struct CountChannels : public ShaderChannelsEnumCB
    {
      int cnt = 0, code_flags = 0;
      void enum_shader_channel(int, int, int, int, int, ChannelModifier, int) override { cnt++; }
    };
    CountChannels ch_cnt;
    if (!material->enum_channels(ch_cnt, ch_cnt.code_flags) || !ch_cnt.cnt)
    {
      if (!optional)
        logerr("%s - stub shader used for '%s'!", module_info, material->getShaderClassName());
      material = NULL;
      return true;
    }

    DAG_FATAL("%s - invalid channels for shader '%s'!", module_info, material->getShaderClassName());
    material = NULL;
    return false;
  }

  shader = material->make_elem();
  if (!shader)
  {
    if (!optional)
      DAG_FATAL("%s - empty shader '%s'!", module_info, material->getShaderClassName());
    material = NULL;
    return false;
  }
  return true;
}

void DynamicShaderHelper::close()
{
  shader = NULL;
  material = NULL;
}

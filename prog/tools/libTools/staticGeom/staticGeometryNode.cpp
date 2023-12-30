#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/matFlags.h>

#include <obsolete/dag_cfg.h>

#include <math/dag_boundingSphere.h>

#include <debug/dag_debug.h>


//==================================================================================================
void StaticGeometryNode::calcBoundSphere()
{
  boundSphere.setempty();

  if (mesh)
    boundSphere = ::mesh_bounding_sphere(mesh->mesh.getVert().data(), mesh->mesh.getVert().size());
}


//==================================================================================================
void StaticGeometryNode::calcBoundBox()
{
  boundBox.setempty();

  if (mesh)
    for (int i = 0; i < mesh->mesh.getVert().size(); ++i)
      boundBox += mesh->mesh.getVert()[i];
}


//==================================================================================================
bool StaticGeometryNode::haveBillboardMat() const
{
  for (int i = 0; i < mesh->mats.size(); ++i)
    if (mesh->mats[i] && (mesh->mats[i]->flags & MatFlags::FLG_BILLBOARD_MASK))
      return true;

  return false;
}


//==================================================================================================
bool StaticGeometryNode::checkBillboard()
{
  if (flags & StaticGeometryNode::FLG_BILLBOARD_MESH)
    return true;

  if (haveBillboardMat())
    flags |= StaticGeometryNode::FLG_BILLBOARD_MESH;

  return flags & StaticGeometryNode::FLG_BILLBOARD_MESH;
}


//==================================================================================================
void StaticGeometryNode::setScriptLighting(String &script, const char *lighting)
{
  CfgReader cfg;
  cfg.getdiv_text(String("[x]\r\n") + script, "x");

  script.clear();
  CfgDiv &div = cfg;

  for (int i = 0; i < div.var.size(); ++i)
  {
    CfgVar &v = div.var[i];

    if (v.id && v.val)
    {
      const char *val = v.val;

      if (!stricmp(v.id, "lighting"))
        val = lighting;

      script.aprintf(512, "%s=%s\r\n", v.id, val);
    }
  }
}


//==================================================================================================
void StaticGeometryNode::getScriptLighting(const char *script, String &lighting)
{
  CfgReader cfg;
  cfg.getdiv_text(String("[x]\r\n") + script, "x");

  CfgDiv &div = cfg;

  for (int i = 0; i < div.var.size(); ++i)
  {
    CfgVar &v = div.var[i];
    if (v.id && v.val && !stricmp(v.id, "lighting"))
    {
      lighting = v.val;
      return;
    }
  }
}


//==================================================================================================
void StaticGeometryNode::setMaterialLighting(StaticGeometryMaterial &mat) const
{
  if (lighting != LIGHT_DEFAULT)
  {
    const char *scriptLight = NULL;

    switch (lighting)
    {
      case LIGHT_DEFAULT:
      default:
        scriptLight = "none";
        mat.flags &= ~MatFlags::FLG_USE_LM;
        mat.flags &= ~MatFlags::FLG_USE_VLM;
        break;
// we don't have lightmaps anymore
#if HAVE_LIGHTMAPS
      case LIGHT_LIGHTMAP:
        scriptLight = "lightmap";
        mat.flags &= ~MatFlags::FLG_USE_VLM;
        mat.flags |= MatFlags::FLG_USE_LM;
        break;
      case LIGHT_VLTMAP:
        scriptLight = "vltmap";
        mat.flags &= ~MatFlags::FLG_USE_LM;
        mat.flags |= MatFlags::FLG_USE_VLM;
        break;
      default: DAG_FATAL("Unknown lighting type");
#endif
    }

    String script(mat.scriptText);
    setScriptLighting(script, scriptLight);
    mat.scriptText = script;
  }
}


//==================================================================================================
const char *StaticGeometryNode::lightingToStr(Lighting light)
{
  switch (light)
  {
    case LIGHT_NONE: return "none";
    case LIGHT_LIGHTMAP: return "ltmap";
    case LIGHT_VLTMAP: return "vltmap";
  }

  return "default";
}


//==================================================================================================
StaticGeometryNode::Lighting StaticGeometryNode::strToLighting(const char *light)
{
  if (light)
  {
    if (!stricmp(light, "none"))
      return LIGHT_NONE;
    else if (!stricmp(light, "ltmap"))
      return LIGHT_LIGHTMAP;
    else if (!stricmp(light, "vltmap"))
      return LIGHT_VLTMAP;
  }

  return LIGHT_DEFAULT;
}

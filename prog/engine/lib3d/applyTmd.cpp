#include <3d/dag_tex3d.h>
#include <util/dag_texMetaData.h>
#include <3d/dag_texMgr.h>

void apply_gen_tex_props(BaseTexture *t, const TextureMetaData &tmd, bool force_addr_from_tmd)
{
  if (force_addr_from_tmd || (tmd.flags & tmd.FLG_OVERRIDE))
  {
    t->texaddru(tmd.d3dTexAddr(tmd.addrU));
    t->texaddrv(tmd.d3dTexAddr(tmd.addrV));
    if (t->restype() == RES3D_VOLTEX)
      reinterpret_cast<VolTexture *>(t)->texaddrw(tmd.d3dTexAddr(tmd.addrW));
  }
  t->setAnisotropy(tmd.calcAnisotropy(::dgs_tex_anisotropy));
  if (tmd.needSetBorder())
    t->texbordercolor(tmd.borderCol);
  if (tmd.lodBias)
    t->texlod(tmd.lodBias / 1000.0f);
  if (tmd.needSetTexFilter())
    t->texfilter(tmd.d3dTexFilter());
  if (tmd.needSetMipFilter())
    t->texmipmap(tmd.d3dMipFilter());
}
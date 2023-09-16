#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/util/meshUtil.h>

#include <math/random/dag_random.h>
#include <ioSys/dag_genIo.h>
#include <util/dag_bitArray.h>
#include <shaders/dag_shaderCommon.h>
#include <fx/dag_leavesWind.h>

#include <debug/dag_debug.h>


void split_two_sided(Mesh &m, const PtrTab<class StaticGeometryMaterial> &mats)
{
  Bitarray materials_2sided;
  materials_2sided.resize(mats.size());
  m.clampMaterials(mats.size() - 1);
  for (int mi = 0; mi < mats.size(); ++mi)
    if ((mats[mi] && (mats[mi]->flags & (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED)) ==
                       (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED)))
      materials_2sided.set(mi);

  split_real_two_sided(m, materials_2sided, -1);
}


void StaticGeometryMaterial::save(IGenSave &cb, StaticGeometryMaterialSaveCB &texcb)
{
  cb.writeString(name);
  cb.writeString(className);
  cb.writeString(scriptText);

  cb.write(&amb, sizeof(amb));
  cb.write(&diff, sizeof(diff));
  cb.write(&spec, sizeof(spec));
  cb.write(&emis, sizeof(emis));

  cb.writeReal(power);

  for (int i = 0; i < NUM_TEXTURES; ++i)
    cb.writeInt(texcb.getTextureIndex(textures[i]));

  cb.write(&trans, sizeof(trans));
  cb.writeInt(atest);
  cb.writeReal(cosPower);

  cb.writeInt(flags);
}


void StaticGeometryMaterial::load(IGenLoad &cb, StaticGeometryMaterialLoadCB &texcb)
{
  cb.readString(name);
  cb.readString(className);
  cb.readString(scriptText);

  cb.read(&amb, sizeof(amb));
  cb.read(&diff, sizeof(diff));
  cb.read(&spec, sizeof(spec));
  cb.read(&emis, sizeof(emis));

  cb.readReal(power);

  for (int i = 0; i < NUM_TEXTURES; ++i)
    textures[i] = texcb.getTexture(cb.readInt());

  cb.read(&trans, sizeof(trans));
  cb.readInt(atest);
  cb.readReal(cosPower);

  cb.readInt(flags);
}

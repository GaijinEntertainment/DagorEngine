#include <gamePhys/phys/rendinstDestrRefl.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <daNet/reflection.h>
#include <EASTL/unique_ptr.h>

namespace rendinstdestr
{

class RendinstDestructableRefl : public danet::ReflectableObject
{
  DECL_REFLECTION(RendinstDestructableRefl, danet::ReflectableObject);

public:
  REFL_VAR_EXT(1, int, dummyForRestorables, 0, 0, DANET_ENCODER(restorablesCoder));
  static int restorablesCoder(DANET_ENCODER_SIGNATURE);

  RendinstDestructableRefl(mpi::ObjectID oid);

  void onRestorablesChanged() { dummyForRestorables.markAsChanged(); }
};
static eastl::unique_ptr<RendinstDestructableRefl> ri_destr_refl;

RendinstDestructableRefl::RendinstDestructableRefl(mpi::ObjectID oid) : danet::ReflectableObject(oid), dummyForRestorables(0) {}

int RendinstDestructableRefl::restorablesCoder(DANET_ENCODER_SIGNATURE)
{
  G_UNUSED(ro);
  G_UNUSED(meta);

  if (op == DANET_REFLECTION_OP_ENCODE)
    rendinstdestr::serialize_destr_data(*bs);
  else if (op == DANET_REFLECTION_OP_DECODE)
    rendinstdestr::deserialize_destr_data(*bs);
  return 1;
}

void init_refl_object(mpi::ObjectID oid) { ri_destr_refl.reset(new RendinstDestructableRefl(oid)); }
void destroy_refl_object() { ri_destr_refl.reset(); }
void refl_object_on_changed()
{
  if (ri_destr_refl)
    ri_destr_refl->onRestorablesChanged();
}

void enable_reflection_refl_object(bool enable)
{
  if (ri_destr_refl)
  {
    if (enable)
      ri_destr_refl->enableReflection();
    else
      ri_destr_refl->disableReflection(true);
  }
}

mpi::IObject *get_refl_object() { return ri_destr_refl.get(); }

} // namespace rendinstdestr

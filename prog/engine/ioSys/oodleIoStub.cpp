#include <ioSys/dag_oodleIo.h>

size_t oodle_compress(void *, size_t, const void *, size_t, int)
{
  G_ASSERTF_RETURN(false, 0, "%s(...) not implemented", __FUNCTION__);
}
size_t oodle_decompress(void *, size_t, const void *, size_t) { G_ASSERTF_RETURN(false, 0, "%s(...) not implemented", __FUNCTION__); }

int oodle_compress_data(IGenSave &, IGenLoad &, int, int) { G_ASSERTF_RETURN(false, 0, "%s(...) not implemented", __FUNCTION__); }

int oodle_decompress_data(IGenSave &, IGenLoad &, int, int) { G_ASSERTF_RETURN(false, 0, "%s(...) not implemented", __FUNCTION__); }

void OodleLoadCB::open(IGenLoad &in_crd, int, int)
{
  G_UNUSED(in_crd);
  G_ASSERTF(false, "%s not implemented (%s)", __FUNCTION__, in_crd.getTargetName());
}
void OodleLoadCB::close() {}


int OodleLoadCB::tryRead(void *, int) { G_ASSERTF_RETURN(false, 0, "%s(...) not implemented", __FUNCTION__); }

void OodleLoadCB::read(void *, int size)
{
  if (0 != size)
    DAGOR_THROW(LoadException("Oodle read error", -1));
  G_ASSERTF(false, "%s(...) not implemented", __FUNCTION__);
}
void OodleLoadCB::seekrel(int) { G_ASSERTF(false, "%s(...) not implemented", __FUNCTION__); }
bool OodleLoadCB::ceaseReading() { return true; }
#include "oodleIoFatal.cpp"

#define EXPORT_PULL dll_pull_iosys_oodleIo
#include <supp/exportPull.h>

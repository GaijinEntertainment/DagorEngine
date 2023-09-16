#include "geomWater.h"
#include <libTools/staticGeom/geomObject.h>
#include <render/waterObjects.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_drv3d.h>


static void on_remove(GeomObject *p) { waterobjects::del(p); }
extern void on_water_add(GeomObject *p, const ShaderMesh *m)
{
  uint16_t *inds;
  // debug("si =%d sv= %d numf=%d", m->getAllElems()[0].si, m->getAllElems()[0].sv, m->getAllElems()[0].numf);
  m->getAllElems()[0].vertexData->getIB()->lock(m->getAllElems()[0].si * sizeof(uint16_t),
    m->getAllElems()[0].numf * 3 * sizeof(uint16_t), &inds, VBLOCK_READONLY);
  Point3 *data;
  m->getAllElems()[0].vertexData->getVB()->lock(0, 0, (void **)&data, VBLOCK_READONLY);

  Plane3 avplane(0, 0, 0, 0);
  for (int fi = 0; fi < m->getAllElems()[0].numf; ++fi)
  {
    Point3 v[3];
    v[0] = data[inds[fi * 3 + 0]];
    v[1] = data[inds[fi * 3 + 1]];
    v[2] = data[inds[fi * 3 + 2]];
    Point3 n = normalize((v[1] - v[0]) % (v[2] - v[0]));
    Plane3 plane;
    if (n.y < 0)
      n = -n;
    // n = Point3(0,1,0);
    plane.set(n, v[0]);
    avplane.n += plane.n;
    avplane.d += plane.d;
  }
  avplane.n = normalize(avplane.n);
  avplane.d /= m->getAllElems()[0].numf;
  m->getAllElems()[0].vertexData->getVB()->unlock();
  m->getAllElems()[0].vertexData->getIB()->unlock();

  waterobjects::add(p, m->getAllElems()[0].mat, p->getBoundBox(false), Plane3(0, 1, 0, 0));
}

void GeomObject::initWater()
{
  stat_geom_on_remove = on_remove;
  stat_geom_on_water_add = on_water_add;
}

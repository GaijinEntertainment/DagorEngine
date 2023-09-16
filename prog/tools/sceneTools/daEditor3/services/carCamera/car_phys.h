#pragma once
#include <phys/dag_physResource.h>


class DynamicPhysObjectData;
class Point3;

namespace carphyssimulator
{
enum
{
  PHYS_DEFAULT = 1,
  PHYS_BULLET,
  PHYS_TYPE_LAST
};


extern real springFactor;
extern real damperFactor;


void simulate(float dt);

int getDefaultPhys();

bool begin(DynamicPhysObjectData *base_obj, int phys_type = PHYS_DEFAULT);
void setTargetObj(void *phys_body, const char *res);
void end();
void *getPhysWorld();

void beforeRender();
void renderTrans(bool render_collision, bool render_geom, bool draw_cmass);
void render();

void init();
void close();

bool connectSpring(const Point3 &pt, const Point3 &dir);
bool disconnectSpring();
void setSpringCoord(const Point3 &pt, const Point3 &dir);

bool shortOnObject(const Point3 &pt, const Point3 &dir);

bool buildAndSetCollisions();
}; // namespace carphyssimulator

// #include <de3_grassSrv.h>
#include <de3_gpuGrassService.h>
#include <render/gpuGrass.h>

class GPUGrass;

struct RandomGPUGrassRenderHelper : IRandomGrassRenderHelper
{
  BBox3 box;
  IRenderingService *hmap = nullptr;

  virtual bool beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &tm) override;
  virtual void endRender() override;
  virtual void renderColor() override;
  virtual void renderMask() override;
  bool isValid() const;
};

class GPUGrassService : public IGPUGrassService
{
  eastl::unique_ptr<GPUGrass> grass;
  bool enabled = false;
  RandomGPUGrassRenderHelper grassHelper;
  eastl::vector<GPUGrassType> grassTypes;
  eastl::vector<GPUGrassDecal> grassDecals;
  void enumerateGrassTypes(DataBlock &grassBlk);
  void enumerateGrassDecals(DataBlock &grassBlk);
  int findFreeDecalId() const;

public:
  bool srvEnabled;
  virtual void createGrass(DataBlock &grassBlk) override;
  virtual void closeGrass() override;
  virtual DataBlock *createDefaultGrass() override;
  virtual void enableGrass(bool flag) override;
  virtual void beforeRender(Stage stage) override;
  virtual void renderGeometry(Stage stage) override;
  virtual BBox3 *getGrassBbox() override;

  virtual bool isGrassEnabled() const override;
  virtual int getTypeCount() const override;
  virtual GPUGrassType &getType(int index) override;
  virtual GPUGrassType &addType(const char *name) override;
  virtual void removeType(const char *name) override;
  virtual GPUGrassType *getTypeByName(const char *name) override;

  virtual int getDecalCount() const override;
  virtual GPUGrassDecal &getDecal(int index) override;
  virtual GPUGrassDecal &addDecal(const char *name) override;
  virtual void removeDecal(const char *name) override;
  virtual GPUGrassDecal *getDecalByName(const char *name) override;
  virtual bool findDecalId(int id) const override;

  virtual void invalidate() override;
};
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/dagFileRW/sceneImpIface.h>
#include <libTools/util/strUtil.h>

bool get_dag_textures(const char *path, Tab<String> &list)
{
  class DagTexLoader : public ImpSceneCB
  {
  public:
    DagTexLoader(Tab<String> &list) : texList(list), valid(true) {}

    inline bool isValid() const { return valid; }

    void nomem_error(const char *fn) override { valid = false; }
    void open_error(const char *fn) override { valid = false; }
    void format_error(const char *fn) override { valid = false; }
    void read_error(const char *fn) override { valid = false; }

    int load_textures(int num) override { return -1; /*cease reading after textures field*/ }
    int load_material(const char *name) override { return 0; }
    int load_node() override { return 0; }
    int load_node_children() override { return 0; }
    int load_node_data() override { return 0; }
    int load_node_tm() override { return 0; }
    int load_node_mat(int numsubs) override { return 0; }
    int load_node_script() override { return 0; }
    int load_node_obj() override { return 0; }
    int load_node_bones(int num) override { return 0; }
    int load_node_bonesinf(int numv) override { return 0; }
    bool load_node_morph(int num_targets) override { return 0; }

    int load_texture(int, const char *texfn) override
    {
      String tex(texfn);
      ::simplify_fname(tex);
      texList.push_back(tex);
      return 1;
    }
    int load_material(ImpMat &) override { return 0; }
    int load_node_data(char *name, uint16_t id, int numchild, int flg) override { return 0; }
    int load_node_tm(TMatrix &) override { return 0; }
    int load_node_submat(int, uint16_t id) override { return 0; }
    int load_node_script(const char *) override { return 0; }
    int load_node_mesh(Mesh *) override { return 0; }
    int load_node_light(D3dLight &) override { return 0; }
    int load_node_splines(SplineShape *) override { return 0; }
    int load_node_bone(int, uint16_t nodeid, TMatrix &) override { return 0; }
    int load_node_bonesinf(real *inf) override { return 0; }
    bool load_node_morph_target(int index, uint16_t nodeid, const char *name) override { return 0; }

  private:
    bool valid;
    Tab<String> &texList;
  };


  DagTexLoader loader(list);

  ::load_scene(path, loader);

  if (!loader.isValid())
    return false;

  return true;
}

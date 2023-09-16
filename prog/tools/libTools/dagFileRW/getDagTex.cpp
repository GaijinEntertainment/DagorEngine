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

    virtual void nomem_error(const char *fn) { valid = false; }
    virtual void open_error(const char *fn) { valid = false; }
    virtual void format_error(const char *fn) { valid = false; }
    virtual void read_error(const char *fn) { valid = false; }

    virtual int load_textures(int num) { return -1; /*cease reading after textures field*/ }
    virtual int load_material(const char *name) { return 0; }
    virtual int load_node() { return 0; }
    virtual int load_node_children() { return 0; }
    virtual int load_node_data() { return 0; }
    virtual int load_node_tm() { return 0; }
    virtual int load_node_mat(int numsubs) { return 0; }
    virtual int load_node_script() { return 0; }
    virtual int load_node_obj() { return 0; }
    virtual int load_node_bones(int num) { return 0; }
    virtual int load_node_bonesinf(int numv) { return 0; }
    virtual bool load_node_morph(int num_targets) { return 0; }

    virtual int load_texture(int, const char *texfn)
    {
      String tex(texfn);
      ::simplify_fname(tex);
      texList.push_back(tex);
      return 1;
    }
    virtual int load_material(ImpMat &) { return 0; }
    virtual int load_node_data(char *name, uint16_t id, int numchild, int flg) { return 0; }
    virtual int load_node_tm(TMatrix &) { return 0; }
    virtual int load_node_submat(int, uint16_t id) { return 0; }
    virtual int load_node_script(const char *) { return 0; }
    virtual int load_node_mesh(Mesh *) { return 0; }
    virtual int load_node_light(D3dLight &) { return 0; }
    virtual int load_node_splines(SplineShape *) { return 0; }
    virtual int load_node_bone(int, uint16_t nodeid, TMatrix &) { return 0; }
    virtual int load_node_bonesinf(real *inf) { return 0; }
    virtual bool load_node_morph_target(int index, uint16_t nodeid, const char *name) { return 0; }

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

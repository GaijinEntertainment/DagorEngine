//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderVar.h>
#include <generic/dag_DObject.h>
#include <3d/dag_texMgr.h>
#include <memory/dag_mem.h>
#include <util/dag_globDef.h>

// forward declarations for external classes
class Sbuffer;
typedef Sbuffer Ibuffer;
typedef Sbuffer Vbuffer;
class IGenSave;
class IGenLoad;
class MaterialData;
class TextureIdSet;

class ScriptedShaderMaterial;
class ScriptedShaderElement;
struct ShaderMatData;

struct AdditionalInstanced;


// return variableId for shader variable name (with optional fatal when name not found)
extern bool dgs_all_shader_vars_optionals;
__forceinline int get_shader_variable_id(const char *name, bool is_optional = false)
{
  int varId = VariableMap::getVariableId(name);
#if DAGOR_DBGLEVEL > 0
  if (!is_optional && !VariableMap::isVariablePresent(varId) && !dgs_all_shader_vars_optionals)
    logerr("Shader variable '%s' not found.", name);
#endif
  G_UNUSED(is_optional);
  return varId;
};

// obsolete
__forceinline int get_shader_glob_var_id(const char *name, bool is_optional = false)
{
  return get_shader_variable_id(name, is_optional);
};

// returns size of shader channel for specified type
int shader_channel_type_size(int t);


class ShaderMaterial : public DObject
{
public:
  // DAG_DECLARE_NEW(tmpmem)
  DAG_DECLARE_NEW(midmem)

  decl_class_name(ShaderMaterial)

  inline ScriptedShaderMaterial &native() { return *(ScriptedShaderMaterial *)this; }
  inline const ScriptedShaderMaterial &native() const { return *(const ScriptedShaderMaterial *)this; }

  virtual ShaderMaterial *clone() const = 0;

  virtual int get_flags() const = 0;
  virtual void set_flags(int value, int mask) = 0;

  virtual bool set_int_param(const int variable_id, const int v) = 0;
  virtual bool set_real_param(const int variable_id, const real v) = 0;
  virtual bool set_color4_param(const int variable_id, const struct Color4 &) = 0;
  virtual bool set_texture_param(const int variable_id, const TEXTUREID v) = 0;
  virtual bool set_sampler_param(const int variable_id, d3d::SamplerHandle v) = 0;

  virtual bool hasVariable(const int variable_id) const = 0;
  virtual bool getColor4Variable(const int variable_id, Color4 &value) const = 0;
  virtual bool getRealVariable(const int variable_id, real &value) const = 0;
  virtual bool getIntVariable(const int variable_id, int &value) const = 0;
  virtual bool getTextureVariable(const int variable_id, TEXTUREID &value) const = 0;
  virtual bool getSamplerVariable(const int variable_id, d3d::SamplerHandle &value) const = 0;

  // returns false if shader is not renderable in specified mode (if pi==NULL - default rm)
  virtual bool enum_channels(ShaderChannelsEnumCB &, int &ret_code_flags) const = 0;
  virtual ShaderElement *make_elem(bool acquire_tex_refs, const char *info) = 0;
  inline ShaderElement *make_elem(const char *info = NULL) { return make_elem(true, info); }

  virtual bool isPositionPacked() const = 0;

  // return true, if channels are valid for this material & specified render mode (if pi==NULL - default rm)
  virtual bool checkChannels(CompiledShaderChannelId *ch, int ch_count) const = 0;

  virtual int get_num_textures() const = 0;
  virtual TEXTUREID get_texture(int) const = 0;
  virtual void set_texture(int, TEXTUREID) = 0;

  virtual void gatherUsedTex(TextureIdSet &tex_id_list) const = 0;

  virtual bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new) = 0;

  virtual void getMatData(ShaderMatData &out_data) const = 0;

  //! build material data from shader material (if orig_mat_script is passed then it is used to filter out default values)
  virtual void buildMaterialData(MaterialData &out_data, const char *orig_mat_script = nullptr) = 0;

  // get shader script name
  virtual const char *getShaderClassName() const = 0;

  // Disable some shaders for debug.
  virtual bool isSelectedForDebug() const = 0;

  // get material info for debug
  virtual const String getInfo() const = 0;

  // set sting for error info while loading material. don't forget to clear it after loading!
  static void setLoadingString(const char *s);

  // get string for error info. return NULL, if no error string set
  static const char *getLoadingString();

private:
  static const char *loadingStirngInfo;
};

class ShaderElement : public DObject
{
public:
  DAG_DECLARE_NEW(midmem)

  decl_class_name(ShaderElement)

  inline ScriptedShaderElement &native() { return *(ScriptedShaderElement *)this; }
  inline const ScriptedShaderElement &native() const { return *(const ScriptedShaderElement *)this; }

  virtual bool setStates() const = 0;
  inline bool setStates(int, bool) { return setStates(); }
  virtual bool setStatesDispatch() const = 0;
  virtual void render(int minvert, int numvert, int sind, int numf, int base_vertex = 0, int prim = PRIM_TRILIST) const = 0;

  virtual int getTextureCount() const = 0;
  virtual TEXTUREID getTexture(int index) const = 0;
  virtual void gatherUsedTex(TextureIdSet &tex_id_list) const = 0;
  virtual bool replaceTexture(TEXTUREID tex_id_prev, TEXTUREID tex_id_new) = 0;
  virtual bool hasTexture(TEXTUREID tex_id) const = 0;

  virtual void acquireTexRefs() = 0;
  virtual void releaseTexRefs() = 0;

  // return element flags
  virtual const char *getShaderClassName() const = 0;
  virtual void setProgram(uint32_t variant) = 0;

  // Return vertex size on shader input.
  virtual unsigned int getVertexStride() const = 0;

  virtual dag::ConstSpan<ShaderChannelId> getChannels() const = 0;
  virtual void replaceVdecl(VDECL vDecl) = 0;
  virtual VDECL getEffectiveVDecl() const = 0;

  virtual int getSupportedBlock(int variant, int layer) const = 0;

  virtual bool setReqTexLevel(int req_level = 15) const = 0;

  virtual bool checkAndPrefetchMissingTextures() const = 0;
  //! invalidates internal cached state block (to force block re-apply and d3d program update)
  static void invalidate_cached_state_block();
};


ShaderMaterial *new_shader_material(const MaterialData &m, bool sec_dump_for_exp = false, bool do_log = true);
ShaderMaterial *new_shader_material_by_name_optional(const char *shader_name, const char *mat_script = NULL,
  bool sec_dump_for_exp = false);
ShaderMaterial *new_shader_material_by_name(const char *shader_name, const char *mat_script = NULL, bool sec_dump_for_exp = false);
bool shader_exists(const char *shader_name);
const char *get_shader_class_name_by_material_name(const char *mat_name);

d3d::shadermodel::Version getMaxFSHVersion();

// startups shaders (if shbindump_base!= NULL schedules shaders data loading)
void startup_shaders(const char *shbindump_base, d3d::shadermodel::Version shader_model_version = d3d::smAny);


// direct load/unload shaders binary dump (called from shaders startup proc)
bool load_shaders_bindump(const char *src_filename, d3d::shadermodel::Version shader_model_version, bool sec_dump_for_exp = false);
void unload_shaders_bindump(bool sec_dump_for_exp = false);

bool load_shaders_bindump_with_fence(const char *src_filename, d3d::shadermodel::Version shader_model_version);

// all job mangers whose jobs are going to use shaders bindump should call this function
void register_job_manager_requiring_shaders_bindump(int job_mgr_id);

// enable usage of stateblocks in shaders (default is platform-specific)
inline void enable_shaders_use_stateblock(bool) {}

// enable emulation of stateblocks in shaders (default is false)
inline void enable_shaders_emulate_stateblock(bool) {}

// discards shader stateblocks (useful after 3d reset or when internal texture data is changed)
void rebuild_shaders_stateblocks();

// discards shader stateblocks, if too many were destroyed
void defrag_shaders_stateblocks(bool force); //

// registers console processor for shader-related commands, such as 'reload_shader'
using ShaderReloadCb = eastl::fixed_function<sizeof(void *), void(bool)>;
void shaders_register_console(
  bool allow_reload = true, const ShaderReloadCb &after_reload_cb = [](bool) {});

// enable or disable shaders reloading depending on build configuration and settings
void shaders_set_reload_flags();

// Shaders global time:

float get_shader_global_time();

void set_shader_global_time(float);

// Adds specified elapsed time to global time variable.
void advance_shader_global_time(float dt);

// Returns global time phase (number in range [0,1)) for specified period and time offset.
real get_shader_global_time_phase(float period, float offset);

//************************************************************************
//* low-level dynamic shader rendering
//************************************************************************
namespace dynrender
{
// [9/28/2004] Tigrenok
// This is optional structure for rendering.
// You can use this structure (fill it & call render()) or
// set params to driver and render all data manually (shElem->render())
struct RElem
{
  Ptr<ShaderElement> shElem; // element to render
  Ibuffer *ib;               // index buffer
  Vbuffer *vb;               // vertex buffer
  VDECL vDecl;               // vdecl
  int stride;                // stride

  // this parameters required only for member function render()
  int minVert;
  int numVert;
  int startIndex;
  int numPrim;

  inline RElem() : shElem(NULL), ib(NULL), vb(NULL), vDecl(BAD_VDECL), stride(-1), minVert(0), numVert(0), startIndex(0), numPrim(0) {}

  // set ib, vb, vdecl & render this element
  // if ib field is NULL or indexed == false, use d3d::draw instead d3d::drawind
  void render(bool indexed = true, int base_vertex_index = 0) const;
  void setBuffers(bool indexed = true) const;
  void setStates(bool indexed = true) const;
};

// add vdecl or get existing for specified channel set. if stride < 0, calculate it automatically
VDECL addShaderVdecl(const CompiledShaderChannelId *ch, int numch, int stride = -1, dag::Span<VSDTYPE> add_vsd = {});

// get stride for channel set. return -1, if failed
int getStride(const CompiledShaderChannelId *ch, int numch);

void convert_channels_to_vsd(const CompiledShaderChannelId *ch, int numch, Tab<VSDTYPE> &out_vsd);
} // namespace dynrender

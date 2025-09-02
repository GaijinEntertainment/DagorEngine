// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_unicode.h>

#include <ioSys/dag_genIo.h>
#include <ioSys/dag_fileIo.h>

#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>

#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <debug/dag_except.h>

#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_threadPool.h>

#include <libTools/util/strUtil.h>

#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/dagFileRW/dagMatRemapUtil.h>

#include <shaders/dag_shaders.h>
#include <shaders/dag_shMaterialUtils.h>
#include <shadersBinaryData.h>

#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>
#include <EASTL/hash_set.h>
#include <EASTL/hash_map.h>
#include <EASTL/deque.h>

#include <dag/dag_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <signal.h>
#include <stdio.h>
#include <filesystem>
#include <chrono>

//======================================================================
// options & logging
//======================================================================

#define DEBUG_LOG(fmt, ...)

//#define DEBUG_LOG(fmt, ...)  \
//  {                          \
//    debug(fmt, __VA_ARGS__); \
//  }

//#define DEBUG_LOG(fmt, ...)        \
//  {                                \
//    printf(fmt "\n", __VA_ARGS__); \
//  }

static bool is_quiet = false;

#define PRINT(fmt, ...)              \
  {                                  \
    if (!is_quiet)                   \
      printf(fmt "\n", __VA_ARGS__); \
  }

#define PRINT_TIME_MS(begin, end)                                                                              \
  {                                                                                                            \
    if (!is_quiet)                                                                                             \
      printf("Time: %lld [ms]\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()); \
  }

static bool is_exact = false;
static const char *exact_shdump = nullptr;
static const char *specific_shader = nullptr;
static bool class_name = false;
static const char *class_builtin = "__builtin__";
static bool force_defaults = false;
static eastl::string output("output.csv");
static bool unique_only = false;
static eastl::string output_unique("");
static unsigned int workers = 0;

//======================================================================
// assets & proxymats
//======================================================================

typedef dag::Vector<eastl::string> Names;                                   // e.g.: file-names
typedef ska::flat_hash_map<eastl::string, String> MatData;                  // mat key & value
typedef dag::Vector<MatData> MatUsages;                                     // multiple values-sets for a mat
typedef ska::flat_hash_map<eastl::string, MatUsages> MatLibrary;            // mats and their usages
typedef ska::flat_hash_map<eastl::string, Names> MatUniqueUsages;           // unique value-sets & assets using it
typedef ska::flat_hash_map<eastl::string, MatUniqueUsages> MatUsageLibrary; // mats and all their unique usages
typedef ska::flat_hash_map<eastl::string, MatData> DefShaderVarsLib;        // default values of all processed shaders

#define EXT_PROXYMAT ".proxymat.blk"
static int proxymat_ext_len = strlen(EXT_PROXYMAT);

#define N_NAME         "name"
#define N_CLASS        "class"
#define N_TWOSIDED     "twosided"
#define N_AMB          "amb"
#define N_DIFF         "diff"
#define N_SPEC         "spec"
#define N_EMIS         "emis"
#define N_POWER        "power"
#define N_TEX16SUPPORT "tex16support"

static void find_asset_files(eastl::vector<eastl::string> &filenames, ska::flat_hash_map<eastl::string, String> &proxymats,
  eastl::vector<std::filesystem::path> &subfolder_paths, const std::filesystem::path &folder_path)
{
  for (const auto &entry : std::filesystem::directory_iterator(folder_path))
  {
    const std::filesystem::path &path = entry.path();
    if (std::filesystem::is_directory(path))
    {
      const auto &dir = path.filename().string();
      if (dir.size() > 0 && dir[0] != '.' && dir != "cvs")
      {
        subfolder_paths.push_back(path);
      }
    }
    else
    {
      const std::filesystem::path &ext = path.extension();
      if (::dd_stricmp(ext.generic_string().c_str(), ".dag") == 0)
      {
        filenames.push_back(path.generic_string().c_str());
      }
      else
      {
        const auto &filename = path.filename().generic_string();
        const size_t f_size = filename.size();
        if (f_size >= proxymat_ext_len && ::dd_stricmp(filename.c_str() + f_size - proxymat_ext_len, EXT_PROXYMAT) == 0)
        {
          eastl::string key = filename.c_str();
          key.resize(f_size - proxymat_ext_len);
          proxymats.insert_or_assign(key, String(path.generic_string().c_str()));
        }
      }
    }
  }
}

static ska::flat_hash_map<eastl::string, DataBlock> proxymats_cache;

static void cache_proxymats(const ska::flat_hash_map<eastl::string, String> &proxymats)
{
  for (auto &proxymat : proxymats)
  {
    DataBlock &proxymat_block = proxymats_cache[proxymat.first];
    if (!proxymat_block.load(proxymat.second.c_str()))
    {
      proxymats_cache.erase(proxymat.first);
      PRINT("Failed to load proxymat from \"%s\"", proxymat.second.c_str());
    }
  }
}

static void process_name(MatData &material_data, const char *name)
{
  String real_name(name ? name : "");
  real_name.replaceAll("\n", "\\n");
  Tab<char> utf8;
  material_data.emplace(N_NAME, String(acp_to_utf8(real_name.c_str(), utf8)));
}

static void process_rgb(MatData &material_data, const char *name, int r, int g, int b, int def_r, int def_g, int def_b)
{
  if (force_defaults || r != def_r || g != def_g || b != def_b)
  {
    String col;
    col.printf(14, "%d, %d, %d", r, g, b);
    material_data.emplace(name, col);
  }
}

static void process_color(MatData &material_data, const char *name, const DagColor &c, int def_r, int def_g, int def_b)
{
  process_rgb(material_data, name, c.r, c.g, c.b, def_r, def_g, def_b);
}

static void process_proxymat_color(DataBlock &proxymat_block, MatData &material_data, const char *name, int def_r, int def_g,
  int def_b)
{
  int nameId = proxymat_block.getNameId(name);
  int idx = proxymat_block.findParam(nameId);
  if (idx >= 0)
  {
    IPoint3 c = proxymat_block.getIPoint3(idx);
    process_rgb(material_data, name, c.x, c.y, c.z, def_r, def_g, def_b);
  }
}

static void process_power(MatData &material_data, float power)
{
  if (force_defaults || power != 8.0f)
    material_data.emplace(N_POWER, String(0, "%g", power));
}

static void process_support_flag(MatData &material_data, const char *name, bool supported)
{
  if (force_defaults || supported)
    material_data.emplace(name, String(supported ? "yes" : "no"));
}

static void process_script(const eastl::string &filename, const String &shader_name, MatData &material_data,
  const eastl::string &script)
{
  eastl::string variable = script;
  if (variable.find("script:t") != eastl::string::npos)
  {
    PRINT("Script contains faulty (blk) data?! \"%s\":%s = \'%s\'", filename.c_str(), shader_name.c_str(), script.c_str());

    eastl_size_t start = script.find('"');
    eastl_size_t end = script.rfind('"');
    if (start != eastl::string::npos && end != eastl::string::npos)
    {
      variable.erase(end, eastl::string::npos);
      variable.erase(0, start + 1);
    }
  }

  eastl_size_t at = variable.find('=');
  if (at != eastl::string::npos)
  {
    eastl::string scrname = variable.substr(0, at);
    eastl_size_t var_at = at + 1;
    material_data.emplace(scrname, String(variable.substr(var_at, variable.length() - var_at).c_str()));
    DEBUG_LOG("  script '%s' = %s (%s)", scrname.c_str(), material_data[scrname].c_str(), variable.c_str());
  }
}

static bool process_asset(const eastl::string &filename, const ska::flat_hash_map<eastl::string, String> &proxymats,
  MatLibrary &local_library, const char *specific_shader)
{
  DEBUG_LOG("process_asset: \"%s\"", filename.c_str());

  DagData data;

  DAGOR_TRY
  {
    if (!load_scene(filename.c_str(), data))
    {
      DEBUG_LOG("Unable to find materials in asset \"%s\"\n", filename.c_str());
      return false;
    }
  }
  DAGOR_CATCH(const IGenLoad::LoadException &e)
  {
    PRINT("Failed to load file \"%s\"\n", filename.c_str());
    return false;
  }

  for (const auto &material : data.matlist)
  {
    MatData material_data;
    String shader_name(material.classname.c_str());
    DEBUG_LOG("shader_name = %s", shader_name.c_str());
    eastl::string key(shader_name.c_str());
    if (shader_name.suffix(":proxymat"))
    {
      DEBUG_LOG(":proxymat");
      eastl_size_t at = key.find(':');
      if (at != eastl::string::npos)
        key = key.substr(0, at);

      size_t key_size = key.size();
      if (proxymats.find(key) == proxymats.end())
      {
        if (key_size >= 4)
        {
          eastl::string_view digits(key.data() + (key_size - 4), 3);
          if (digits == ".00")
          {
            key = key.substr(0, key_size - 4);
            key_size -= 4;
          }
        }
      }
      if (proxymats.find(key) == proxymats.end())
      {
        eastl_size_t at = key.rfind("\\");
        if (at != eastl::string::npos)
        {
          key_size = key_size - at;
          key = key.substr(at, key_size);
        }
      }
      if (proxymats.find(key) == proxymats.end())
        key.make_lower();
      auto it = proxymats.find(key);
      if (it == proxymats.end())
      {
        DEBUG_LOG("Could not find proxymat for: %s", key.c_str());
        continue;
      }

      if (specific_shader != nullptr && key != specific_shader)
      {
        DEBUG_LOG("Non-specific shader found: %s", key.c_str());
        continue;
      }

      auto proxymat_it = proxymats_cache.find(it->first);
      if (proxymat_it == proxymats_cache.end())
      {
        DEBUG_LOG("No cached proxymat found: %s", it->first.c_str());
        continue;
      }
      DataBlock &proxymat_block = proxymat_it->second;

      shader_name = proxymat_block.getStr(N_CLASS, class_builtin);
      DEBUG_LOG("  class shader_name = %s", shader_name.c_str());

      const char *name = nullptr;
      name = proxymat_block.getStr(N_NAME, name);
      if (force_defaults || name)
        process_name(material_data, name);

      int scriptId = proxymat_block.getNameId("script");
      int sci = proxymat_block.findParam(scriptId);
      eastl::string script;
      while (sci >= 0)
      {
        script = proxymat_block.getStr(sci);
        process_script(filename, shader_name, material_data, script);
        sci = proxymat_block.findParam(scriptId, sci);
      }

      process_support_flag(material_data, N_TWOSIDED, proxymat_block.getBool(N_TWOSIDED, false));

      process_proxymat_color(proxymat_block, material_data, N_AMB, 255, 255, 255);
      process_proxymat_color(proxymat_block, material_data, N_DIFF, 255, 255, 255);
      process_proxymat_color(proxymat_block, material_data, N_SPEC, 255, 255, 255);
      process_proxymat_color(proxymat_block, material_data, N_EMIS, 0, 0, 0);

      process_power(material_data, proxymat_block.getReal(N_POWER, 8.0f));

      const bool tex_16_support = proxymat_block.getBool(N_TEX16SUPPORT, false);
      process_support_flag(material_data, N_TEX16SUPPORT, tex_16_support);
      for (int ti = 0, te = tex_16_support ? DAGTEXNUM : 8; ti < te; ++ti)
      {
        String texname(0, "tex%d", ti);
        const char *tex = proxymat_block.getStr(texname, nullptr);
        if (tex)
        {
          material_data.emplace(texname.c_str(), String(::dd_get_fname(tex)));
          DEBUG_LOG("  tex%d = '%s' (%s)", ti, texname.c_str(), ::dd_get_fname(tex));
        }
      }
    }

    if (specific_shader != nullptr && key != specific_shader)
    {
      DEBUG_LOG("Non-specific shader found: %s", key.c_str());
      continue;
    }

    if (shader_name == "")
      shader_name = String(class_builtin);

    process_name(material_data, material.name.c_str());

    DEBUG_LOG("  full script '%s'", material.script.c_str());

    String script;
    bool scriptNonASCII = false;
    for (int si = 0; si <= material.script.length(); ++si)
    {
      if (static_cast<unsigned char>(material.script[si]) > 127)
      {
        scriptNonASCII = true;
        PRINT("Script contains non-ASCII characters! \"%s\":%s = \'%s\'", filename.c_str(), shader_name.c_str(), script.c_str());
      }

      if (material.script[si] == '\n' || material.script[si] == '\0')
      {
        if (script.length())
        {
          script.push_back(0);
          char *raw_script = script.c_str();
          Tab<char> utf8;
          if (scriptNonASCII)
            raw_script = acp_to_utf8(raw_script, utf8);
          process_script(filename, shader_name, material_data, raw_script);
          script.clear();
          scriptNonASCII = false;
        }
      }
      else if (material.script[si] != '\r')
        script.push_back(material.script[si]);
    }

    const DagMater &mater = material.mater;
    process_support_flag(material_data, N_TWOSIDED, (mater.flags & DAG_MF_2SIDED));

    process_color(material_data, N_AMB, mater.amb, 255, 255, 255);
    process_color(material_data, N_DIFF, mater.diff, 255, 255, 255);
    process_color(material_data, N_SPEC, mater.spec, 255, 255, 255);
    process_color(material_data, N_EMIS, mater.emis, 0, 0, 0);

    process_power(material_data, mater.power);

    const bool tex_16_support = mater.flags & DAG_MF_16TEX;
    process_support_flag(material_data, N_TEX16SUPPORT, tex_16_support);
    for (int ti = 0; ti < DAGTEXNUM; ++ti)
    {
      String texname(0, "tex%d", ti);
      if (mater.texid[ti] != DAGBADMATID && (ti < 8 || tex_16_support))
      {
        const char *tex = ::dd_get_fname(data.texlist[mater.texid[ti]].c_str());
        if (tex)
        {
          material_data.emplace(texname.c_str(), String(tex));
          DEBUG_LOG("tex%d = '%s' (%s)", ti, tex, data.texlist[mater.texid[ti]].c_str());
        }
      }
    }

    Tab<char> utf8;
    eastl::string libname = acp_to_utf8(shader_name.c_str(), utf8);
    if (local_library.find(libname) == local_library.end())
      local_library.emplace(libname, MatUsages());
    local_library[libname].push_back(material_data);
  }

  return true;
}

static void process_def_shader_vars(MatLibrary &local_library, DefShaderVarsLib &shader_vars_lib)
{
  DEBUG_LOG("process_def_shader_vars:");

  for (auto &materials : local_library)
  {
    const eastl::string &shader_name = materials.first;
    DEBUG_LOG("  shader_name: \"%s\"", shader_name.c_str());

    for (auto &material_data : materials.second)
    {
      MatData *default_data;
      auto it = shader_vars_lib.find(shader_name);
      if (it == shader_vars_lib.end())
      {
        DEBUG_LOG("    cache miss!");
        default_data = &shader_vars_lib[shader_name];
        // cache all the default values  of the shader, converted to their string repr
        dag::Vector<ShaderVarDesc> script_vars = get_shclass_script_vars_list(shader_name.c_str());
        for (int vi = 0; vi < script_vars.size(); ++vi)
        {
          ShaderVarDesc &var_desc = script_vars[vi];
          eastl::string name = var_desc.name.c_str();
          String value;
          switch (var_desc.type)
          {
            case SHVT_INT: value.printf(16, "%d", var_desc.value.i); break;
            case SHVT_REAL: value.printf(32, "%f", var_desc.value.r); break;
            case SHVT_COLOR4:
              E3DCOLOR col = e3dcolor(var_desc.value.c4());
              if (col.a == 255)
                value.printf(14, "%d, %d, %d", col.r, col.g, col.b);
              else
                value.printf(19, "%d, %d, %d, %d", col.r, col.g, col.b, col.a);
              break;
            default: DEBUG_LOG("    unhandled script var \"%s\" type = %d", var_desc.name, var_desc.type); break;
          }
          default_data->insert_or_assign(name, value);
        }
      }
      else
        default_data = &it->second;

      // add the default values to the material usage if it is not overwritten yet!
      for (auto it : *default_data)
        material_data.emplace(it.first, it.second);
    }
  }
}

//======================================================================
// multithreading
//======================================================================

#define DEBUG_BEGIN_JOB(fmt, ...)                             \
  const int64_t thread_id = get_current_thread_id();          \
  DEBUG_LOG(fmt " (on thread %lld)", __VA_ARGS__, thread_id); \
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

#define DEBUG_END_JOB(fmt)                                                                        \
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();                   \
  const int64_t took = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count(); \
  DEBUG_LOG(fmt " (on thread %lld): %lld [ns]", thread_id, took);

struct FindAssetsJob : cpujobs::IJob
{
  const std::filesystem::path folder_path;

  eastl::vector<eastl::string> local_filenames;
  ska::flat_hash_map<eastl::string, String> local_proxymats;

  eastl::vector<std::filesystem::path> subfolder_paths;

  FindAssetsJob(const std::filesystem::path &_f) : folder_path(_f) {}

  void doJob() override
  {
    DEBUG_BEGIN_JOB("Job searching for assets under \"%s\"", folder_path.c_str());
    find_asset_files(local_filenames, local_proxymats, subfolder_paths, folder_path);
    DEBUG_END_JOB("Job searching for assets finished");
  }

  void releaseJob() override {}

  const char *getJobName(bool &) const override { return "FindAssetsJob"; }
};

struct AnalyiseJob : cpujobs::IJob
{
  const eastl::string &filename;
  const ska::flat_hash_map<eastl::string, String> &proxymats;

  const char *specific_shader;

  MatLibrary local_lib;

  bool processed;

  AnalyiseJob(const eastl::string &_f, const ska::flat_hash_map<eastl::string, String> &_p, const char *_s) :
    filename(_f), proxymats(_p), specific_shader(_s)
  {}

  void doJob() override
  {
    DEBUG_BEGIN_JOB("Job analyising asset \"%s\"", filename.c_str());
    processed = process_asset(filename, proxymats, local_lib, specific_shader);
    DEBUG_END_JOB("Job analyising asset finished");
  }

  virtual void releaseJob() override {}

  const char *getJobName(bool &) const override { return "AnalyiseJob"; }
};

// Threadpool backend data
static eastl::deque<cpujobs::IJob *> jobs_in_flight;
static uint32_t queue_pos = 0;

static constexpr unsigned MAX_WORKERS_FOR_AFFINITY_MASK = 64;
static constexpr int THREADPOOL_QUEUE_SIZE = 1024;

static unsigned worker_cnt = 0;

static constexpr size_t WORKER_STACK_SIZE = 1 << 20;

bool is_multithreaded() { return worker_cnt > 1; }
bool is_in_worker() { return threadpool::get_current_worker_id() != -1; }

void init_jobs(unsigned num_workers = 0)
{
  G_ASSERT(is_main_thread());

  cpujobs::init(-1, false);

  const unsigned req_workers = num_workers ? num_workers : cpujobs::get_physical_core_count();
  const unsigned workers = min<unsigned>(req_workers, MAX_WORKERS_FOR_AFFINITY_MASK);

  worker_cnt = workers;

  if (!worker_cnt)
    worker_cnt = cpujobs::get_physical_core_count();
  else if (num_workers > MAX_WORKERS_FOR_AFFINITY_MASK)
    worker_cnt = MAX_WORKERS_FOR_AFFINITY_MASK;

  if (worker_cnt == 1)
  {
    DEBUG_LOG("started in job-less mode");
    return;
  }

  threadpool::init(worker_cnt, THREADPOOL_QUEUE_SIZE, WORKER_STACK_SIZE);
  DEBUG_LOG("started threadpool");
}

void deinit_jobs()
{
  G_ASSERT(!is_in_worker());

  if (!is_multithreaded())
  {
    if (cpujobs::is_inited())
      cpujobs::term(false);
    return;
  }

  threadpool::shutdown();
  cpujobs::term(false);
}

void await_all_jobs()
{
  G_ASSERT(!is_in_worker());

  if (jobs_in_flight.empty())
    return;

  threadpool::barrier_active_wait_for_job(jobs_in_flight.back(), threadpool::PRIO_HIGH, queue_pos);
  for (auto &j : jobs_in_flight)
  {
    threadpool::wait(j);
    j->releaseJob();
  }

  jobs_in_flight.clear();
}

void add_job(cpujobs::IJob *job)
{
  G_ASSERT(is_main_thread());

  if (!is_multithreaded())
  {
    job->doJob();
    job->releaseJob();
    return;
  }

  jobs_in_flight.push_back(job);
  threadpool::add(jobs_in_flight.back(), threadpool::PRIO_DEFAULT, queue_pos, threadpool::AddFlags::IgnoreNotDone);
  threadpool::wake_up_all();
}

//======================================================================
// csv
//======================================================================

// clang-format off
static eastl::vector<eastl::string> csv_keys = {
  N_TWOSIDED,
  N_AMB, N_DIFF, N_SPEC, N_EMIS,
  N_POWER,
  N_TEX16SUPPORT,
  "tex0", "tex1", "tex2", "tex3", "tex4", "tex5", "tex6", "tex7",
  "tex8", "tex9", "tex10", "tex11", "tex12", "tex13", "tex14", "tex15",
  "real_two_sided",
};
// clang-format on

static void csv_update_keys(const MatUsages &usages)
{
  for (auto &usage : usages)
  {
    for (auto &key_val : usage)
    {
      if (key_val.first == "name" && csv_keys[0] != "name")
        csv_keys.insert(csv_keys.begin(), "name");

      if (eastl::find(csv_keys.begin(), csv_keys.end(), key_val.first) == csv_keys.end())
        csv_keys.push_back(key_val.first);
    }
  }
}

static file_ptr_t csv_start_file(const char *fn, const char *shader)
{
  dd_mkpath(fn);
  file_ptr_t fp = df_open(fn, DF_CREATE | DF_WRITE);
  G_ASSERT(fp);
  df_cprintf(fp, "\xEF\xBB\xBF"); // UTF-8 BOM
  if (shader)
    df_cprintf(fp, "%s\n", strcmp(shader, "") == 0 ? "simple" : shader);
  else
    df_cprintf(fp, "Shader name;");
  for (auto &csv_key : csv_keys)
    df_cprintf(fp, "%s;", csv_key.c_str());
  df_cprintf(fp, "Asset path\n");
  return fp;
}

static void csv_write_value(file_ptr_t fp, const eastl::string &key, const MatData &values)
{
  auto it = values.find(key);
  if (it != values.end())
    df_cprintf(fp, "%s;", it->second.c_str());
  else
    df_cprintf(fp, ";");
}

static void csv_write_line(file_ptr_t fp, const char *shader, const MatData &values, const eastl::string &asset)
{
  G_ASSERT(fp);
  if (shader)
    df_cprintf(fp, "%s;", shader);
  for (auto &csv_key : csv_keys)
    csv_write_value(fp, csv_key, values);
  df_cprintf(fp, "%s\n", asset.c_str());
}

static void csv_write_mat(file_ptr_t fp, const char *specific_shader, const eastl::string &material, const MatUsages &usages,
  const eastl::string &asset)
{
  G_ASSERT(fp);
  const char *shader = (specific_shader == nullptr) ? material.c_str() : nullptr;
  for (auto &usage : usages)
    csv_write_line(fp, shader, usage, asset);
}

static eastl::string csv_to_line_str(const MatData &values)
{
  eastl::string line = "";
  for (auto &csv_key : csv_keys)
  {
    auto it = values.find(csv_key);
    if (it != values.end())
      line += it->second.c_str();
    line += ';';
  }
  return line;
}

static void csv_update_unqiue_usages(MatUsageLibrary &library, const eastl::string material, const MatUsages &usages,
  const eastl::string &asset)
{
  MatUniqueUsages &unique_usages = library[material];
  for (auto &usage : usages)
  {
    eastl::string unqiue = csv_to_line_str(usage);
    Names &names = unique_usages[unqiue];
    names.push_back(asset);
  }
}

static void csv_write_line_unique_usage(file_ptr_t fp, const char *shader, const eastl::string &values, const eastl::string &asset)
{
  G_ASSERT(fp);
  if (shader)
    df_cprintf(fp, "%s;", shader);
  df_cprintf(fp, "%s%s\n", values.c_str(), asset.c_str());
}

static void csv_write_mat_unique_usages(file_ptr_t fp, const char *specific_shader, const eastl::string material,
  const MatUniqueUsages &unique_usages)
{
  G_ASSERT(fp);
  const char *shader = (specific_shader == nullptr) ? material.c_str() : nullptr;
  for (auto &unique_usage : unique_usages)
    csv_write_line_unique_usage(fp, shader, unique_usage.first, unique_usage.second[0]);
}

//======================================================================
// main
//======================================================================

void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void init_dagor(const char *base_path);
static void shutdown_dagor();
static void show_usage();

static bool look_for_assets_path(const String &folder_path, String &assets_path)
{
  assets_path.setStrCat(folder_path, "develop/assets/");
  return dd_dir_exists(assets_path.c_str());
}

static void print_progress(float percentage);

int DagorWinMain(bool debugmode)
{
  signal(SIGINT, ctrl_break_handler);

  if (__argc < 2)
  {
    ::show_usage();
    return 1;
  }

  for (int i = 2; i < __argc; ++i)
  {
    if (::dd_stricmp("-quiet", __argv[i]) == 0)
      is_quiet = true;
    else if (::dd_stricmp("-exact", __argv[i]) == 0)
      is_exact = true;
    else if (::dd_strnicmp("-shdump:", __argv[i], 8) == 0)
      exact_shdump = (__argv[i] + 8);
    else if (::dd_stricmp("-class", __argv[i]) == 0)
      class_name = true;
    else if (::dd_strnicmp("-builtin:", __argv[i], 9) == 0)
      class_builtin = (__argv[i] + 9);
    else if (::dd_stricmp("-forcedef", __argv[i]) == 0)
      force_defaults = true;
    else if (::dd_strnicmp("-shader:", __argv[i], 8) == 0)
      specific_shader = (__argv[i] + 8);
    else if (::dd_strnicmp("-output:", __argv[i], 8) == 0)
      output = (__argv[i] + 8);
    else if (::dd_strnicmp("-unique:", __argv[i], 8) == 0)
      output_unique = (__argv[i] + 8);
    else if (::dd_stricmp("-unique", __argv[i]) == 0)
      unique_only = true;
    else if (::dd_strnicmp("-workers:", __argv[i], 9) == 0)
      workers = atoi(__argv[i] + 9);
    else if (i == 2) // optional positional argument
      specific_shader = __argv[i];
    else
    {
      ::show_usage();
      return 1;
    }
  }

  start_classic_debug_system(".debug", false);

  init_dagor(__argv[0]);

  init_jobs(workers);

  eastl::vector<eastl::string> filenames;
  filenames.reserve(9000); // it's probably over it, but oh well...
  ska::flat_hash_map<eastl::string, String> proxymats;
  proxymats.reserve(1000);
  String folder_path(__argv[1]);
  append_slash(folder_path);
  simplify_fname(folder_path);
  // making sure we have the absolute folder path
  std::filesystem::path abs_folder_path(folder_path.c_str());
  abs_folder_path = std::filesystem::absolute(abs_folder_path);
  folder_path.setStr(abs_folder_path.generic_string().c_str());
  String root_path = folder_path;
  if (!is_exact)
  {
    String assets_path;
    if (folder_path.suffix("/develop/"))
    {
      String base_path(folder_path, strlen(folder_path) - 8);
      if (look_for_assets_path(base_path, assets_path))
      {
        root_path = base_path;
        folder_path = assets_path;
      }
      else if (look_for_assets_path(folder_path, assets_path))
      {
        folder_path = assets_path;
      }
    }
    else if (!folder_path.suffix("/develop/assets/"))
    {
      if (look_for_assets_path(folder_path, assets_path))
      {
        folder_path = assets_path;
      }
    }
    else
    {
      root_path = String(folder_path, strlen(folder_path) - 15);
    }
  }

  std::filesystem::path assets_path(folder_path.c_str());
  if (!std::filesystem::is_directory(assets_path))
  {
    PRINT("Could not find assets directory: %s", folder_path.c_str());
    ::show_usage();
    return 1;
  }

  String shdump;
  if (exact_shdump)
    shdump = exact_shdump;
  else
    shdump.setStrCat(root_path, "tools/common/compiledShaders/tools.ps50.shdump.bin");
  FullFileLoadCB crd(shdump.c_str());
  if (!crd.fileHandle)
  {
    if (exact_shdump)
    {
      PRINT("Could not find shdump file at: %s", exact_shdump);
    }
    else
    {
      PRINT("Could not detect tools shdump under: %s", root_path.c_str());
    }
  }
  else if (!shBinDumpOwner().load(crd, df_length(crd.fileHandle)))
  {
    PRINT("Failed to load shdump: %s", shdump.c_str());
  }

  DataBlock::setRootIncludeResolver(root_path.c_str());
  dd_add_base_path(root_path.c_str(), true);
  dd_add_base_path(folder_path.c_str());
  PRINT("Searching for dag files and proxymats under: \"%s\"", folder_path.c_str());
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  dag::Vector<FindAssetsJob> files_jobs{};
  files_jobs.reserve(1000);
  files_jobs.emplace_back(std::filesystem::path(folder_path.c_str()));
  int folderIdx = 0;
  while (folderIdx < files_jobs.size())
  {
    const int nextFolderIdx = files_jobs.size();
    for (int i = folderIdx; i < nextFolderIdx; ++i)
      add_job(&files_jobs[i]);

    await_all_jobs();

    for (int i = folderIdx; i < nextFolderIdx; ++i)
    {
      const auto &job = files_jobs[i];
      filenames.insert(filenames.end(), job.local_filenames.begin(), job.local_filenames.end());
      for (const auto &proxymat : job.local_proxymats)
        proxymats.insert_or_assign(proxymat.first, proxymat.second);

      for (const auto &folder_path : job.subfolder_paths)
        files_jobs.emplace_back(folder_path);
    }
    folderIdx = nextFolderIdx;
  }
  files_jobs.clear();

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  PRINT("Found %u dag files", filenames.size());
  PRINT("Found %u proxymats", proxymats.size());
  PRINT_TIME_MS(begin, end);

  PRINT("Caching proxymats");
  begin = std::chrono::steady_clock::now();
  cache_proxymats(proxymats);
  end = std::chrono::steady_clock::now();
  PRINT("Cached %zu proxymats", proxymats_cache.size());
  PRINT_TIME_MS(begin, end);

  begin = std::chrono::steady_clock::now();

  PRINT("Processing assets");

  dag::Vector<AnalyiseJob> jobs{};
  jobs.reserve(filenames.size());
  const float jobs_size = filenames.size();
  float done = 0;
  for (auto &filename : filenames)
  {
    jobs.emplace_back(filename, proxymats, specific_shader);
    add_job(&jobs.back());

    done++;
    if (!is_quiet)
      print_progress(done / jobs_size);
  }

  await_all_jobs();

  if (!is_quiet)
  {
    print_progress(1.0);
    printf("\n");
  }

  deinit_jobs();

  int processed_assets = 0;
  DefShaderVarsLib shader_vars_lib;
  for (auto &job : jobs)
  {
    if (job.processed)
    {
      process_def_shader_vars(job.local_lib, shader_vars_lib);
      processed_assets++;
    }
  }

  end = std::chrono::steady_clock::now();

  PRINT("Processed %d assets", processed_assets);
  PRINT_TIME_MS(begin, end);

  begin = std::chrono::steady_clock::now();

  if (class_name)
    csv_keys.insert(csv_keys.begin(), "name");
  for (auto &job : jobs)
  {
    if (!job.processed)
      continue;

    for (auto &materials : job.local_lib)
      csv_update_keys(materials.second);
  }

  dd_remove_base_path(root_path.c_str());
  dd_remove_base_path(folder_path.c_str());

  if (!unique_only)
  {
    PRINT("Preparing output: \"%s\"", output.c_str());

    file_ptr_t csv_fp = csv_start_file(output.c_str(), specific_shader);

    for (auto &job : jobs)
    {
      if (!job.processed)
        continue;

      for (auto &materials : job.local_lib)
        csv_write_mat(csv_fp, specific_shader, materials.first, materials.second, job.filename);
    }

    df_flush(csv_fp);
    df_close(csv_fp);
  }

  if (unique_only || output_unique != "")
  {
    const eastl::string &csv_filename = unique_only ? output : output_unique;
    PRINT("Preparing unique set output: \"%s\"", csv_filename.c_str());

    file_ptr_t csv_fp = csv_start_file(csv_filename.c_str(), specific_shader);

    MatUsageLibrary usage_library;
    for (auto &job : jobs)
    {
      if (!job.processed)
        continue;

      for (auto &materials : job.local_lib)
        csv_update_unqiue_usages(usage_library, materials.first, materials.second, job.filename);
    }

    for (auto &unique_usages : usage_library)
      csv_write_mat_unique_usages(csv_fp, specific_shader, unique_usages.first, unique_usages.second);

    df_flush(csv_fp);
    df_close(csv_fp);
  }

  end = std::chrono::steady_clock::now();

  PRINT_TIME_MS(begin, end);

  shutdown_dagor();

  return 0;
}

static void init_dagor(const char *base_path)
{
  ::dd_init_local_conv();
  ::dd_add_base_path("");
}

static void shutdown_dagor() {}

static void show_usage()
{
  printf("DAG asset material analyser tool\n"
         "Copyright (c) Gaijin Games KFT, 2025\n"
         "usage: mat_analyser <path to assets> [<specific shader>] [<options>]\n"
         "where\n"
         "<path to assets>   path to scan for DAG files\n"
         "<specific shader>  optionally limit analysis to one specific shader\n"
         "<options> are:\n"
         "  -exact           don't append \"develop/assets/\" to <path to assets>\n"
         "  -shdump:<path>   the \".shdump.bin\" file to use for default params\n"
         "  -class           output separate \"class\" and \"name\" shader data\n"
         "  -builtin:<cname> change built-in shader class name from \"__builtin__\"\n"
         "  -forcedef        force writing of default params (power, amb, etc...)\n"
         "  -shader:<sname>  optionally limit analysis to one specific shader\n"
         "  -output:<fname>  set output file (default is \"output.csv\")\n"
         "  -unique          output only unique shader usages for materials\n"
         "  -unique:<fname>  output unique shader usages to target file\n"
         "  -workers:<num>   cpu worker num (defaults to 0 which is cpu core count)\n"
         "  -quiet           don't print anything during execution\n");
}

#define RROGRESS_STR   "||||||||||||||||||||||||||||||||||||||||"
#define RROGRESS_WIDTH 40

static int last_progress_val = -1;

static void print_progress(float percentage)
{
  int val = (int)(percentage * 100);
  if (val == last_progress_val)
    return;

  last_progress_val = val;
  int lpad = (int)(percentage * RROGRESS_WIDTH);
  int rpad = RROGRESS_WIDTH - lpad;
  printf("\r%3d%% [%.*s%*s]%s", val, lpad, RROGRESS_STR, rpad, "", (val < 100 ? "\r" : ""));
  fflush(stdout);
}

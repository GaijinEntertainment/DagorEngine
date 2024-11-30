// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_finally.h>
#include <compressionUtils/compression.h>

#include <fmod.hpp>
#include <fmod_studio.hpp>

#include <soundSystem/fmodApi.h>
#include <soundSystem/geometry.h>
#include <soundSystem/debug.h>

#include "internal/fmodCompatibility.h"
#include "internal/debug.h"


static WinCritSec g_geometry_cs;
#define SNDSYS_GEOMETRY_BLOCK WinAutoLock geometryLock(g_geometry_cs);

namespace sndsys
{
#if DAGOR_DBGLEVEL > 0
struct GeometryDebug
{
  int id;
  eastl::vector<Point3> debugFaces;
};
static eastl::vector<GeometryDebug> g_geometry_debug;

static GeometryDebug &add_geometry_debug(int geometry_id)
{
  GeometryDebug &geometryDebug = g_geometry_debug.push_back();
  geometryDebug.id = geometry_id;
  return geometryDebug;
}
static GeometryDebug *find_geometry_debug(int geometry_id)
{
  auto pred = [geometry_id](const GeometryDebug &it) { return it.id == geometry_id; };
  auto it = eastl::find_if(g_geometry_debug.begin(), g_geometry_debug.end(), pred);
  return it != g_geometry_debug.end() ? it : nullptr;
}
static void remove_geometry_debug(int geometry_id)
{
  if (GeometryDebug *it = find_geometry_debug(geometry_id))
    g_geometry_debug.erase(it);
}
static void remove_all_geometry_debug() { g_geometry_debug.clear(); }
#else
static void remove_geometry_debug(int) {}
static void remove_all_geometry_debug() {}
#endif

struct Geometry
{
  FMOD::Geometry *const fmodGeometry;
  const int id;
  Geometry() = delete;
};

static eastl::vector<Geometry> g_geometry;

static constexpr int g_first_valid_geometry_id = 0;
static constexpr int g_invalid_geometry_id = g_first_valid_geometry_id - 1;
static int g_next_valid_geometry_id = g_first_valid_geometry_id;

static Geometry *find_geometry(int geometry_id)
{
  auto pred = [geometry_id](const Geometry &it) { return it.id == geometry_id; };
  auto it = eastl::find_if(g_geometry.begin(), g_geometry.end(), pred);
  return it != g_geometry.end() ? it : nullptr;
}

static int add_geometry(FMOD::Geometry *fmod_geometry)
{
  const int id = g_next_valid_geometry_id;
  g_geometry.push_back(Geometry{fmod_geometry, id});
  ++g_next_valid_geometry_id;
  return id;
}

int add_geometry(int max_polygons, int max_vertices)
{
  SNDSYS_GEOMETRY_BLOCK;
  FMOD::System *system = fmodapi::get_system();
  if (!system)
    return g_invalid_geometry_id;

  FMOD::Geometry *fmodGeometry = nullptr;
  SOUND_VERIFY_AND_RETURN_(system->createGeometry(max_polygons, max_vertices, &fmodGeometry), g_invalid_geometry_id);
  G_ASSERT(fmodGeometry != nullptr);

  return add_geometry(fmodGeometry);
}

void remove_geometry(int geometry_id)
{
  SNDSYS_GEOMETRY_BLOCK;
  remove_geometry_debug(geometry_id);
  if (Geometry *it = find_geometry(geometry_id))
  {
    it->fmodGeometry->release();
    g_geometry.erase(it);
  }
}

void remove_all_geometry()
{
  SNDSYS_GEOMETRY_BLOCK;
  for (Geometry &it : g_geometry)
    it.fmodGeometry->release();
  g_geometry.clear();
  remove_all_geometry_debug();
}

void add_polygons(int geometry_id, dag::ConstSpan<Point3> vertices, int num_verts_per_poly, float direct_occlusion,
  float reverb_occlusion, bool doublesided)
{
  SNDSYS_GEOMETRY_BLOCK;
  Geometry *geometry = find_geometry(geometry_id);
  G_ASSERT_AND_DO(geometry != nullptr, return);

  const int numVertices = vertices.size();
  const int numPolygons = numVertices / num_verts_per_poly;

  G_ASSERT_AND_DO(num_verts_per_poly >= 3, return);
  G_ASSERT_AND_DO(numVertices >= num_verts_per_poly, return);
  G_ASSERT_AND_DO(!(numVertices % num_verts_per_poly), return);

  for (int idx = 0; idx < numPolygons; ++idx)
  {
    SOUND_VERIFY(geometry->fmodGeometry->addPolygon(direct_occlusion, reverb_occlusion, doublesided, num_verts_per_poly,
      &as_fmod_vector(vertices[idx * num_verts_per_poly]), nullptr));
  }
  remove_geometry_debug(geometry_id);
}

void add_polygon(int geometry_id, const Point3 &a, const Point3 &b, const Point3 &c, float direct_occlusion, float reverb_occlusion,
  bool doublesided)
{
  const Point3 verts[] = {a, b, c};
  add_polygons(geometry_id, make_span_const(verts, countof(verts)), countof(verts), direct_occlusion, reverb_occlusion, doublesided);
}

void set_geometry_position(int geometry_id, const Point3 &position)
{
  SNDSYS_GEOMETRY_BLOCK;
  const Geometry *geometry = find_geometry(geometry_id);
  G_ASSERT_AND_DO(geometry != nullptr, return);
  SOUND_VERIFY(geometry->fmodGeometry->setPosition(&as_fmod_vector(position)));
}

Point3 get_geometry_position(int geometry_id)
{
  SNDSYS_GEOMETRY_BLOCK;
  const Geometry *geometry = find_geometry(geometry_id);
  G_ASSERT_AND_DO(geometry != nullptr, return {});
  FMOD_VECTOR position;
  SOUND_VERIFY(geometry->fmodGeometry->getPosition(&position));
  return as_point3(position);
}

int get_geometry_count()
{
  SNDSYS_GEOMETRY_BLOCK;
  return g_geometry.size();
}

int get_geometry_id(int idx)
{
  SNDSYS_GEOMETRY_BLOCK;
  return g_geometry[idx].id;
}

//
static constexpr int g_geometry_version = 1000;
static constexpr const char *g_geometry_header_id = "FMGM";
static constexpr const char *g_geometry_compression = "snappy";

struct GeometryFileHeader
{
  char id[4];
  int version;
  int numGeometry;
};

struct GeometryHeader
{
  int plainDataSize;
  int compressedDataSize;
};

using Data = eastl::vector<uint8_t>;

static const Data *compress(const char *compression_name, const Data &data, Data &out_data, int &out_data_size)
{
  auto &instance = Compression::getInstanceByName(compression_name);
  out_data_size = instance.getRequiredCompressionBufferLength(data.size());
  out_data.resize(out_data_size);
  instance.compress(data.begin(), data.size(), out_data.begin(), out_data_size);
  return &out_data;
}

static const Data *decompress(const char *compression_name, const Data &data, Data &out_data, int &out_data_size)
{
  auto &instance = Compression::getInstanceByName(compression_name);
  out_data_size = instance.getRequiredDecompressionBufferLength(data.begin(), data.size());
  out_data.resize(out_data_size);
  instance.decompress(data.begin(), data.size(), out_data.begin(), out_data_size);
  return &out_data;
}

void save_geometry_to_file(const char *filename)
{
  SNDSYS_GEOMETRY_BLOCK;

  file_ptr_t fp = df_open(filename, DF_WRITE | DF_CREATE);
  G_ASSERT_RETURN(fp, );
  FINALLY([fp]() { df_close(fp); });

  GeometryFileHeader fileHeader = {};
  memcpy(fileHeader.id, g_geometry_header_id, sizeof(fileHeader.id));
  fileHeader.version = g_geometry_version;
  fileHeader.numGeometry = g_geometry.size();
  G_ASSERT_RETURN(df_write(fp, &fileHeader, sizeof(fileHeader)) == sizeof(fileHeader), );

  Data plainData;
  Data buffer;

  for (const Geometry &it : g_geometry)
  {
    int plainDataSize = 0;
    SOUND_VERIFY(it.fmodGeometry->save(nullptr, &plainDataSize));
    plainData.resize(plainDataSize);
    SOUND_VERIFY(it.fmodGeometry->save(plainData.begin(), &plainDataSize));

    int compressedDataSize = 0;
    auto compressedData = compress(g_geometry_compression, plainData, buffer, compressedDataSize);

    GeometryHeader header;
    header.plainDataSize = plainDataSize;
    header.compressedDataSize = compressedDataSize;

    G_ASSERT_RETURN(df_write(fp, &header, sizeof(header)) == sizeof(header), );

    G_ASSERT_RETURN(df_write(fp, compressedData->begin(), compressedDataSize) == compressedDataSize, );
  }

  debug_trace_info("saved geometry to '%s'", filename);
}

bool load_geometry_from_file(const char *filename)
{
  SNDSYS_GEOMETRY_BLOCK;

  auto readFileErr = [&]() {
    logerr("[SNDSYS] load_geometry_from_file '%s': read file error", filename);
    remove_all_geometry();
    return false;
  };

  remove_all_geometry();

  FMOD::System *system = fmodapi::get_system();
  if (!system)
  {
    logerr("[SNDSYS] load_geometry_from_file '%s': no fmod system", filename);
    return false;
  }

  file_ptr_t fp = df_open(filename, DF_READ);
  if (!fp)
  {
    logerr("[SNDSYS] load_geometry_from_file '%s': error open file", filename);
    return false;
  }
  FINALLY([fp]() { df_close(fp); });

  GeometryFileHeader fileHeader = {};
  if (df_read(fp, &fileHeader, sizeof(fileHeader)) != sizeof(fileHeader))
    return readFileErr();

  if (memcmp(fileHeader.id, g_geometry_header_id, sizeof(fileHeader.id)) != 0)
  {
    logerr("[SNDSYS] load_geometry_from_file '%s': bad file id", filename);
    return false;
  }

  if (fileHeader.version != g_geometry_version)
  {
    logerr("[SNDSYS] load_geometry_from_file '%s': bad file version: %d, expected: %d", filename, fileHeader.id, g_geometry_header_id);
    return false;
  }

  Data compressedData;
  Data buffer;

  for (int i = 0; i < fileHeader.numGeometry; ++i)
  {
    GeometryHeader header;
    if (df_read(fp, &header, sizeof(header)) != sizeof(header))
      return readFileErr();

    compressedData.resize(header.compressedDataSize);
    if (df_read(fp, compressedData.begin(), compressedData.size()) != compressedData.size())
      return readFileErr();

    int plainDataSize = 0;
    auto plainData = decompress(g_geometry_compression, compressedData, buffer, plainDataSize);
    if (plainDataSize != header.plainDataSize)
      return readFileErr();

    FMOD::Geometry *fmodGeometry = nullptr;
    SOUND_VERIFY(system->loadGeometry(plainData->begin(), plainDataSize, &fmodGeometry));

    if (fmodGeometry)
      add_geometry(fmodGeometry);
  }

  debug_trace_info("loaded geometry from '%s'", filename);

  return true;
}

// debug
const eastl::vector<Point3> *get_geometry_faces(int geometry_id)
{
  SNDSYS_GEOMETRY_BLOCK;
#if DAGOR_DBGLEVEL > 0
  const Geometry *geometry = find_geometry(geometry_id);
  if (!geometry)
    return nullptr;

  GeometryDebug *geometryDebug = find_geometry_debug(geometry_id);
  if (!geometryDebug)
  {
    geometryDebug = &add_geometry_debug(geometry_id);

    int numpolygons = 0;
    SOUND_VERIFY_AND_RETURN_(geometry->fmodGeometry->getNumPolygons(&numpolygons), nullptr);

    eastl::vector<Point3> faces;
    faces.reserve(numpolygons * 3);
    for (int pidx = 0; pidx < numpolygons; ++pidx)
    {
      SOUND_VERIFY_AND_RETURN_(geometry->fmodGeometry->getNumPolygons(&numpolygons), nullptr);
      int numvertices = 0;
      SOUND_VERIFY_AND_RETURN_(geometry->fmodGeometry->getPolygonNumVertices(pidx, &numvertices), nullptr);
      G_ASSERT_AND_DO(numvertices >= 3, return nullptr);

      FMOD_VECTOR v0, v1, v2;
      // index order is currently unknown, may need research
      SOUND_VERIFY_AND_RETURN_(geometry->fmodGeometry->getPolygonVertex(pidx, 0, &v0), nullptr);
      SOUND_VERIFY_AND_RETURN_(geometry->fmodGeometry->getPolygonVertex(pidx, 1, &v1), nullptr);

      for (int vidx = 2; vidx < numvertices; ++vidx)
      {
        SOUND_VERIFY_AND_RETURN_(geometry->fmodGeometry->getPolygonVertex(pidx, vidx, &v2), nullptr);
        faces.push_back(as_point3(v0));
        faces.push_back(as_point3(v1));
        faces.push_back(as_point3(v2));
      }
    }
    geometryDebug->debugFaces = eastl::move(faces);
  }
  return &geometryDebug->debugFaces;
#else
  G_UNREFERENCED(geometry_id);
  return nullptr;
#endif
}

// debug
Point2 get_geometry_occlusion(const Point3 &source, const Point3 &listener)
{
  Point2 directReverb = {};
  FMOD::System *system = fmodapi::get_system();
  if (system)
    SOUND_VERIFY(system->getGeometryOcclusion(&as_fmod_vector(source), &as_fmod_vector(listener), &directReverb.x, &directReverb.y));
  return directReverb;
}

} // namespace sndsys

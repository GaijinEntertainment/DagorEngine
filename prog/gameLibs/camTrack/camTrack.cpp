// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <camTrack/camTrack.h>
#include <ioSys/dag_asyncWrite.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <3d/dag_render.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_tab.h>
#include <generic/dag_tabUtils.h>
#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

struct CamRecord
{
  float time;
  TMatrix itm;
  float fov;
};

struct CameraTrack
{
  const char *filename = NULL; // points to hash_map's string key
  Tab<CamRecord> records;
  int cachedRecord = -1;

  void init(const char *fname)
  {
    G_ASSERTF(fname && *fname, "Incorrect fname %s", fname);
    cachedRecord = -1;

    file_ptr_t file = df_open(fname, DF_READ | DF_IGNORE_MISSING);
    G_ASSERT_RETURN(file, );
    CamRecord recBuffer;
    while (df_read(file, &recBuffer, sizeof(CamRecord)) > 0)
    {
      if (!records.empty())
        G_ASSERT_BREAK(records.back().time < recBuffer.time);
      records.push_back(recBuffer);
    }
    df_close(file);
  }

  Point2 getTimeRange() const
  {
    G_ASSERT_RETURN(!records.empty(), Point2(0.f, 0.f));
    return Point2(records[0].time, records.back().time);
  }

  float getDistToTime(float timestamp) const
  {
    Point2 timeRange = getTimeRange();
    return fabsf(timestamp - clamp(timestamp, timeRange.x, timeRange.y));
  }

  int bin_range_search(float time, int from, int to)
  {
    int a = from;
    int b = to;
    while (a <= b)
    {
      int c = (a + b) / 2;
      if (c == a)
        return a;
      int v = records[c].time > time ? -1 : +1;
      if (v < 0)
        b = c;
      else
        a = c;
    }
    return -1;
  }

  void calcCachedRecord(float time)
  {
    if (time <= records[0].time)
      cachedRecord = 0;
    else if (time >= records.back().time)
      cachedRecord = records.size() - 1;
    else
      cachedRecord = bin_range_search(time, 0, records.size() - 1);
  }

  void apply(float abs_time, TMatrix &out_itm, float &out_fov)
  {
    if (!tabutils::isCorrectIndex(records, cachedRecord) ||
        (cachedRecord + 1 < records.size() && abs_time > records[cachedRecord + 1].time))
      calcCachedRecord(abs_time);

    G_ASSERT_RETURN(tabutils::isCorrectIndex(records, cachedRecord), );

    out_itm = records[cachedRecord].itm;
    out_fov = records[cachedRecord].fov;
    if (cachedRecord + 1 < records.size() && abs_time >= records[cachedRecord].time)
    {
      // interpolate
      float k = safediv(abs_time - records[cachedRecord].time, records[cachedRecord + 1].time - records[cachedRecord].time);
      Quat from(records[cachedRecord].itm);
      Quat to(records[cachedRecord + 1].itm);
      out_itm = makeTM(qinterp(from, to, k));
      out_itm.setcol(3, lerp(records[cachedRecord].itm.getcol(3), records[cachedRecord + 1].itm.getcol(3), k));
      out_fov = lerp(out_fov, records[cachedRecord + 1].fov, k);
    }
  }
};
static eastl::unique_ptr<IGenSave> write_stream;
static eastl::hash_map<eastl::string, CameraTrack> tracks;
static float latest_written_timestamp = -1.f;

void camtrack::record(const char *filename)
{
  if (write_stream)
    stop_record();
  write_stream.reset(create_async_writer(filename, 64 << 10));
}

inline float fov_to_deg(float fov) { return RAD_TO_DEG * 2.f * atan(1.f / fov); }

void camtrack::update_record(float abs_time, const TMatrix &itm, const float hor_fov)
{
  if (!write_stream)
    return;
  if (latest_written_timestamp == abs_time)
    return;
  latest_written_timestamp = abs_time;
  CamRecord rec;
  rec.time = abs_time;
  rec.itm = itm;
  rec.fov = fov_to_deg(hor_fov);
  write_stream->write(&rec, sizeof(CamRecord));
}


void camtrack::stop_record() { write_stream.reset(); }

camtrack::handle_t camtrack::load_track(const char *filename)
{
  G_ASSERT_RETURN(dd_file_exists(filename), camtrack::INVALID_HANDLE);
  auto it = tracks.find_as(filename);
  if (it == tracks.end())
  {
    it = tracks.insert(eastl::string(filename)).first;
    it->second.filename = it->first.c_str();
    it->second.init(it->first.c_str());
  }
  return &it->second;
}

bool camtrack::get_track(handle_t handle, float abs_time, TMatrix &out_itm, float &out_fov)
{
  G_ASSERT_RETURN(handle, false);
  static_cast<CameraTrack *>(handle)->apply(abs_time, out_itm, out_fov);
  return true;
}

void camtrack::stop_play(handle_t handle)
{
  G_ASSERT_RETURN(handle, );
  auto it = tracks.find_as(static_cast<CameraTrack *>(handle)->filename);
  G_ASSERT_RETURN(it != tracks.end(), );
  tracks.erase(it);
}

void camtrack::term() { tracks.clear(); }

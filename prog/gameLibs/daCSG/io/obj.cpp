// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "io/readSeam.h"
#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/io_api.h>
#include <gameLibs/daCSG/mesh_api.h>
#include <math/dag_check_nan.h>
#include <math/dag_intrin.h>
#include <math/dag_Point2.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_oaHashNameMap.h>
#include <stdlib.h>
#include <string.h>

namespace dacsg
{
static bool write_text(const char *path, const eastl::string &text, eastl::string &err)
{
  file_ptr_t fp = df_open(path, DF_WRITE | DF_CREATE);
  bool ok = fp && df_write(fp, text.data(), text.size()) == int(text.size());
  if (fp)
    ok = df_close(fp) == 0 && ok;
  if (!ok)
    err.sprintf("cannot write %s", path);
  return ok;
}

bool write_obj(const Mesh &m, const char *path, eastl::string &err)
{
  const Channel *uv = m.findChannel(CH_UV), *nrm = m.findChannel(CH_NORMAL), *color = m.findChannel(CH_COLOR);
  if ((uv && uv->dim != 2) || (nrm && nrm->dim != 3) || (color && color->dim != 4))
  {
    err = "uv, normal or color channel has a wrong dim";
    return false;
  }
  const char *ext = dd_get_fname_ext(path);
  const eastl::string mtlPath = eastl::string(path, ext ? ext - path : strlen(path)) + ".mtl";

  eastl::string obj(eastl::string::CtorSprintf(), "mtllib %s\n", dd_get_fname(mtlPath.c_str()));
  for (const Point3 &p : m.pos)
    obj.append_sprintf("v %.9g %.9g %.9g\n", p.x, p.y, p.z);
  const uint32_t corners = m.tri.size();
  dag::Vector<uint32_t> vtIdx, vnIdx, matOf;
  dag::Vector<float> vt, vn, rgba;
  if (uv)
    index_tuples(uv->data.data(), corners, 2, 2, vtIdx, vt);
  if (nrm)
    index_tuples(nrm->data.data(), corners, 3, 3, vnIdx, vn);
  for (uint32_t i = 0; i < vt.size(); i += 2)
    obj.append_sprintf("vt %.9g %.9g\n", vt[i], vt[i + 1]);
  for (uint32_t i = 0; i < vn.size(); i += 3)
    obj.append_sprintf("vn %.9g %.9g %.9g\n", vn[i], vn[i + 1], vn[i + 2]);

  index_tuples(color ? color->data.data() : DEFAULT_COLOR, m.faceCount(), 4, color ? 12 : 0, matOf, rgba);
  eastl::string mtl;
  for (uint32_t i = 0; i < rgba.size(); i += 4)
    mtl.append_sprintf("newmtl mat%u\nKd %.9g %.9g %.9g\nd %.9g\n", i / 4, rgba[i], rgba[i + 1], rgba[i + 2], rgba[i + 3]);
  uint32_t lastMat = ~0u, lastGroup = 0;
  for (uint32_t f = 0; f < m.faceCount(); ++f)
  {
    const uint32_t mat = matOf[f];
    if (mat != lastMat)
      obj.append_sprintf("usemtl mat%u\n", lastMat = mat);
    const uint32_t group = m.face[f].smgr ? __ctz_unsafe(m.face[f].smgr) + 1 : 0;
    if (group != lastGroup)
      obj.append_sprintf("s %u\n", lastGroup = group);
    obj += "f";
    for (uint32_t c = f * 3; c < f * 3 + 3; ++c)
    {
      obj.append_sprintf(" %u", m.tri[c] + 1);
      if (uv)
        obj.append_sprintf("/%u", vtIdx[c] + 1);
      if (nrm)
        obj.append_sprintf(uv ? "/%u" : "//%u", vnIdx[c] + 1);
    }
    obj += "\n";
  }
  return write_text(mtlPath.c_str(), mtl, err) && write_text(path, obj, err);
}

// the next blank-separated token of a statement, [tok, tok + len); len 0 at the statement's end
static size_t token(const char *&p, const char *&tok)
{
  while (*p == ' ' || *p == '\t')
    ++p;
  tok = p;
  while (*p && *p != ' ' && *p != '\t')
    ++p;
  return p - tok;
}

static bool word(const char *w, size_t n, const char *t) { return strncmp(w, t, n) == 0 && !t[n]; }

// the whole token as a finite decimal float: strtof alone takes hex, inf, nan and a prefix of the token
static bool decimal(const char *s, size_t n, float &x)
{
  char *end;
  x = strtof(s, &end);
  return strspn(s, "0123456789.eE+-") == n && end == s + n && check_finite(x);
}

// the statements of the format a Mesh does not hold, skipped whole; any other word is refused
static const char *const IGNORED_STATEMENTS[] = {"vn", "vp", "cstype", "deg", "bmat", "step", "p", "l", "curv", "curv2", "surf",
  "parm", "trim", "hole", "scrv", "sp", "end", "con", "g", "mg", "o", "bevel", "c_interp", "d_interp", "lod", "mtllib", "maplib",
  "usemap", "call", "csh", "shadow_obj", "trace_obj", "ctech", "stech"};

// an index field of a corner token [s, e): a minus or a digit first (strtoll alone takes a space or a plus), then the
// token's end or a slash
static bool parse_index(const char *&s, const char *e, uint32_t count, uint32_t &out)
{
  char *end;
  if (s == e || (*s != '-' && (*s < '0' || *s > '9')))
    return false;
  const long long i = strtoll(s, &end, 10);
  if (i == 0 || i > (long long)count || i < -(long long)count || (end != e && *end != '/'))
    return false;
  out = i > 0 ? uint32_t(i - 1) : count - uint32_t(-i);
  s = end;
  return true;
}

bool read_obj(const char *path, Mesh &m, eastl::string &err) { return read_obj_through(path, m, err, df_read); }

bool read_obj_through(const char *path, Mesh &m, eastl::string &err, ObjReader reader)
{
  m = Mesh();
  file_ptr_t fp = df_open(path, DF_READ | DF_IGNORE_MISSING);
  if (!fp)
  {
    err.sprintf("cannot open %s", path);
    return false;
  }
  dag::Vector<Point2> vts;
  dag::Vector<float> uv;
  OAHashNameMap<false> mats; // ids in first-seen order
  uint32_t mat = ~0u, smgr = 0, normals = 0;
  dag::Vector<uint32_t> fv, ft;
  // a statement: the physical lines a trailing backslash joins, each cut at its comment first, blank-separated; its
  // first line names an error, the line count runs over the read blocks
  eastl::string stmt;
  uint32_t line = 0, first = 0;
  const char *why = "malformed line or bad index";
  const auto statement = [&] {
    const char *p = stmt.c_str(), *kw, *tok;
    const size_t n = token(p, kw);
    bool ok = true;
    // need values, any more numbers (v: w or a color), each one whole token
    const auto coords = [&](int need, int keep, float *c) {
      int k = 0;
      for (size_t len = token(p, tok); len && ok; len = token(p, tok))
      {
        float x;
        ok = decimal(tok, len, x);
        if (k < keep)
          c[k] = x;
        ++k;
      }
      ok = ok && k >= need;
    };
    if (word(kw, n, "v"))
    {
      float c[3] = {};
      coords(3, 3, c);
      m.pos.push_back(Point3(c[0], c[1], c[2]));
    }
    else if (word(kw, n, "vt"))
    {
      float c[2] = {};
      coords(1, 2, c);
      vts.push_back(Point2(c[0], c[1]));
    }
    else if (word(kw, n, "usemtl"))
    {
      eastl::string name(p);
      name.trim();
      ok = !name.empty();
      const int id = mats.getNameId(name.c_str(), name.size());
      if (id < 0 && mats.nameCount() > 0xFFFF)
      {
        ok = false;
        why = "more than 65536 materials";
      }
      else
        mat = id >= 0 ? id : mats.addNameId(name.c_str(), name.size());
    }
    else if (word(kw, n, "s"))
    {
      const size_t len = token(p, tok);
      const bool off = word(tok, len, "off");
      ok = len && (off || strspn(tok, "0123456789") == len);
      const long group = off || !ok ? 0 : strtol(tok, nullptr, 10);
      ok = ok && !token(p, tok);
      if (group > 32)
      {
        ok = false;
        why = "a smoothing group above 32";
      }
      smgr = group > 0 && group <= 32 ? 1u << (group - 1) : 0;
    }
    else if (word(kw, n, "f"))
    {
      if (mat == ~0u)
        mat = mats.addNameId("", 0);
      fv.clear();
      ft.clear();
      for (size_t len = token(p, tok); len && ok; len = token(p, tok))
      {
        const char *s = tok, *const e = tok + len;
        uint32_t v = 0, t = ~0u, vn = 0;
        ok = parse_index(s, e, m.pos.size(), v);
        if (ok && s < e && *++s != '/')
          ok = parse_index(s, e, vts.size(), t);
        if (ok && s < e)
          ok = parse_index(++s, e, normals, vn) && s == e;
        fv.push_back(v);
        ft.push_back(t);
      }
      ok = ok && fv.size() >= 3;
      for (uint32_t k = 1; ok && k + 1 < fv.size(); ++k)
      {
        const uint32_t idx[3] = {0, k, k + 1};
        for (uint32_t i : idx)
        {
          m.tri.push_back(fv[i]);
          const Point2 t = ft[i] == ~0u ? Point2(0, 0) : vts[ft[i]];
          uv.push_back(t.x);
          uv.push_back(t.y);
        }
        m.face.push_back(FaceInfo{0, m.face.size(), smgr, uint16_t(mat), 0});
      }
    }
    else if (n)
    {
      ok = false;
      for (const char *t : IGNORED_STATEMENTS)
        ok = ok || word(kw, n, t);
      normals += word(kw, n, "vn");
      if (!ok)
        why = "an unknown statement";
    }
    stmt.clear();
    return ok;
  };
  // the file comes in blocks of any size the seam gives; a line cut by a block end moves to the front and waits for
  // the next block, so the reader holds a block and the longest line, not the file
  constexpr uint32_t BLOCK = 1 << 20, MAX_LINE = 16 * BLOCK;
  dag::Vector<char> buf;
  const int length = df_length(fp);
  uint32_t have = 0, read = 0;
  bool more = true, ok = true;
  while (ok && (more || have))
  {
    // stop reading before have + BLOCK wraps
    more = more && have <= MAX_LINE;
    if (more)
    {
      buf.resize_noinit(have + BLOCK + 1);
      const int got = reader(fp, buf.data() + have, BLOCK);
      // the end before the file's length is a failed read
      if (got < 0 || (got == 0 && length >= 0 && read != uint32_t(length)))
      {
        df_close(fp);
        err.sprintf("cannot read %s past byte %u", path, read);
        m = Mesh();
        return false;
      }
      more = got > 0;
      have += got;
      read += got;
    }
    char *s = buf.data(), *const stop = s + have;
    *stop = 0;
    while (ok && s < stop)
    {
      char *const eol = (char *)memchr(s, '\n', stop - s);
      if (!eol && more)
        break;
      char *end = eol ? eol : stop;
      if (end > s && end[-1] == '\r')
        --end;
      ++line;
      if (stmt.empty())
        first = line;
      // a NUL anywhere in the line is the line's error, as strlen would take the prefix for the whole line
      ok = end - s <= ptrdiff_t(MAX_LINE) && !memchr(s, 0, end - s);
      if (!ok)
      {
        first = line;
        why = end - s <= ptrdiff_t(MAX_LINE) ? "a NUL in the line" : "a line longer than 16 MiB";
      }
      *end = 0;
      if (char *hash = strchr(s, '#'))
        *hash = 0;
      const size_t len = strlen(s);
      const bool continued = len && s[len - 1] == '\\';
      if (ok && stmt.size() + len > MAX_LINE)
      {
        ok = false;
        why = "a statement longer than 16 MiB";
      }
      stmt.append(s, len - continued);
      stmt += ' ';
      s = eol ? eol + 1 : stop;
      if (ok && !continued)
        ok = statement();
    }
    have = stop - s;
    memmove(buf.data(), s, have);
  }
  // a statement continued at the file's end is complete
  if (ok && !stmt.empty())
    ok = statement();
  if (!ok)
  {
    df_close(fp);
    err.sprintf("%s:%u: %s", path, first, why);
    m = Mesh();
    return false;
  }
  df_close(fp);
  // the uv goes in as it is: addChannel would fill a copy with zeros first
  m.channels.push_back(Channel{CH_UV, 2, eastl::move(uv)});
  m.addDefaultChannels();
  return true;
}
} // namespace dacsg

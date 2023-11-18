#include <debug/dag_debug3d.h>
#include <3d/dag_drv3d.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>

struct BufferedLine
{
  Point3 p0;
  Point3 p1;
  E3DCOLOR color;
  size_t deadlineFrame;
  size_t bufferFrames;
};

static Tab<BufferedLine> buffered_line_list(midmem_ptr()); // Note: sorted by deadline
static size_t current_frame = 0;
static bool last_frame_game_was_paused = false;

void draw_debug_line_buffered(const Point3 &p0, const Point3 &p1, E3DCOLOR c, size_t requested_frames)
{
  size_t frames = last_frame_game_was_paused ? 1 : requested_frames;
  size_t deadlineFrame = current_frame + frames;
  for (int last = buffered_line_list.size() - 1, i = last; i >= 0; --i) // lookup place to insert into (according to deadline)
    if (deadlineFrame >= buffered_line_list[i].deadlineFrame)
    {
      if (i == last)
        buffered_line_list.push_back({p0, p1, c, current_frame + frames, frames}); // insert last (newest)
      else
        insert_item_at(buffered_line_list, i + 1, BufferedLine{p0, p1, c, deadlineFrame, frames}); // insert after i
      return;
    }
  insert_item_at(buffered_line_list, 0, BufferedLine{p0, p1, c, deadlineFrame, frames}); // insert first (oldest)
}

void clear_buffered_debug_lines() { clear_and_shrink(buffered_line_list); }

void draw_debug_arrow_buffered(const Point3 &from, const Point3 &to, E3DCOLOR color, size_t frames)
{
  draw_debug_line_buffered(from, to, color, frames);
  Point3 vec = to - from;
  Point3 crossCandidate1 = Point3(10, 0.0, 0.0);
  Point3 crossCandidate2 = Point3(0.0, 10.0, 0.0);
  Point3 crossCandidate = fabs(dot(vec, crossCandidate1)) > 0.5 ? crossCandidate2 : crossCandidate1;
  Point3 normalVec1 = normalize(cross(vec, crossCandidate));
  Point3 normalVec2 = normalize(cross(vec, normalVec1));
  const float headRelativeSize = 0.1;
  Point3 headOrigin = from + vec * (1 - headRelativeSize);
  float headSize = vec.length() * headRelativeSize * 0.5;
  float s, c;
  const int numFeathers = 3;
  for (int i = 0; i < numFeathers; i++)
  {
    sincos(2 * i * PI / (float)numFeathers, s, c);
    draw_debug_line_buffered(headOrigin + (normalVec1 * c + normalVec2 * s) * headSize, to, color, frames);
  }
}

void draw_debug_rect_buffered(const Point3 &p0, const Point3 &p1, const Point3 &p2, E3DCOLOR color, size_t frames)
{
  draw_debug_line_buffered(p0, p1, color, frames);
  draw_debug_line_buffered(p0, p2, color, frames);
  draw_debug_line_buffered(p1, p1 + p2 - p0, color, frames);
  draw_debug_line_buffered(p1 + p2 - p0, p2, color, frames);
}

void draw_debug_box_buffered(const BBox3 &box, E3DCOLOR color, size_t frames)
{
#define DRLN(p0, p1) draw_debug_line_buffered((p0), (p1), color, frames);
  DRLN(Point3(box.lim[0].x, box.lim[0].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[0].y, box.lim[0].z));
  DRLN(Point3(box.lim[0].x, box.lim[1].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[0].z));
  DRLN(Point3(box.lim[0].x, box.lim[1].y, box.lim[1].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[1].z));
  DRLN(Point3(box.lim[0].x, box.lim[0].y, box.lim[1].z), Point3(box.lim[1].x, box.lim[0].y, box.lim[1].z));
  DRLN(Point3(box.lim[0].x, box.lim[0].y, box.lim[0].z), Point3(box.lim[0].x, box.lim[1].y, box.lim[0].z));
  DRLN(Point3(box.lim[1].x, box.lim[0].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[0].z));
  DRLN(Point3(box.lim[1].x, box.lim[0].y, box.lim[1].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[1].z));
  DRLN(Point3(box.lim[0].x, box.lim[0].y, box.lim[1].z), Point3(box.lim[0].x, box.lim[1].y, box.lim[1].z));
  DRLN(Point3(box.lim[0].x, box.lim[0].y, box.lim[0].z), Point3(box.lim[0].x, box.lim[0].y, box.lim[1].z));
  DRLN(Point3(box.lim[1].x, box.lim[0].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[0].y, box.lim[1].z));
  DRLN(Point3(box.lim[1].x, box.lim[1].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[1].z));
  DRLN(Point3(box.lim[0].x, box.lim[1].y, box.lim[0].z), Point3(box.lim[0].x, box.lim[1].y, box.lim[1].z));
#undef DRLN
}

void draw_debug_box_buffered(const Point3 &p0, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color, size_t frames)
{
  draw_debug_line_buffered(p0, p0 + ax, color, frames);
  draw_debug_line_buffered(p0, p0 + ay, color, frames);
  draw_debug_line_buffered(p0, p0 + az, color, frames);
  draw_debug_line_buffered(p0 + ax + ay, p0 + ax, color, frames);
  draw_debug_line_buffered(p0 + ax + ay, p0 + ay, color, frames);
  draw_debug_line_buffered(p0 + ax + ay, p0 + ax + ay + az, color, frames);
  draw_debug_line_buffered(p0 + ax + az, p0 + ax, color, frames);
  draw_debug_line_buffered(p0 + ax + az, p0 + az, color, frames);
  draw_debug_line_buffered(p0 + ax + az, p0 + ax + ay + az, color, frames);
  draw_debug_line_buffered(p0 + ay + az, p0 + ay, color, frames);
  draw_debug_line_buffered(p0 + ay + az, p0 + az, color, frames);
  draw_debug_line_buffered(p0 + ay + az, p0 + ax + ay + az, color, frames);
}


void draw_debug_box_buffered(const BBox3 &b, const TMatrix &tm, E3DCOLOR color, size_t frames)
{
  draw_debug_box_buffered(tm * b[0], tm.getcol(0) * b.width()[0], tm.getcol(1) * b.width()[1], tm.getcol(2) * b.width()[2], color,
    frames);
}

void draw_debug_elipse_buffered(const Point3 &pos, const Point3 &half_diag1, const Point3 &half_diag2, E3DCOLOR col, int segs,
  size_t frames)
{
  const int MAX_SEGS = 256;

  if (segs < 3)
    segs = 3;
  else if (segs > MAX_SEGS)
    segs = MAX_SEGS;

  Point3 pt[MAX_SEGS];

  for (int i = 0; i < segs; ++i)
  {
    real a = i * TWOPI / segs;
    pt[i] = pos + half_diag1 * cosf(a) + half_diag2 * sinf(a);
  }

  for (int i = 0; i < segs; ++i)
  {
    int idx2 = i == 0 ? segs - 1 : i - 1;
    draw_debug_line_buffered(pt[i], pt[idx2], col, frames);
  }
}

void draw_debug_circle_buffered(const Point3 &pos, Point3 norm, real rad, E3DCOLOR col, int segs, size_t frames)
{
  if (rad < 0)
    return;

  norm.normalize(); // Just in case
  Point3 noise = fabs(dot(norm, Point3(1, 0, 0))) < 0.5 ? Point3(1, 0, 0) : Point3(0, 1, 0);
  Point3 ax1 = normalize(cross(norm, norm + noise));
  Point3 ax2 = cross(norm, ax1);

  draw_debug_elipse_buffered(pos, ax1 * rad, ax2 * rad, col, segs, frames);
}

void draw_debug_sphere_buffered(const Point3 &c, real rad, E3DCOLOR col, int segs, size_t frames)
{
#define DRLN(p0, p1, col) draw_debug_line_buffered((p0), (p1), col, frames);
  if (rad < 0)
    return;

  const int MAX_SEGS = 64;

  if (segs < 3)
    segs = 3;
  else if (segs > MAX_SEGS)
    segs = MAX_SEGS;

  Point2 pt[MAX_SEGS];

  for (int i = 0; i < segs; ++i)
  {
    real a = i * TWOPI / segs;
    pt[i] = Point2(cosf(a), sinf(a)) * rad;
  }

  for (int i = 0; i < segs; ++i)
  {
    const Point2 &p0 = pt[i == 0 ? segs - 1 : i - 1];
    const Point2 &p1 = pt[i];
    DRLN(Point3(p0.x, p0.y, 0) + c, Point3(p1.x, p1.y, 0) + c, col);
    DRLN(Point3(p0.x, 0, p0.y) + c, Point3(p1.x, 0, p1.y) + c, col);
    DRLN(Point3(0, p0.x, p0.y) + c, Point3(0, p1.x, p1.y) + c, col);
  }

  DRLN(c - Point3(rad, 0, 0), c + Point3(rad, 0, 0), col);
  DRLN(c - Point3(0, rad, 0), c + Point3(0, rad, 0), col);
  DRLN(c - Point3(0, 0, rad), c + Point3(0, 0, rad), col);
#undef DRLN
}

void draw_debug_capsule_buffered(const Point3 &p0, const Point3 &p1, real rad, E3DCOLOR col, int segs, size_t frames)
{
  Point3 dir = (p1 - p0);
  float length = dir.length();
  dir *= safeinv(length);
  Point3 norm;
  if (rabs(dir.x) >= FLT_EPSILON)
    norm = Point3(0, dir.z, -dir.y); // dir % Point3(1,0,0)
  else
    norm = Point3(dir.z, 0, -dir.x); // Point3(0, 1, 0) % dir
  norm.normalize();
  ::draw_debug_line_buffered(p0, p0 + dir * length, col, frames);

  const float angleStep = 2 * PI / segs;
  for (size_t i = 0; i < segs; ++i)
  {
    Quat quaternion = Quat(dir, i * angleStep);
    Point3 newDir = quaternion * norm;
    Point3 newPos = p0 + newDir * rad;
    ::draw_debug_line_buffered(newPos, newPos + dir * length, col, frames);
  }

  ::draw_debug_sphere_buffered(p0, rad, col, segs, frames);
  ::draw_debug_sphere_buffered(p1, rad, col, segs, frames);
}

void draw_debug_tube_buffered(const Point3 &p0, const Point3 &p1, float radius, E3DCOLOR col, int segs, float circle_density,
  size_t frames)
{
  Point3 dir = p1 - p0;
  float len = dir.length();
  int count = len * circle_density + 1;
  for (int i = 0; i < count; i++)
    draw_debug_circle_buffered(p0 + dir * (float(i) / count), dir, radius, col, segs, frames);
  draw_debug_circle_buffered(p1, dir, radius, col, segs, frames);
}

void draw_debug_cone_buffered(const Point3 &p0, const Point3 &p1, real angleRad, E3DCOLOR col, int segs, size_t frames)
{
  Point3 dir = (p1 - p0);
  float length = dir.length();
  dir *= safeinv(length);
  Point3 norm;
  if (rabs(dir.x) >= FLT_EPSILON)
    norm = Point3(0, dir.z, -dir.y); // dir % Point3(1,0,0)
  else
    norm = Point3(dir.z, 0, -dir.x); // Point3(0, 1, 0) % dir
  norm.normalize();
  ::draw_debug_line_buffered(p0, p0 + dir * length, col, frames);
  float rad = length * tanf(angleRad);

  const float angleStep = 2 * PI / segs;
  Point3 lastPoint = ZERO<Point3>();
  Point3 firstPoint = ZERO<Point3>();
  for (size_t i = 0; i < segs; ++i)
  {
    Quat quaternion = Quat(dir, i * angleStep);
    Point3 newDir = quaternion * norm;
    Point3 newPos = p0 + newDir * rad;
    Point3 pushedPoint = newPos + dir * length;
    ::draw_debug_line_buffered(p0, pushedPoint, col, frames);
    if (i > 0)
      ::draw_debug_line_buffered(lastPoint, pushedPoint, col, frames);
    else
      firstPoint = newPos + dir * length;
    lastPoint = newPos + dir * length;
  }
  ::draw_debug_line_buffered(lastPoint, firstPoint, col, frames);
}

// Deliberately upside-down as this seems more useful.
// Can always be inverted using negative radius.
static carray<Point3, 4> tetra_vertices()
{
  float s = 0, c = 0;
  sincos(PI / 3, s, c);
  return carray<Point3, 4>{Point3(0, -1, 0), Point3(s, c, 0), Point3(-s * c, c, s), Point3(-s * c, c, -s)};
}

void draw_debug_tetrapod_buffered(const Point3 &c, real radius, E3DCOLOR col, size_t frames)
{
  carray<Point3, 4> vertices = tetra_vertices();
  for (auto &v : vertices)
    ::draw_debug_line_buffered(c, c + v * radius, col, frames);
}

void draw_debug_tehedron_buffered(const Point3 &c, real radius, E3DCOLOR col, size_t frames)
{
  carray<Point3, 4> vertices = tetra_vertices();
  for (int i = 0; i < vertices.size(); i++)
    for (int k = i + 1; k < vertices.size(); k++)
      ::draw_debug_line_buffered(c + vertices[i] * radius, c + vertices[k] * radius, col, frames);
}

static int draw_buffered_lines(dag::ConstSpan<BufferedLine> lines, size_t cur_frame, E3DCOLOR force_color = E3DCOLOR(0))
{
#if DAGOR_DBGLEVEL > 1
  for (int i = 1; i < lines.size(); ++i)
    G_ASSERT(lines[i].deadlineFrame >= lines[i - 1].deadlineFrame);
#endif

  begin_draw_cached_debug_lines(true, false); // Note: this sets wtm to identity

  int eraseNum = -1;
  for (int i = 0; i < lines.size(); ++i)
  {
    const BufferedLine &ln = lines[i];
    if (cur_frame >= ln.deadlineFrame)
      eraseNum = i + 1;
    else
    {
      E3DCOLOR color = force_color.u ? force_color : ln.color;
      size_t framesLeft = ln.deadlineFrame - cur_frame;
      color.a =
        static_cast<uint8_t>(color.a * (float(framesLeft * framesLeft) / float(ln.bufferFrames * ln.bufferFrames))); // use floating
                                                                                                                     // point math to
                                                                                                                     // avoid
                                                                                                                     // oveflowing
                                                                                                                     // squares for big
                                                                                                                     // 'frames'
      ::draw_cached_debug_line(ln.p0, ln.p1, color);
    }
  }

  end_draw_cached_debug_lines();

  return eraseNum;
}

void flush_buffered_debug_lines(bool game_is_paused)
{
  last_frame_game_was_paused = game_is_paused;
  if (buffered_line_list.empty())
    return;

  if (game_is_paused)
    for (auto &line : buffered_line_list)
      if (line.bufferFrames > 1)
        line.deadlineFrame++;

  int eraseNum = draw_buffered_lines(buffered_line_list, current_frame);
  if (eraseNum > 0)
    erase_items(buffered_line_list, 0, eraseNum);

  current_frame++;
}

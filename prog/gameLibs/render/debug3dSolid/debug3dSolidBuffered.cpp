#include <render/debug3dSolidBuffered.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_mathUtils.h>

struct BufferedMesh
{
  size_t deadlineFrame;
  size_t bufferFrames;
  Color4 color;
  TMatrix tm;
  Tab<uint16_t> indices;
  Tab<float> xyz_pos;
};

struct BufferedSphere
{
  size_t deadlineFrame;
  size_t bufferFrames;
  Color4 color;
  Point3 c;
  float r;
};

static Tab<BufferedMesh> buffered_mesh_list(midmem_ptr());     // Note: sorted by deadline
static Tab<BufferedSphere> buffered_sphere_list(midmem_ptr()); // Note: sorted by deadline
static size_t current_frame = 0;
static bool last_frame_game_was_paused = false;

void clear_buffered_debug_solids()
{
  clear_and_shrink(buffered_mesh_list);
  clear_and_shrink(buffered_sphere_list);
}

static BufferedMesh get_mesh(const uint16_t *indices, int faces_count, const float *xyz_pos, int vertex_size, int vertices_count,
  const TMatrix &tm, Color4 color, size_t requested_frames)
{
  size_t frames = last_frame_game_was_paused ? 1 : requested_frames;
  BufferedMesh mesh = {current_frame + frames, frames, color, tm};

  mesh.indices = Tab<uint16_t>();
  mesh.indices.reserve(faces_count * 3);
  mesh.indices.resize(faces_count * 3);
  mem_copy_from(mesh.indices, indices);

  mesh.xyz_pos = Tab<float>();
  mesh.xyz_pos.reserve(vertex_size * vertices_count);
  mesh.xyz_pos.resize(vertex_size * vertices_count);
  mem_copy_from(mesh.xyz_pos, xyz_pos);

  return mesh;
}

void draw_debug_solid_mesh_buffered(const uint16_t *indices, int faces_count, const float *xyz_pos, int vertex_size,
  int vertices_count, const TMatrix &tm, Color4 color, size_t requested_frames)
{
  if (faces_count <= 0 || vertex_size <= 0 || vertices_count <= 0)
    return;
  size_t frames = last_frame_game_was_paused ? 1 : requested_frames;
  size_t deadlineFrame = current_frame + frames;
  for (int last = buffered_mesh_list.size() - 1, i = last; i >= 0; --i) // lookup place to insert into (according to deadline)
    if (deadlineFrame >= buffered_mesh_list[i].deadlineFrame)
    {
      if (i == last)
        buffered_mesh_list.push_back(
          get_mesh(indices, faces_count, xyz_pos, vertex_size, vertices_count, tm, color, frames)); // insert last (newest)
      else
        insert_item_at(buffered_mesh_list, i + 1,
          get_mesh(indices, faces_count, xyz_pos, vertex_size, vertices_count, tm, color, frames)); // insert after i
      return;
    }
  insert_item_at(buffered_mesh_list, 0,
    get_mesh(indices, faces_count, xyz_pos, vertex_size, vertices_count, tm, color, frames)); // insert first (oldest)
}

void draw_debug_ball_buffered(const Point3 &sphere_c, float sphere_r, const Color4 &color, size_t requested_frames)
{
  size_t frames = last_frame_game_was_paused ? 1 : requested_frames;
  size_t deadlineFrame = current_frame + frames;
  for (int last = buffered_sphere_list.size() - 1, i = last; i >= 0; --i) // lookup place to insert into (according to deadline)
    if (deadlineFrame >= buffered_sphere_list[i].deadlineFrame)
    {
      if (i == last)
        buffered_sphere_list.push_back({deadlineFrame, frames, color, sphere_c, sphere_r}); // insert last (newest)
      else
        insert_item_at(buffered_sphere_list, i + 1, BufferedSphere{deadlineFrame, frames, color, sphere_c, sphere_r}); // insert after
                                                                                                                       // i
      return;
    }
  insert_item_at(buffered_sphere_list, 0, BufferedSphere{deadlineFrame, frames, color, sphere_c, sphere_r}); // insert first (oldest)
}

void draw_debug_solid_cube_buffered(const BBox3 &cube, const TMatrix &tm, const Color4 &color, size_t frames)
{
  Point3 vertices[8];
  for (int i = 0; i < countof(vertices); ++i)
    vertices[i] = cube.point(i);

  static const uint16_t box_indices[72] = {0, 3, 1, 0, 2, 3, 0, 1, 5, 0, 5, 4, 0, 4, 6, 0, 6, 2, 1, 7, 5, 1, 3, 7, 4, 5, 7, 4, 7, 6, 2,
    7, 3, 2, 6, 7,

    1, 3, 0, 3, 2, 0, 5, 1, 0, 4, 5, 0, 6, 4, 0, 2, 6, 0, 5, 7, 1, 7, 3, 1, 7, 5, 4, 6, 7, 4, 3, 7, 2, 7, 6, 2};

  draw_debug_solid_mesh_buffered(box_indices, countof(box_indices) / 3, &vertices[0].x, sizeof(Point3), countof(vertices), tm, color,
    frames);
}

void draw_debug_solid_triangle_buffered(Point3 a, Point3 b, Point3 c, const Color4 &color, size_t frames)
{
  Point3 vertices[3] = {a, b, c};

  static const uint16_t indices[6] = {0, 1, 2, 0, 2, 1}; // double - sided

  draw_debug_solid_mesh_buffered(indices, countof(indices) / 3, &vertices[0].x, sizeof(Point3), countof(vertices), TMatrix::IDENT,
    color, frames);
}

void draw_debug_solid_disk_buffered(const Point3 pos, Point3 norm, float radius, int segments, const Color4 &color, size_t frames)
{
  if (radius == 0)
    return;
  segments = segments < 3 ? min((int)max(ceil(radius * PI), 6.f), 48) : segments;
  dag::Vector<Point3> vertices(segments);
  dag::Vector<uint16_t> indices;
  indices.reserve((segments - 2) * 3);

  norm.normalize(); // Just in case
  Point3 noise = fabs(dot(norm, Point3(1, 0, 0))) < 0.5 ? Point3(1, 0, 0) : Point3(0, 1, 0);
  Point3 ax1 = cross(norm, norm + noise);
  Point3 ax2 = cross(norm, ax1);
  for (int i = 0; i < segments; i++)
  {
    // alternating clockwise and counter-clockwise angles
    int halfIdx = (i + 1) / 2;
    int dir = i % 2 == 0 ? 1 : -1;
    float angle = ((float)(halfIdx * dir)) * 2.f * PI / segments;
    float s, c;
    sincos(angle, s, c);
    Point3 p = pos + ax1 * c * radius + ax2 * s * radius;
    vertices[i] = p;
    if (i > 1)
    {
      if (i % 2 == 0)
      {
        indices.push_back(i);
        indices.push_back(i - 1);
        indices.push_back(i - 2);
      }
      else
      {
        indices.push_back(i - 2);
        indices.push_back(i - 1);
        indices.push_back(i);
      }
    }
  }

  draw_debug_solid_mesh_buffered(indices.data(), segments - 2, &vertices[0].x, sizeof(Point3), segments, TMatrix::IDENT, color,
    frames);
}

void draw_debug_solid_cone_buffered(const Point3 pos, Point3 norm, float radius, float height, int segments, const Color4 &color,
  size_t frames)
{
  if (radius <= 0)
    return;
  segments = segments < 3 ? min((int)max(ceil(radius * PI), 6.f), 48) : segments;
  const size_t vertexCount = segments + 2;
  const size_t indexCount = segments * 6;
  dag::Vector<Point3> vertices(vertexCount);
  dag::Vector<uint16_t> indices;
  indices.reserve(indexCount);

  norm.normalize(); // Just in case
  Point3 noise = fabs(dot(norm, Point3(1, 0, 0))) < 0.5 ? Point3(1, 0, 0) : Point3(0, 1, 0);
  Point3 ax1 = cross(norm, norm + noise);
  Point3 ax2 = cross(norm, ax1);
  for (int i = 0; i < segments; i++)
  {
    float angle = i * 2.f * PI / segments;
    float s, c;
    sincos(angle, s, c);
    Point3 p = pos + ax1 * c * radius + ax2 * s * radius;
    vertices[i] = p;
    int i2 = (i + 1) % segments;
    indices.push_back(i2);
    indices.push_back(i);
    indices.push_back(vertexCount - 1);
    indices.push_back(i);
    indices.push_back(i2);
    indices.push_back(vertexCount - 2);
  }

  vertices[vertexCount - 1] = pos + norm * height;
  vertices[vertexCount - 2] = pos;

  draw_debug_solid_mesh_buffered(indices.data(), indexCount / 3, &vertices[0].x, sizeof(Point3), vertexCount, TMatrix::IDENT, color,
    frames);
}

void draw_debug_solid_quad_buffered(Point3 half_vec_i, Point3 half_vec_j, const TMatrix &tm, const Color4 &color, size_t frames)
{
  Point3 vertices[4] = {-half_vec_i - half_vec_j, half_vec_i - half_vec_j, -half_vec_i + half_vec_j, half_vec_i + half_vec_j};

  static const uint16_t indices[12] = {0, 3, 1, 0, 2, 3, 1, 3, 0, 3, 2, 0}; // double - sided

  draw_debug_solid_mesh_buffered(indices, countof(indices) / 3, &vertices[0].x, sizeof(Point3), countof(vertices), tm, color, frames);
}

void draw_debug_solid_quad_buffered(Point3 top_left, Point3 bottom_left, Point3 bottom_right, Point3 top_right, const Color4 &color,
  size_t frames)
{
  Point3 vertices[4] = {top_left, bottom_left, bottom_right, top_right};

  static const uint16_t indices[12] = {0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0};

  draw_debug_solid_mesh_buffered(indices, countof(indices) / 3, &vertices[0].x, sizeof(Point3), countof(vertices), TMatrix::IDENT,
    color, frames);
}

static int draw_buffered_meshes(dag::ConstSpan<BufferedMesh> meshes, size_t cur_frame)
{
  int eraseNum = -1;
  for (int i = 0; i < meshes.size(); ++i)
  {
    const BufferedMesh &mesh = meshes[i];
    if (cur_frame >= mesh.deadlineFrame)
      eraseNum = i + 1;
    else
      draw_debug_solid_mesh(&mesh.indices.front(), mesh.indices.size() / 3, &mesh.xyz_pos.front(), sizeof(Point3),
        mesh.xyz_pos.size() / sizeof(Point3), mesh.tm, mesh.color);
  }

  return eraseNum;
}

static int draw_buffered_spheres(dag::ConstSpan<BufferedSphere> spheres, size_t cur_frame)
{
  int eraseNum = -1;
  for (int i = 0; i < spheres.size(); ++i)
  {
    const BufferedSphere &sphere = spheres[i];
    if (cur_frame >= sphere.deadlineFrame)
      eraseNum = i + 1;
    else
      draw_debug_solid_sphere(sphere.c, sphere.r, TMatrix::IDENT, sphere.color);
  }

  return eraseNum;
}

void flush_buffered_debug_meshes(bool game_is_paused)
{
  last_frame_game_was_paused = game_is_paused;
  if (buffered_mesh_list.empty() && buffered_sphere_list.empty())
    return;

  if (game_is_paused)
  {
    for (auto &mesh : buffered_mesh_list)
      if (mesh.bufferFrames > 1)
        mesh.deadlineFrame++;
    for (auto &sphere : buffered_sphere_list)
      if (sphere.bufferFrames > 1)
        sphere.deadlineFrame++;
  }

  int eraseNum = draw_buffered_meshes(buffered_mesh_list, current_frame);
  if (eraseNum > 0)
    erase_items(buffered_mesh_list, 0, eraseNum);
  eraseNum = draw_buffered_spheres(buffered_sphere_list, current_frame);
  if (eraseNum > 0)
    erase_items(buffered_sphere_list, 0, eraseNum);

  current_frame++;
}
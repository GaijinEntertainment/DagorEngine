// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaders.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_e3dColor.h>

// forward declarations for external classes
class DynamicShadersBuffer;


/// Used to render various geometry by means of built-in shader buffer
/// (see #DynamicShadersBuffer).
/// At first the data are accumulated in internal class's buffer,
/// then after calling flushToBuffer() they are moved to shader buffer.
/// After call to flush() the data are displayed on the screen.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
class DynRenderBuffer
{
public:
  /// Vertex.
  struct Vertex
  {
    Point3 pos;     ///< Spatial coordinates
    E3DCOLOR color; ///< Color
    Point2 tc;      ///< Texture coordinates
  };

  enum SafetyBounds
  {
    SAFETY_MAX_BUFFER_VERTEX = 0xF0FF,
    SAFETY_MAX_BUFFER_FACES = SAFETY_MAX_BUFFER_VERTEX,
  };

  /// Constructor.
  /// @param[in] class_name - name of shader used for rendering
  DynRenderBuffer(const char *class_name = "editor_helper");

  /// Destructor.
  ~DynRenderBuffer();

  /// Get vertex count.
  /// @return number of vertices in buffer
  inline int getVertexCount() const { return edVerts.size(); }

  /// Get vertex index count.
  /// @return number of vertex indexes in buffer
  inline int getIndexCount() const { return edFaces.size(); }

  //*******************************************************
  ///@name Data accumulation methods.
  //@{
  /// Clear buffer.
  void clearBuf();

  /// Put arbitrary geometry in buffer.
  /// @param[in] vertex_tab - array of vertexes
  /// @param[in] face_tab - array of vertex indexes
  void drawCustom(dag::ConstSpan<Vertex> vertex_tab, dag::ConstSpan<int> face_tab);

  /// starts filling data for net surface; reserves space for w*h vertices and inits indices;
  /// returns pointer to first vertex, vertices are arranged as (0,0), (1,0) ... (w-1,0), (0, 1), (1,1), etc.
  Vertex *drawNetSurface(int w, int h);

  /// Put quad in buffer.
  /// @param[in] p0, p1, p2, p3 - coordinates of vertexes
  /// @param[in] color - quad color
  void drawQuad(const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3, E3DCOLOR color, float u = 1, float v = 1);

  /// Place rectangular line on surface perpendicular to camera line of view
  /// and put it in buffer.
  /// @param[in] from - start coordinates of the line
  /// @param[in] to - end coordinates of the line
  /// @param[in] width - line width
  /// @param[in] color - line color
  void drawLine(const Point3 &from, const Point3 &to, real width, E3DCOLOR color);

  /// Place square on surface perpendicular to camera line of view and put
  /// it in buffer.
  /// @param[in] p - coordinates of square center
  /// @param[in] radius - half length of square side
  /// @param[in] color - square color
  void drawSquare(const Point3 &p, real radius, E3DCOLOR color);

  /// Place ring on surface perpendicular to camera line of view and put it
  /// in buffer.
  /// @param[in] pos - coordinates of ring center
  /// @param[in] radius - ring radius
  /// @param[in] width - ring width
  /// @param[in] color - ring color
  void drawDebugSphere(const Point3 &pos, real radius, real width, E3DCOLOR color);

  //???
  void drawWireSphere(const Point3 &pos, real radius, real width, E3DCOLOR color);

  //???
  void drawSphere(const TMatrix &tm, E3DCOLOR color, int segs = -1);

  /// Put parallelepiped (box) in buffer.
  /// @param[in] tm - the matrix that defines coordinates, dimensions and
  ///                 rotation of the parallelepiped.
  ///                 Parallelepiped is the result of matrix multiplication,
  ///                 where one matrix represents cube with side equal 1 metre
  ///                 and other matrix is tm matrix.
  /// @param[in] color - parallelepiped color
  void drawBox(const TMatrix &tm, E3DCOLOR color);

  //???
  void drawWireBox(const TMatrix &tm, real width, E3DCOLOR color);
  //@}

  //*******************************************************
  ///@name Data render methods.
  //@{
  /// The function begins process of rendering shader geometry
  /// and calls addFaces().
  /// @param[in] tid - ID of texture used for rendering
  void flushToBuffer(TEXTUREID tid) { addFaces(tid); }

  /// The function moves data from internal buffer to shader buffer.
  /// @param[in] tid - ID of texture used for rendering
  void addFaces(TEXTUREID tid);

  /// The function outputs shader buffer to screen
  /// and ends process of rendering shader geometry
  /// (see #ShaderMesh::end_render()).
  void flush();
  //@}

  int addVert(Vertex *v, int numv) { return append_items(edVerts, numv, v); }
  void addInd(int *i, int numi, int sv)
  {
    int idx = append_items(edFaces, numi);
    int *d = edFaces.data() + idx;
    for (; numi > 0; i++, d++, numi--)
      *d = *i + sv;
  }

private:
  typedef Tab<Vertex> VertexTab;
  typedef Tab<int> FaceTab;

  DynamicShadersBuffer *edBuffer;
  CompiledShaderChannelId channels[3];
  Ptr<ShaderMaterial> edMat;
  Ptr<ShaderElement> edShader;

  int texVarId;
  int texSamplerstateVarId;
  VertexTab edVerts;
  FaceTab edFaces;
  unsigned vertexMaxCount;
  unsigned faceMaxCount;

  void createBuffer(int max_verts, int max_faces);
};

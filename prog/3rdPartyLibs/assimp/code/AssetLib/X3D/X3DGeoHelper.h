#pragma once

#include <assimp/vector2.h>
#include <assimp/vector3.h>
#include <assimp/color4.h>
#include <assimp/types.h>

#include <EASTL/list.h>
#include <EASTL/vector.h>

struct aiFace;
struct aiMesh;

namespace Assimp {

class X3DGeoHelper {
public:
    static aiVector3D make_point2D(float angle, float radius);
    static void make_arc2D(float pStartAngle, float pEndAngle, float pRadius, size_t numSegments, eastl::list<aiVector3D> &pVertices);
    static void extend_point_to_line(const eastl::list<aiVector3D> &pPoint, eastl::list<aiVector3D> &pLine);
    static void polylineIdx_to_lineIdx(const eastl::list<int32_t> &pPolylineCoordIdx, eastl::list<int32_t> &pLineCoordIdx);
    static void rect_parallel_epiped(const aiVector3D &pSize, eastl::list<aiVector3D> &pVertices);
    static void coordIdx_str2faces_arr(const eastl::vector<int32_t> &pCoordIdx, eastl::vector<aiFace> &pFaces, unsigned int &pPrimitiveTypes);
    static void add_color(aiMesh &pMesh, const eastl::list<aiColor3D> &pColors, const bool pColorPerVertex);
    static void add_color(aiMesh &pMesh, const eastl::list<aiColor4D> &pColors, const bool pColorPerVertex);
    static void add_color(aiMesh &pMesh, const eastl::vector<int32_t> &pCoordIdx, const eastl::vector<int32_t> &pColorIdx,
            const eastl::list<aiColor3D> &pColors, const bool pColorPerVertex);
    static void add_color(aiMesh &pMesh, const eastl::vector<int32_t> &pCoordIdx, const eastl::vector<int32_t> &pColorIdx,
            const eastl::list<aiColor4D> &pColors, const bool pColorPerVertex);
    static void add_normal(aiMesh &pMesh, const eastl::vector<int32_t> &pCoordIdx, const eastl::vector<int32_t> &pNormalIdx,
            const eastl::list<aiVector3D> &pNormals, const bool pNormalPerVertex);
    static void add_normal(aiMesh &pMesh, const eastl::list<aiVector3D> &pNormals, const bool pNormalPerVertex);
    static void add_tex_coord(aiMesh &pMesh, const eastl::vector<int32_t> &pCoordIdx, const eastl::vector<int32_t> &pTexCoordIdx,
            const eastl::list<aiVector2D> &pTexCoords);
    static void add_tex_coord(aiMesh &pMesh, const eastl::list<aiVector2D> &pTexCoords);
    static aiMesh *make_mesh(const eastl::vector<int32_t> &pCoordIdx, const eastl::list<aiVector3D> &pVertices);
};

} // namespace Assimp

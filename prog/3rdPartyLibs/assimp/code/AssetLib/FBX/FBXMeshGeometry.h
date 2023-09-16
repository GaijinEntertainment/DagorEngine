/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file  FBXImporter.h
*  @brief Declaration of the FBX main importer class
*/
#ifndef INCLUDED_AI_FBX_MESHGEOMETRY_H
#define INCLUDED_AI_FBX_MESHGEOMETRY_H

#include "FBXParser.h"
#include "FBXDocument.h"

namespace Assimp {
namespace FBX {

/**
 *  DOM base class for all kinds of FBX geometry
 */
class Geometry : public Object {
public:
    /// @brief The class constructor with all parameters.
    /// @param id       The id.
    /// @param element  
    /// @param name 
    /// @param doc 
    Geometry( uint64_t id, const Element& element, const eastl::string& name, const Document& doc );
    virtual ~Geometry() = default;

    /// Get the Skin attached to this geometry or nullptr
    const Skin* DeformerSkin() const;

    /// Get the BlendShape attached to this geometry or nullptr
    const eastl::vector<const BlendShape*>& GetBlendShapes() const;

private:
    const Skin* skin;
    eastl::vector<const BlendShape*> blendShapes;
};

typedef eastl::vector<int> MatIndexArray;


/**
 *  DOM class for FBX geometry of type "Mesh"
 */
class MeshGeometry : public Geometry {
public:
    /** The class constructor */
    MeshGeometry( uint64_t id, const Element& element, const eastl::string& name, const Document& doc );

    /** The class destructor */
    virtual ~MeshGeometry() = default;

    /** Get a list of all vertex points, non-unique*/
    const eastl::vector<aiVector3D>& GetVertices() const;

    /** Get a list of all vertex normals or an empty array if
    *  no normals are specified. */
    const eastl::vector<aiVector3D>& GetNormals() const;

    /** Get a list of all vertex tangents or an empty array
    *  if no tangents are specified */
    const eastl::vector<aiVector3D>& GetTangents() const;

    /** Get a list of all vertex bi-normals or an empty array
    *  if no bi-normals are specified */
    const eastl::vector<aiVector3D>& GetBinormals() const;

    /** Return list of faces - each entry denotes a face and specifies
    *  how many vertices it has. Vertices are taken from the
    *  vertex data arrays in sequential order. */
    const eastl::vector<unsigned int>& GetFaceIndexCounts() const;

    /** Get a UV coordinate slot, returns an empty array if
    *  the requested slot does not exist. */
    const eastl::vector<aiVector2D>& GetTextureCoords( unsigned int index ) const;

    /** Get a UV coordinate slot, returns an empty array if
    *  the requested slot does not exist. */
    eastl::string GetTextureCoordChannelName( unsigned int index ) const;

    /** Get a vertex color coordinate slot, returns an empty array if
    *  the requested slot does not exist. */
    const eastl::vector<aiColor4D>& GetVertexColors( unsigned int index ) const;

    /** Get per-face-vertex material assignments */
    const MatIndexArray& GetMaterialIndices() const;

    /** Convert from a fbx file vertex index (for example from a #Cluster weight) or nullptr
    * if the vertex index is not valid. */
    const unsigned int* ToOutputVertexIndex( unsigned int in_index, unsigned int& count ) const;

    /** Determine the face to which a particular output vertex index belongs.
    *  This mapping is always unique. */
    unsigned int FaceForVertexIndex( unsigned int in_index ) const;

private:
    void ReadLayer( const Scope& layer );
    void ReadLayerElement( const Scope& layerElement );
    void ReadVertexData( const eastl::string& type, int index, const Scope& source );

    void ReadVertexDataUV( eastl::vector<aiVector2D>& uv_out, const Scope& source,
        const eastl::string& MappingInformationType,
        const eastl::string& ReferenceInformationType );

    void ReadVertexDataNormals( eastl::vector<aiVector3D>& normals_out, const Scope& source,
        const eastl::string& MappingInformationType,
        const eastl::string& ReferenceInformationType );

    void ReadVertexDataColors( eastl::vector<aiColor4D>& colors_out, const Scope& source,
        const eastl::string& MappingInformationType,
        const eastl::string& ReferenceInformationType );

    void ReadVertexDataTangents( eastl::vector<aiVector3D>& tangents_out, const Scope& source,
        const eastl::string& MappingInformationType,
        const eastl::string& ReferenceInformationType );

    void ReadVertexDataBinormals( eastl::vector<aiVector3D>& binormals_out, const Scope& source,
        const eastl::string& MappingInformationType,
        const eastl::string& ReferenceInformationType );

    void ReadVertexDataMaterials( MatIndexArray& materials_out, const Scope& source,
        const eastl::string& MappingInformationType,
        const eastl::string& ReferenceInformationType );

private:
    // cached data arrays
    MatIndexArray m_materials;
    eastl::vector<aiVector3D> m_vertices;
    eastl::vector<unsigned int> m_faces;
    mutable eastl::vector<unsigned int> m_facesVertexStartIndices;
    eastl::vector<aiVector3D> m_tangents;
    eastl::vector<aiVector3D> m_binormals;
    eastl::vector<aiVector3D> m_normals;

    eastl::string m_uvNames[ AI_MAX_NUMBER_OF_TEXTURECOORDS ];
    eastl::vector<aiVector2D> m_uvs[ AI_MAX_NUMBER_OF_TEXTURECOORDS ];
    eastl::vector<aiColor4D> m_colors[ AI_MAX_NUMBER_OF_COLOR_SETS ];

    eastl::vector<unsigned int> m_mapping_counts;
    eastl::vector<unsigned int> m_mapping_offsets;
    eastl::vector<unsigned int> m_mappings;
};

/**
*  DOM class for FBX geometry of type "Shape"
*/
class ShapeGeometry : public Geometry
{
public:
    /** The class constructor */
    ShapeGeometry(uint64_t id, const Element& element, const eastl::string& name, const Document& doc);

    /** The class destructor */
    virtual ~ShapeGeometry();

    /** Get a list of all vertex points, non-unique*/
    const eastl::vector<aiVector3D>& GetVertices() const;

    /** Get a list of all vertex normals or an empty array if
    *  no normals are specified. */
    const eastl::vector<aiVector3D>& GetNormals() const;

    /** Return list of vertex indices. */
    const eastl::vector<unsigned int>& GetIndices() const;

private:
    eastl::vector<aiVector3D> m_vertices;
    eastl::vector<aiVector3D> m_normals;
    eastl::vector<unsigned int> m_indices;
};
/**
*  DOM class for FBX geometry of type "Line"
*/
class LineGeometry : public Geometry
{
public:
    /** The class constructor */
    LineGeometry(uint64_t id, const Element& element, const eastl::string& name, const Document& doc);

    /** The class destructor */
    virtual ~LineGeometry();

    /** Get a list of all vertex points, non-unique*/
    const eastl::vector<aiVector3D>& GetVertices() const;

    /** Return list of vertex indices. */
    const eastl::vector<int>& GetIndices() const;

private:
    eastl::vector<aiVector3D> m_vertices;
    eastl::vector<int> m_indices;
};

}
}

#endif // INCLUDED_AI_FBX_MESHGEOMETRY_H


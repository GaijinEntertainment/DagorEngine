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
#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_3MF_EXPORTER

#include "D3MFExporter.h"

#include <assimp/Exceptional.h>
#include <assimp/StringUtils.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

#include "3MFXmlTags.h"
#include "D3MFOpcPackage.h"

#ifdef ASSIMP_USE_HUNTER
#include <zip/zip.h>
#else
#include <contrib/zip/src/zip.h>
#endif

namespace Assimp {

void ExportScene3MF(const char *pFile, IOSystem *pIOSystem, const aiScene *pScene, const ExportProperties * /*pProperties*/) {
    if (nullptr == pIOSystem) {
        throw DeadlyExportError("Could not export 3MP archive: " + eastl::string(pFile));
    }
    D3MF::D3MFExporter myExporter(pFile, pScene);
    if (myExporter.validate()) {
        if (pIOSystem->Exists(pFile)) {
            if (!pIOSystem->DeleteFile(pFile)) {
                throw DeadlyExportError("File exists, cannot override : " + eastl::string(pFile));
            }
        }
        bool ok = myExporter.exportArchive(pFile);
        if (!ok) {
            throw DeadlyExportError("Could not export 3MP archive: " + eastl::string(pFile));
        }
    }
}

namespace D3MF {

D3MFExporter::D3MFExporter(const char *pFile, const aiScene *pScene) :
        mArchiveName(pFile), m_zipArchive(nullptr), mScene(pScene), mModelOutput(), mRelOutput(), mContentOutput(), mBuildItems(), mRelations() {
    // empty
}

D3MFExporter::~D3MFExporter() {
    for (size_t i = 0; i < mRelations.size(); ++i) {
        delete mRelations[i];
    }
    mRelations.clear();
}

bool D3MFExporter::validate() {
    if (mArchiveName.empty()) {
        return false;
    }

    if (nullptr == mScene) {
        return false;
    }

    return true;
}

bool D3MFExporter::exportArchive(const char *file) {
    bool ok(true);

    m_zipArchive = zip_open(file, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    if (nullptr == m_zipArchive) {
        return false;
    }

    ok |= exportContentTypes();
    ok |= export3DModel();
    ok |= exportRelations();

    zip_close(m_zipArchive);
    m_zipArchive = nullptr;

    return ok;
}

bool D3MFExporter::exportContentTypes() {
    mContentOutput.clear();

    mContentOutput << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    mContentOutput << eastl::endl;
    mContentOutput << "<Types xmlns = \"http://schemas.openxmlformats.org/package/2006/content-types\">";
    mContentOutput << eastl::endl;
    mContentOutput << "<Default Extension = \"rels\" ContentType = \"application/vnd.openxmlformats-package.relationships+xml\" />";
    mContentOutput << eastl::endl;
    mContentOutput << "<Default Extension = \"model\" ContentType = \"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\" />";
    mContentOutput << eastl::endl;
    mContentOutput << "</Types>";
    mContentOutput << eastl::endl;
    zipContentType(XmlTag::CONTENT_TYPES_ARCHIVE);

    return true;
}

bool D3MFExporter::exportRelations() {
    mRelOutput.clear();

    mRelOutput << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    mRelOutput << eastl::endl;
    mRelOutput << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";

    for (size_t i = 0; i < mRelations.size(); ++i) {
        if (mRelations[i]->target[0] == '/') {
            mRelOutput << "<Relationship Target=\"" << mRelations[i]->target << "\" ";
        } else {
            mRelOutput << "<Relationship Target=\"/" << mRelations[i]->target << "\" ";
        }
        mRelOutput << "Id=\"" << mRelations[i]->id << "\" ";
        mRelOutput << "Type=\"" << mRelations[i]->type << "\" />";
        mRelOutput << eastl::endl;
    }
    mRelOutput << "</Relationships>";
    mRelOutput << eastl::endl;

    zipRelInfo("_rels", ".rels");
    mRelOutput.flush();

    return true;
}

bool D3MFExporter::export3DModel() {
    mModelOutput.clear();

    writeHeader();
    mModelOutput << "<" << XmlTag::model << " " << XmlTag::model_unit << "=\"millimeter\""
                 << " xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
                 << eastl::endl;
    mModelOutput << "<" << XmlTag::resources << ">";
    mModelOutput << eastl::endl;

    writeMetaData();

    writeBaseMaterials();

    writeObjects();

    mModelOutput << "</" << XmlTag::resources << ">";
    mModelOutput << eastl::endl;
    writeBuild();

    mModelOutput << "</" << XmlTag::model << ">\n";

    OpcPackageRelationship *info = new OpcPackageRelationship;
    info->id = "rel0";
    info->target = "/3D/3DModel.model";
    info->type = XmlTag::PACKAGE_START_PART_RELATIONSHIP_TYPE;
    mRelations.push_back(info);

    zipModel("3D", "3DModel.model");
    mModelOutput.flush();

    return true;
}

void D3MFExporter::writeHeader() {
    mModelOutput << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    mModelOutput << eastl::endl;
}

void D3MFExporter::writeMetaData() {
    if (nullptr == mScene->mMetaData) {
        return;
    }

    const unsigned int numMetaEntries(mScene->mMetaData->mNumProperties);
    if (0 == numMetaEntries) {
        return;
    }

    const aiString *key = nullptr;
    const aiMetadataEntry *entry(nullptr);
    for (size_t i = 0; i < numMetaEntries; ++i) {
        mScene->mMetaData->Get(i, key, entry);
        eastl::string k(key->C_Str());
        aiString value;
        mScene->mMetaData->Get(k, value);
        mModelOutput << "<" << XmlTag::meta << " " << XmlTag::meta_name << "=\"" << key->C_Str() << "\">";
        mModelOutput << value.C_Str();
        mModelOutput << "</" << XmlTag::meta << ">" << eastl::endl;
    }
}

void D3MFExporter::writeBaseMaterials() {
    mModelOutput << "<basematerials id=\"1\">\n";
    eastl::string strName, std::hexDiffuseColor, tmp;
    for (size_t i = 0; i < mScene->mNumMaterials; ++i) {
        aiMaterial *mat = mScene->mMaterials[i];
        aiString name;
        if (mat->Get(AI_MATKEY_NAME, name) != aiReturn_SUCCESS) {
            strName = "basemat_" + ai_to_string(i);
        } else {
            strName = name.C_Str();
        }
        aiColor4D color;
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS) {
            std::hexDiffuseColor.clear();
            tmp.clear();
            // rgbs %
            if (color.r <= 1 && color.g <= 1 && color.b <= 1 && color.a <= 1) {

                std::hexDiffuseColor = ai_rgba2std::hex(
                        (int)((ai_real)color.r) * 255,
                        (int)((ai_real)color.g) * 255,
                        (int)((ai_real)color.b) * 255,
                        (int)((ai_real)color.a) * 255,
                        true);

            } else {
                std::hexDiffuseColor = "#";
                tmp = ai_decimal_to_std::hexa((ai_real)color.r);
                std::hexDiffuseColor += tmp;
                tmp = ai_decimal_to_std::hexa((ai_real)color.g);
                std::hexDiffuseColor += tmp;
                tmp = ai_decimal_to_std::hexa((ai_real)color.b);
                std::hexDiffuseColor += tmp;
                tmp = ai_decimal_to_std::hexa((ai_real)color.a);
                std::hexDiffuseColor += tmp;
            }
        } else {
            std::hexDiffuseColor = "#FFFFFFFF";
        }

        mModelOutput << "<base name=\"" + strName + "\" " + " displaycolor=\"" + std::hexDiffuseColor + "\" />\n";
    }
    mModelOutput << "</basematerials>\n";
}

void D3MFExporter::writeObjects() {
    if (nullptr == mScene->mRootNode) {
        return;
    }

    aiNode *root = mScene->mRootNode;
    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        aiNode *currentNode(root->mChildren[i]);
        if (nullptr == currentNode) {
            continue;
        }
        mModelOutput << "<" << XmlTag::object << " id=\"" << i + 2 << "\" type=\"model\">";
        mModelOutput << eastl::endl;
        for (unsigned int j = 0; j < currentNode->mNumMeshes; ++j) {
            aiMesh *currentMesh = mScene->mMeshes[currentNode->mMeshes[j]];
            if (nullptr == currentMesh) {
                continue;
            }
            writeMesh(currentMesh);
        }
        mBuildItems.push_back(i);

        mModelOutput << "</" << XmlTag::object << ">";
        mModelOutput << eastl::endl;
    }
}

void D3MFExporter::writeMesh(aiMesh *mesh) {
    if (nullptr == mesh) {
        return;
    }

    mModelOutput << "<"
                 << XmlTag::mesh
                 << ">" << "\n";
    mModelOutput << "<"
                 << XmlTag::vertices
                 << ">" << "\n";
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        writeVertex(mesh->mVertices[i]);
    }
    mModelOutput << "</"
                 << XmlTag::vertices << ">"
                 << "\n";

    const unsigned int matIdx(mesh->mMaterialIndex);

    writeFaces(mesh, matIdx);

    mModelOutput << "</"
                 << XmlTag::mesh << ">"
                 << "\n";
}

void D3MFExporter::writeVertex(const aiVector3D &pos) {
    mModelOutput << "<" << XmlTag::vertex << " x=\"" << pos.x << "\" y=\"" << pos.y << "\" z=\"" << pos.z << "\" />";
    mModelOutput << eastl::endl;
}

void D3MFExporter::writeFaces(aiMesh *mesh, unsigned int matIdx) {
    if (nullptr == mesh) {
        return;
    }

    if (!mesh->HasFaces()) {
        return;
    }
    mModelOutput << "<"
                 << XmlTag::triangles << ">"
                 << "\n";
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace &currentFace = mesh->mFaces[i];
        mModelOutput << "<" << XmlTag::triangle << " v1=\"" << currentFace.mIndices[0] << "\" v2=\""
                     << currentFace.mIndices[1] << "\" v3=\"" << currentFace.mIndices[2]
                     << "\" pid=\"1\" p1=\"" + ai_to_string(matIdx) + "\" />";
        mModelOutput << "\n";
    }
    mModelOutput << "</"
                 << XmlTag::triangles
                 << ">";
    mModelOutput << "\n";
}

void D3MFExporter::writeBuild() {
    mModelOutput << "<"
                 << XmlTag::build
                 << ">"
                 << "\n";

    for (size_t i = 0; i < mBuildItems.size(); ++i) {
        mModelOutput << "<" << XmlTag::item << " objectid=\"" << i + 2 << "\"/>";
        mModelOutput << "\n";
    }
    mModelOutput << "</" << XmlTag::build << ">";
    mModelOutput << "\n";
}

void D3MFExporter::zipContentType(const eastl::string &filename) {
    addFileInZip(filename, mContentOutput.str());
}

void D3MFExporter::zipModel(const eastl::string &folder, const eastl::string &modelName) {
    const eastl::string entry = folder + "/" + modelName;
    addFileInZip(entry, mModelOutput.str());
}

void D3MFExporter::zipRelInfo(const eastl::string &folder, const eastl::string &relName) {
    const eastl::string entry = folder + "/" + relName;
    addFileInZip(entry, mRelOutput.str());
}

void D3MFExporter::addFileInZip(const eastl::string& entry, const eastl::string& content) {
    if (nullptr == m_zipArchive) {
        throw DeadlyExportError("3MF-Export: Zip archive not valid, nullptr.");
    }

    zip_entry_open(m_zipArchive, entry.c_str());
    zip_entry_write(m_zipArchive, content.c_str(), content.size());
    zip_entry_close(m_zipArchive);
}

} // Namespace D3MF
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_3MF_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT

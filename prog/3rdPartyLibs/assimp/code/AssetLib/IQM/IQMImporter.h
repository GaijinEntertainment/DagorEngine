/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

/** @file IQMImporter.h
*   @brief Declares the importer class to read a scene from an Inter-Quake Model file
*/

#pragma once

#ifndef ASSIMP_BUILD_NO_IQM_IMPORTER

#include <assimp/BaseImporter.h>
#include <assimp/material.h>

namespace Assimp {

class IQMImporter : public BaseImporter {
public:
	/// \brief  Default constructor
	IQMImporter();
    ~IQMImporter() override = default;

    /// \brief  Returns whether the class can handle the format of the given file.
	/// \remark See BaseImporter::CanRead() for details.
	bool CanRead(const eastl::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

protected:
    //! \brief  Appends the supported extension.
    const aiImporterDesc *GetInfo() const override;

    //! \brief  File import implementation.
    void InternReadFile(const eastl::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;

private:
    aiScene *mScene = nullptr; // the scene to import to
};

} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_IQM_IMPORTER

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

/** @file  FBXParser.h
 *  @brief FBX parsing code
 */
#ifndef INCLUDED_AI_FBX_PARSER_H
#define INCLUDED_AI_FBX_PARSER_H

#include <stdint.h>
#include <EASTL/map.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <assimp/LogAux.h>
#include <assimp/fast_atof.h>

#include "FBXCompileConfig.h"
#include "FBXTokenizer.h"

namespace Assimp {
namespace FBX {

class Scope;
class Parser;
class Element;

// XXX should use C++11's unique_ptr - but assimp's need to keep working with 03
typedef eastl::vector< Scope* > ScopeList;
typedef eastl::fbx_unordered_multimap< eastl::string, Element* > ElementMap;

typedef eastl::pair<ElementMap::const_iterator,ElementMap::const_iterator> ElementCollection;

#   define new_Scope new Scope
#   define new_Element new Element


/** FBX data entity that consists of a key:value tuple.
 *
 *  Example:
 *  @verbatim
 *    AnimationCurve: 23, "AnimCurve::", "" {
 *        [..]
 *    }
 *  @endverbatim
 *
 *  As can be seen in this sample, elements can contain nested #Scope
 *  as their trailing member.  **/
class Element
{
public:
    Element(const Token& key_token, Parser& parser);
    ~Element() = default;

    const Scope* Compound() const {
        return compound.get();
    }

    const Token& KeyToken() const {
        return key_token;
    }

    const TokenList& Tokens() const {
        return tokens;
    }

private:
    const Token& key_token;
    TokenList tokens;
    eastl::unique_ptr<Scope> compound;
};

/** FBX data entity that consists of a 'scope', a collection
 *  of not necessarily unique #Element instances.
 *
 *  Example:
 *  @verbatim
 *    GlobalSettings:  {
 *        Version: 1000
 *        Properties70:
 *        [...]
 *    }
 *  @endverbatim  */
class Scope
{
public:
    Scope(Parser& parser, bool topLevel = false);
    ~Scope();

    const Element* operator[] (const eastl::string& index) const {
        ElementMap::const_iterator it = elements.find(index);
        return it == elements.end() ? nullptr : (*it).second;
    }

	const Element* FindElementCaseInsensitive(const eastl::string& elementName) const {
		const char* elementNameCStr = elementName.c_str();
		for (auto element = elements.begin(); element != elements.end(); ++element)
		{
			if (!ASSIMP_strincmp(element->first.c_str(), elementNameCStr, MAXLEN)) {
				return element->second;
			}
		}
        return nullptr;
	}

    ElementCollection GetCollection(const eastl::string& index) const {
        return elements.equal_range(index);
    }

    const ElementMap& Elements() const  {
        return elements;
    }

private:
    ElementMap elements;
};

/** FBX parsing class, takes a list of input tokens and generates a hierarchy
 *  of nested #Scope instances, representing the fbx DOM.*/
class Parser
{
public:
    /** Parse given a token list. Does not take ownership of the tokens -
     *  the objects must persist during the entire parser lifetime */
    Parser (const TokenList& tokens,bool is_binary);
    ~Parser() = default;

    const Scope& GetRootScope() const {
        return *root.get();
    }

    bool IsBinary() const {
        return is_binary;
    }

private:
    friend class Scope;
    friend class Element;

    TokenPtr AdvanceToNextToken();
    TokenPtr LastToken() const;
    TokenPtr CurrentToken() const;

private:
    const TokenList& tokens;

    TokenPtr last, current;
    TokenList::const_iterator cursor;
    eastl::unique_ptr<Scope> root;

    const bool is_binary;
};


/* token parsing - this happens when building the DOM out of the parse-tree*/
uint64_t ParseTokenAsID(const Token& t, const char*& err_out);
size_t ParseTokenAsDim(const Token& t, const char*& err_out);

float ParseTokenAsFloat(const Token& t, const char*& err_out);
int ParseTokenAsInt(const Token& t, const char*& err_out);
int64_t ParseTokenAsInt64(const Token& t, const char*& err_out);
eastl::string ParseTokenAsString(const Token& t, const char*& err_out);

/* wrapper around ParseTokenAsXXX() with DOMError handling */
uint64_t ParseTokenAsID(const Token& t);
size_t ParseTokenAsDim(const Token& t);
float ParseTokenAsFloat(const Token& t);
int ParseTokenAsInt(const Token& t);
int64_t ParseTokenAsInt64(const Token& t);
eastl::string ParseTokenAsString(const Token& t);

/* read data arrays */
void ParseVectorDataArray(eastl::vector<aiVector3D>& out, const Element& el);
void ParseVectorDataArray(eastl::vector<aiColor4D>& out, const Element& el);
void ParseVectorDataArray(eastl::vector<aiVector2D>& out, const Element& el);
void ParseVectorDataArray(eastl::vector<int>& out, const Element& el);
void ParseVectorDataArray(eastl::vector<float>& out, const Element& el);
void ParseVectorDataArray(eastl::vector<unsigned int>& out, const Element& el);
void ParseVectorDataArray(eastl::vector<uint64_t>& out, const Element& e);
void ParseVectorDataArray(eastl::vector<int64_t>& out, const Element& el);

bool HasElement( const Scope& sc, const eastl::string& index );

// extract a required element from a scope, abort if the element cannot be found
const Element &GetRequiredElement(const Scope &sc, const eastl::string &index, const Element *element = nullptr);

// extract required compound scope
const Scope& GetRequiredScope(const Element& el);
// get token at a particular index
const Token& GetRequiredToken(const Element& el, unsigned int index);

// read a 4x4 matrix from an array of 16 floats
aiMatrix4x4 ReadMatrix(const Element& element);

} // ! FBX
} // ! Assimp

#endif // ! INCLUDED_AI_FBX_PARSER_H

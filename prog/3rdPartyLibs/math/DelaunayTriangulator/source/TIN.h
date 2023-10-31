/****************************************************************************
Copyright 2017, Cognitics Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
THE SOFTWARE.
****************************************************************************/
/*!	\file ctl/TIN.h
\headerfile ctl/TIN.h
\brief Provides ctl::TIN.
\author Joshua Anghel
*/
#pragma once

#include "Subdivision.h"
#include "DelaunayTriangulation.h"


namespace ctl {

/*!	\class ctl::TIN ctl/TIN.h ctl/TIN.h
 *	\brief TIN
 *
 *	Provides a basic TIN data structure.
 *	Stores two Vector lists for points and their normals.
 *	Triangles are stored as a single IDList.
 *		Each 3 consecutive IDs in the list represents the locations of a single triangle's Vertex points inside
 *		the verts and normals lists.
 *	Triangles are stored CCW.
 *	
 *	Triangles can be created using the following code:
 *	
 *	\code
 *	for (int i = 0; i < int(tin.triangles.size()/3); i++)
 *	{
 *		Point a = tin.verts[tin.triangles[i*3 + 0]];
 *		Point b = tin.verts[tin.triangles[i*3 + 1]];
 *		Point c = tin.verts[tin.trianlges[i*3 + 2]];
 *
 *	//	Normals are retrieved the same way, but call tin.normals[] instead of tin.verts[]
 *	//	..insert triangle handing code here
 *	}
 *	\endcode
 */
	class TIN
	{
	public:
		PointList				verts;
		std::vector<Vector>		normals;
		IDList					triangles;

		TIN(void) { }

		TIN(DelaunayTriangulation* DT);

		TIN(DelaunayTriangulation* DT, const std::vector<Edge*> &triangle_edges);

		~TIN(void) { }
		void CreateFromDT(DelaunayTriangulation* DT, const std::vector<Edge*> &triangle_edges);

	};

}
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
#include "TIN.h"
#include "Util.h"
#include <cmath>
#include <map>
#include <set>

using std::abs;

namespace ctl {

	struct TriangleKey {
		ID a;
		ID b;
		ID c;

		TriangleKey(void)
		{
			a = 0;
			b = 0;
			c = 0;
		}

		TriangleKey(ID id1, ID id2, ID id3)
		{
			a = id1;
			b = id2;
			c = id3;

			if (a < b)
				std::swap(a, b);
			if (b < c)
				std::swap(b, c);
			if (a < b)
				std::swap(a, b);
		}

		bool operator<(const TriangleKey& other) const
		{
			if (a < other.a) 
				return true;
			else if (a > other.a) 
				return false;
			else if (b < other.b) 
				return true;
			else if (b > other.b) 
				return false;
			else if (c < other.c) 
				return true;
			else if (c > other.c) 
				return false;
			else
				return false;
		}
	};

	TIN::TIN(DelaunayTriangulation* DT)
	{
		CreateFromDT(DT,DT->GatherTriangles(PointList()));
	}

	TIN::TIN(DelaunayTriangulation* DT, const std::vector<Edge*> &triangle_edges)
	{
		CreateFromDT(DT,triangle_edges);
	}

	void TIN::CreateFromDT(DelaunayTriangulation* DT, const std::vector<Edge*> &triangle_edges)
	{
		const Subdivision* graph = DT->GetSubdivision();
		verts.resize(graph->getMaxVerts());
		normals.resize(graph->getMaxVerts());

		ID invalid = graph->getMaxEdges();
		IDList VertexIDMap(invalid,invalid);

		int nextIndex = 0;
		for (unsigned int i = 0 ; i < triangle_edges.size(); i++)
		{
		//For each vertex test if its already included (invalid means its not)
			Vertex* a = triangle_edges[i]->Org();
			Vertex* b = triangle_edges[i]->Dest();
			Vertex* c = triangle_edges[i]->Onext()->Dest();

			if ( VertexIDMap[a->getID()] == invalid )
			{
				verts[nextIndex] = DT->TransformPointToGlobal(a->point);
				normals[nextIndex] = DT->GetVertexNormal(a);
				VertexIDMap[a->getID()] = nextIndex;
				nextIndex++;
			}

			if ( VertexIDMap[b->getID()] == invalid )
			{
				verts[nextIndex] = DT->TransformPointToGlobal(b->point);
				normals[nextIndex] = DT->GetVertexNormal(b);
				VertexIDMap[b->getID()] = nextIndex;
				nextIndex++;
			}			
			
			if ( VertexIDMap[c->getID()] == invalid )
			{
				verts[nextIndex] = DT->TransformPointToGlobal(c->point);
				normals[nextIndex] = DT->GetVertexNormal(c);
				VertexIDMap[c->getID()] = nextIndex;
				nextIndex++;
			}
		}

	//	Resize Verts and normals array back to true size
		verts.resize(nextIndex);
		normals.resize(nextIndex);

		std::set<Edge*> processedEdges;

	//	Add triangles
		for (unsigned int i = 0; i < triangle_edges.size(); i++)
		{
			Edge* next = triangle_edges[i];

			if (processedEdges.count(next) == 0)
			{
				processedEdges.insert(next);
				processedEdges.insert(next->Lnext());
				processedEdges.insert(next->Lprev());

				Vertex* a = next->Org();
				Vertex* b = next->Lnext()->Org();
				Vertex* c = next->Lprev()->Org();

				if (abs(TArea2D(a->point, b->point, c->point)) > DT->GetAreaEpsilon())
				{
					triangles.push_back(VertexIDMap[ a->getID() ]);
					triangles.push_back(VertexIDMap[ b->getID() ]);
					triangles.push_back(VertexIDMap[ c->getID() ]);
				}
			}
		}
	}
}
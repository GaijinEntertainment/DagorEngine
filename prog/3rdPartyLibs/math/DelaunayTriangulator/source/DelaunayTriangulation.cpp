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

//#pragma optimize( "", off )
#include <float.h>
#include <math.h>
#include "DelaunayTriangulation.h"
#include "Vertex.h"
#include "Edge.h"
#include "Util.h"

#include <algorithm>
#include <typeinfo>
#include <map>
#include <list>
#include <set>
#include <unordered_set>

#undef min
#undef max

int GlobalCTLDebugEnabled = 1;

//#define CTL_DEBUG
#ifdef CTL_DEBUG
#include <ctlv/ctlViewer.h>
#endif

#define INIT_CORRUPT_TEST\
	int _CORRUPT_TEST_counter = 0;\
	int _CORRUPT_TEST_max_counter = subdivision_->getNumEdges()*32;	

#define RUN_CORRUPT_TEST\
	_CORRUPT_TEST_counter++;\
	if (_CORRUPT_TEST_counter > _CORRUPT_TEST_max_counter) {\
		error_ |= CORRUPTION_DETECTED;\
		break;\
	}

namespace ctl {


/******************************************************************************************************
	INITIALIZATION
*******************************************************************************************************/

	DelaunayTriangulation::DelaunayTriangulation
	(
		PointList			boundary,
		int					resizeIncriment,
		double				epsilon,
		double				areaEpsilon,
		int					maxEdgeFlips,
		int					settings
	)
	{
		epsilon_		= epsilon;
		areaEpsilon_	= areaEpsilon;
		maxEdgeFlips_	= maxEdgeFlips;
		settings_		= settings;
		subdivision_	= new Subdivision(resizeIncriment < 10 ? 10 : resizeIncriment);
		error_			= 0;

	// transform to local coordinates
		origin_ = boundary.size()>0 ? boundary[0] : ctl::Point(0,0,0);
		for (size_t i = 0; i < boundary.size(); i++)
			boundary[i] = TransformPointToLocal(boundary[i]);

		bsp = new DelaunayBSP(boundary);


	//	remove duplicate points
		boundary.reserve(boundary.size());
		for (size_t i = 0; i < boundary.size(); i++)
		{
			if (boundary_.empty())
				boundary_.push_back(boundary[i]);
			else if ((boundary_.front() - boundary[i]).length2D() <= epsilon)
				continue;
			else if ((boundary_.back() - boundary[i]).length2D() <= epsilon)
				continue;
			else
				boundary_.push_back(boundary[i]);
		}

	//	check for valid boundary
		if (boundary_.size() < 3)
		{
			//ccl::ObjLog log;
			//log << ccl::LERR << "(DelaunayTriangulation) Boundary has too few unique points." << log.endl;
			error_ = INVALID_BOUNDARY;
			return;
		}
		double area = PArea2D(boundary_);
		if (area > 0)
			std::reverse(boundary_.begin(), boundary_.end());
		else
			area = -area;
		if (area < areaEpsilon_)
		{
			//ccl::ObjLog log;
			//log << ccl::LERR << "(DelaunayTriangulation) Boundary has 0 area." << log.endl;
			error_ = INVALID_BOUNDARY;
			return;
		}
		for (int curr = 0, num = int(boundary_.size()); curr < num; curr++)
		{
			int prev = (curr - 1 + num) % num;
			int next = (curr + 1) % num;

			if (IsLeft(boundary_[curr], boundary_[prev], boundary_[next]))
			{
				//ccl::ObjLog log;
				//log << ccl::LERR << "(DelaunayTriangulation) Boundary must be convex." << log.endl;
				error_ = INVALID_BOUNDARY;
				return;
			}
		}

#ifdef CTL_DEBUG
		if (GlobalCTLDebugEnabled)
		{
			double minx = 0, maxx = 0, miny = 0, maxy = 0;
			for (size_t i = 0; i < boundary.size(); i++)
			{
				const ctl::Point& p = boundary[i];
				if (p.x < minx) minx = p.x;
				if (p.x > maxx) maxx = p.x;
				if (p.y < miny) miny = p.y;
				if (p.y > maxy) maxy = p.y;
			}
			glViewer::Initialize();
			glViewer::glVector target((minx + maxx) / 2, (miny + maxy) / 2);
			glViewer::glVector position = glViewer::glVector(0, 0, maxx / 2);
			glViewer::setWindow(minx, maxx, miny, maxy);
		}
#endif

		neighbors_.resize(boundary_.size(), NULL);

	//	Reserve constraint ID of 0 for boundary
		cmap_.GetNextConstraintID();

		size_t n = boundary_.size();

		std::vector<Vertex*> verts;
		for (size_t i = 0; i < n; i++)
		{
			Vertex* vert = subdivision_->CreateVertex(boundary_[i]);
			verts.push_back(vert);
		}

		// save the boundary verts so we can remove them later
		boundary_verts = verts;
		
		std::vector<Edge*> edges;
		for (size_t i = 0; i < n; i++)
			edges.push_back(subdivision_->CreateEdge(verts[i], verts[(i+1)%n]));

		for (size_t i = 0; i < n; i++)
		{
			cmap_.BindEdge(edges[i],0);
			subdivision_->Splice(edges[i]->Sym(), edges[(i+1)%n]);
		}

	//	Connect edges
		for (size_t i = 1; i < (n-2); i++)
			subdivision_->Connect(edges[i],edges[0]);

	//	Flip edges
		for (size_t i = 1; i < (n-1); i++)
			FlipEdges(boundary[0], edges[i]);
	}

	DelaunayTriangulation::~DelaunayTriangulation(void)
	{
		delete bsp;
		delete subdivision_;
		for (size_t i = 0, n = neighbors_.size(); i < n; i++)
			UnlinkNeighbor(i);
	}

	int DelaunayTriangulation::error(void) const {
		return error_; 
	}

	void DelaunayTriangulation::removeBoundaryVerts(void)
	{
		for(size_t i = 0, c = boundary_verts.size(); i < c; ++i)
			RemoveVertex(boundary_verts[i]);
	}

/******************************************************************************************************
	BOUNDARY QUERIES
*******************************************************************************************************/
	static void GetBoundingRect(const PointList& polygon, Point& minPoint, Point& maxPoint)
	{
		double minX = DBL_MAX;
		double maxX = -DBL_MAX;
		double minY = DBL_MAX;
		double maxY = -DBL_MAX;
		for(auto &point: polygon)
		{
			minX = std::min(minX, point.x);
			maxX = std::max(maxX, point.x);
			minY = std::min(minY, point.y);
			maxY = std::max(maxY, point.y);
		}
		minPoint = Point(minX, minY);
		maxPoint = Point(maxX, maxY);
	}

	Point DelaunayTriangulation::GetLowerBound(void)
	{
		double x = DBL_MAX;
		double y = DBL_MAX;
		double z = DBL_MAX;
		for (size_t i = 1; i < boundary_.size(); i++)
		{
			x = std::min<double>(boundary_[i].x, x);
			y = std::min<double>(boundary_[i].y, y);
			z = std::min<double>(boundary_[i].z, z);
		}
		return TransformPointToGlobal(Point(x, y, z));
	}

	Point DelaunayTriangulation::GetUpperBound(void)
	{
		double x = -DBL_MAX;
		double y = -DBL_MAX;
		double z = -DBL_MAX;
		for (size_t i = 1; i < boundary_.size(); i++)
		{
			x = std::max<double>(boundary_[i].x, x);
			y = std::max<double>(boundary_[i].y, y);
			z = std::max<double>(boundary_[i].z, z);
		}
		return TransformPointToGlobal(Point(x, y, z));
	}

	PointList DelaunayTriangulation::GetBoundary(void)
	{
		PointList temp;
		for (size_t i = 0; i < boundary_.size(); i++)
			temp.push_back(TransformPointToGlobal(boundary_[i]));
		return temp;
	}

/******************************************************************************************************
	SETTINGS CONTROL
*******************************************************************************************************/

	double DelaunayTriangulation::GetEpsilon(void) const {
		return epsilon_;
	}

	void DelaunayTriangulation::SetEpsilon(double newEpsilon) {
		epsilon_ = newEpsilon;
	}

	double DelaunayTriangulation::GetAreaEpsilon(void) const {
		return areaEpsilon_;
	}

	void DelaunayTriangulation::SetAreaEpsilon(double newAreaEpsilon) {
		areaEpsilon_ = newAreaEpsilon;
	}

	int DelaunayTriangulation::GetMaxEdgeFlips(void) const {
		return maxEdgeFlips_;
	}

	void DelaunayTriangulation::SetMaxEdgeFlips(int maxEdgeFlips) {
		maxEdgeFlips_ = maxEdgeFlips;
	}

	bool DelaunayTriangulation::Enabled(Option option) const { 
		return (settings_ & option) == option;
	}

	void DelaunayTriangulation::Enable(Option option) {

#ifdef CTL_DEBUG 
		if (option == VISUAL_DEBUG && GlobalCTLDebugEnabled)
		{
			glViewer::Clear();
			glViewer::AddCDT(this);
			glViewer::Display();
		}
#endif
		settings_ |= option;
	}

	void DelaunayTriangulation::Disable(Option option) {
		settings_ &= ~option;
	}

/******************************************************************************************************
	SIZE QUERIES
*******************************************************************************************************/

	int DelaunayTriangulation::GetNumEdges(void) {
		return subdivision_->getNumEdges();
	}

	int DelaunayTriangulation::GetNumConstrainedEdges(void)
	{
		int count = 0;

		for (int i = 0; i < subdivision_->getMaxEdges(); i++)
			if (cmap_.IsEdgeBound(subdivision_->getEdge(i))) count++;

		return count;
	}
	
	int DelaunayTriangulation::GetNumVertices(void)
	{
		return subdivision_->getNumVerts();
	}

	int DelaunayTriangulation::GetNumConstrainedVertices(void)
	{
		int count = 0;

		for (int i = 0; i < subdivision_->getMaxVerts(); i++)
			if (cmap_.IsVertexBound(subdivision_->getVertex(i))) count++;

		return count;
	}

	int DelaunayTriangulation::GetNumTriangles(bool rough)
	{
		int base = 2*GetNumEdges();
		if (!rough) // extract excess boundary triangles
		{
			for (int i = 0; i < subdivision_->getMaxEdges(); i++)
			{
				Edge* e = subdivision_->getEdge(i);
				if (e && cmap_.IsEdgeBound(e, 0))
					base--;
			}
		}
		return base/3;
	}

/******************************************************************************************************
	POINT QUERIES
*******************************************************************************************************/

	Point DelaunayTriangulation::GetNearestPoint(Point point)
	{
	//	Out of bounds
		if (!IsInside(point)) 
			return point;

		Point nearest = point;

		LocationResult loc = LocatePoint(point);
		if (loc.getType() == LR_VERTEX)
			nearest = loc.getEdge()->Org()->point;
		else if (loc.getType() == LR_EDGE)
		{
		//	Find which end of the edge the point is closest to.
			Point a = loc.getEdge()->Org()->point;
			Point b = loc.getEdge()->Dest()->point;
			double dist = (point-a).dot(b-a)/(b-a).length2D();
			nearest = (dist < 0.5 ? a : b);
		}
		else if (loc.getType() == LR_FACE)
		{
			double d = (point - loc.getEdge()->Org()->point).length2D();
			double d1 = (point - loc.getEdge()->Dest()->point).length2D();
			double d2 = (point - loc.getEdge()->Onext()->Dest()->point).length2D();

			nearest = loc.getEdge()->Org()->point;
			if (d < d1) nearest = loc.getEdge()->Dest()->point;
			if (d < d2) nearest = loc.getEdge()->Onext()->Dest()->point;
		}

		return nearest;
	}

	double DelaunayTriangulation::GetZValue(Point point, double outsideZ)
	{
		point = TransformPointToLocal(point);

	//	Out of bounds
		if (!IsInside(point)) 
			return outsideZ;

		double z = 0;

		Vertex* vert = SnapPointToVertex(point);
		if (vert)
			return vert->point.z + origin_.z;

		LocationResult loc = LocatePoint(point);
		if (loc.getType() == LR_VERTEX)
			z = loc.getEdge()->Org()->point.z;
		else if (loc.getType() == LR_EDGE)
		{
		//	Interpolate
			Point a = loc.getEdge()->Org()->point;
			Point b = loc.getEdge()->Dest()->point;
			double dist = (point-a).dot(b-a)/(b-a).length2D();
			z = a.z*(1 - dist) + b.z;
		}
		else if (loc.getType() == LR_FACE)
		{
			Vector O = loc.getEdge()->Org()->point;
			Vector U = loc.getEdge()->Dest()->point - O;
			Vector V = loc.getEdge()->Onext()->Dest()->point - O;

			Point P = point - O;
			double denom = V.x*U.y - V.y*U.x;
			double v = ( P.x*U.y - P.y*U.x )/denom;
			double u = ( P.y*V.x - P.x*V.y )/denom;
			z = O.z + u*U.z + v*V.z;

		}

		return z + origin_.z;	
	}

/******************************************************************************************************
	INSERTION AND REMOVAL
*******************************************************************************************************/
	
	void DelaunayTriangulation::InsertWorkingPoint(Point point)
	{
		if (error_)
			return;

		point = TransformPointToLocal(point);
		if (IsInside(point)) 
			InsertPoint(point);
	}

	void DelaunayTriangulation::RemoveWorkingPoints(const PointList &polygon)
	{
		if (error_)
			return;

		std::vector<Vertex*> verts = GatherWorkingVerts(polygon);
		for (size_t i = 0; i < verts.size(); i++)
			RemoveVertex(verts[i]);
	}

	ID DelaunayTriangulation::InsertConstrainedPoint(Point point)
	{
		if (error_)
			return 0;

	//	Transform
		point = TransformPointToLocal(point);

		if (!IsInside(point)) 
			return 0;

		Vertex* vert = InsertPoint(point);
		if (error_) return 0;

		ID constraintID = cmap_.GetNextConstraintID();
		cmap_.BindVertex(constraintID,vert);
		return constraintID;
	}

	ID DelaunayTriangulation::InsertConstrainedLineString(PointList constraint)
	{
		if (error_)
			return 0;

	//	Transform
		for (size_t i = 0; i < constraint.size(); i++)
			constraint[i] = TransformPointToLocal(constraint[i]);

	//	Clip
		if (!IsInside(constraint))
		{
			if (Enabled(CLIPPING))
				constraint = ClipToPolygon(constraint,boundary_,0);
			else
				return 0;
		}

		if (constraint.size() < 2) 
			return 0;
		
		return InsertConstrainedLineStringPrivate(constraint);
	}

	ID DelaunayTriangulation::InsertConstrainedLineStringPrivate(const PointList& constraint)
	{
#ifdef CTL_DEBUG
		if (GlobalCTLDebugEnabled && this->Enabled(VISUAL_DEBUG)){
			glViewer::Clear();
			glViewer::AddText("InsertConstraint-constraint",0,0,0);
			glViewer::AddPointList(constraint);
			glViewer::AddCDT(this);
			glViewer::Display();
		}
#endif
	// Insert constraint points
		std::vector<Vertex*> verts;
		verts.reserve(constraint.size());
		for (unsigned int i = 0; i < constraint.size(); i++)
		{
			Vertex* vert = SnapPointToVertex(constraint[i]);
			if (!vert)
				vert = SnapPointToEdge(constraint[i]);
			if (!vert)
				vert = InsertPoint(constraint[i]);
#ifdef CTL_DEBUG
			if (GlobalCTLDebugEnabled && this->Enabled(VISUAL_DEBUG)){
			glViewer::Clear();
			glViewer::AddText("InsertConstraint-points",0,0,0);
			glViewer::AddPoint(vert->point);
			glViewer::AddCDT(this);
			glViewer::Display();
			}

			if (GlobalCTLDebugEnabled && (error_ || !vert))
			{
				glViewer::Clear();
				glViewer::AddText("Error inserting-points",0,0,0);
				glViewer::AddPoint(vert->point);
				glViewer::AddCDT(this);
				glViewer::Display();

			}
#endif
			if (error_) return 0;
			if (vert)
				verts.push_back(vert);
		}

		ID constraintID = cmap_.GetNextConstraintID();
		cmap_.BindVertex(constraintID,verts[0]);

	//	Insert Constrained Edges
		for (int i = 1; i < int(verts.size()); i++)
		{
			InsertSegment(verts[i-1],verts[i],constraintID);
			if (error_) return 0;
		}

		return error_ ? 0 : constraintID;
	}

	ID DelaunayTriangulation::InsertConstrainedPolygon(PointList constraint)
	{
		if (error_)
			return 0;

	//	Transform
		for (size_t i = 0; i < constraint.size(); i++)
			constraint[i] = TransformPointToLocal(constraint[i]);

	//	Clip
		if (!IsInside(constraint))
		{
			if (Enabled(CLIPPING))
			{
				constraint = ClipToPolygon(constraint,boundary_,0);
			}
			else
				return 0;
		}

	//	Collapsed constraint
		if (constraint.size() < 2) 
			return 0;

	//	Check for collapsed polygon
		double area = PArea2D(constraint);
		if (abs(area) < areaEpsilon_)
			return 0;
		if (area < 0)
			std::reverse(constraint.begin(), constraint.end());

		return InsertConstrainedPolygonPrivate(constraint);
	}

	ID DelaunayTriangulation::InsertConstrainedPolygonPrivate(const PointList& constraint)
	{
	//	Insert constraint like normal
		ID constraintID = InsertConstrainedLineStringPrivate(constraint);
		
		if (error_)
			return 0;

		//   Gather Verts inside this constraint. The working vertices are simplified
		//   There are two ways to define the Z of the constrained vertices
		//      -set the polygon vertices to the Z of the grid. This is done if
		//        INTERPOLATE_EDGES and INTERPOLATE_FACES are enabled
		//      -set all the vertices to the Z of the polygon. If 
		//       INTERPOLATE_EDGES and INTERPOLATE_FACES are disabled
		bool usePolygonZ = !Enabled(INTERPOLATE_EDGES);
		std::vector<Vertex*> verts = GatherVertsLocal(constraint, usePolygonZ, true);

		if (error_)
			return 0;

		if (usePolygonZ)
		{
	// Find basis of the polygon list
		Point O = constraint[0];
		Vector U = constraint[1] - constraint[0];
		Vector V;

		for (unsigned int i = 2; i < constraint.size(); i++)
		{
			V = constraint[i] - constraint[0];
			if (std::abs(U.cross(V).z) > areaEpsilon_)
				break;
		}

		for (unsigned int i = 0; i < verts.size(); i++)
				InterpolateZ(O, U, V, verts[i]);
		}
		// Remove unconstrained vertices inside the polygon
		// AFTER Z has been set in case the simplification needs Z
		for (unsigned int i = 0; i < verts.size(); i++)
		{ 
			SimplifyEdges(verts[i]);
			if (error_) return 0;
		}

		return constraintID;
	}
	
	void DelaunayTriangulation::RemoveConstraint(ID constraintID)
	{
		if (error_)
			return;

	//	Do NOT allow removal of boundary
		if (constraintID == 0) 
			return;

	//	Get starting Edge of constraint
		Vertex* vert = subdivision_->getVertex(cmap_.GetBoundVertex(constraintID));
		if (!vert) return;
		Edge* edge = vert->getEdges();
		if (!edge) return;

	//	Unbind this Vertex from the Constraint (this must be done now so the first point can be removed
		cmap_.FreeVertex(constraintID);

		Edge* start = edge->Oprev();
		int LOOP_LIMIT_i = 0;
		int LOOP_LIMIT_n = subdivision_->getNumEdges()*32;
		while (!cmap_.IsEdgeBound(edge,constraintID) && edge != start) 
		{
			if (LOOP_LIMIT_i++ > LOOP_LIMIT_n)
			{
#ifdef CTL_DEBUG
				if (GlobalCTLDebugEnabled)
				{
					glViewer::Clear();
					glViewer::AddCDT(this);
					glViewer::Display();
				}
#endif
				//ccl::ObjLog log;
				//log << ccl::LERR << "(DelaunayTriangulation) Vertex Edge list has become corrupt." << log.endl;
				error_ = CORRUPTION_DETECTED;
				return;
			}
			edge = edge->Onext();
		}

		std::set<Vertex*> verts;

		if (!cmap_.IsEdgeBound(edge,constraintID))
			verts.insert(edge->Org());
		else
		{
		//	Gather Edges of Constraint
			bool *marked = new bool[subdivision_->getMaxEdges()]();
			std::vector<Edge*> edges;
			GatherConstraintEdges(edge,edges,marked,constraintID);
			delete [] marked;

		//	Check for corruption
			if (error_) return;

		//	Remove constraintID from all edges
			for (std::vector<Edge*>::iterator it = edges.begin(); it != edges.end(); ++ it)
				cmap_.FreeEdge(*it,constraintID);

		//	gather all verts of this constraint

			for (std::vector<Edge*>::iterator it = edges.begin(); it != edges.end(); ++it)
			{
				verts.insert((*it)->Org());
				verts.insert((*it)->Dest());
			}
		}

		for (std::set<Vertex*>::iterator it = verts.begin(); it != verts.end(); ++it)
		{
		/*	Constraints may have caused non-delaunay sections.
			In order to eliminate retriangulation errors, FlipEdges now
			that they are no longer constrained, then simplify the remaining
			Edges.
		*/
			FlipEdges((*it)->point,(*it)->getEdges()->Sym());
			SimplifyEdges(*it);
		}
	}

/******************************************************************************************************
	DATA STRUCTURE EXPORTING
*******************************************************************************************************/

	const ConstraintMap* DelaunayTriangulation::GetConstraintMap(void) const
	{
		return &cmap_;
	}

	const Subdivision* DelaunayTriangulation::GetSubdivision(void) const
	{
		return subdivision_;
	}

/******************************************************************************************************
	NEIGHBOOR OPERATIONS
*******************************************************************************************************/

	bool DelaunayTriangulation::LinkNeighbor(size_t location, DelaunayTriangulation* dt, size_t dt_location)
	{
	//	Check if boundaries line up appropriately
		Vertex* b0[2];
		Vertex* b1[2];

		if ( ! (boundary_[location].equals(dt->boundary_[(dt_location+1)%dt->boundary_.size()],epsilon_) &&
			    boundary_[(location+1)%boundary_.size()].equals(dt->boundary_[dt_location],epsilon_)) )
				return false;

		b0[0] = InsertPoint(boundary_[(location+1)%boundary_.size()]);
		b0[1] = InsertPoint(boundary_[location]);
		b1[0] = dt->InsertPoint(dt->boundary_[dt_location]);
		b1[1] = dt->InsertPoint(dt->boundary_[(dt_location+1)%dt->boundary_.size()]);

	//	Now check if the boundaries line up by checking the vertices along the boundary using Edge Traversal
		Edge *b0_e, *b1_e;

		b0_e = b0[0]->getEdges();
		b1_e = b1[0]->getEdges();

		while (IsOn(b0_e->Dest()->point,b0[0]->point,b0[1]->point) == -1)
			b0_e = b0_e->Onext();

		while (IsOn(b1_e->Dest()->point,b1[0]->point,b1[1]->point) == -1)
			b1_e = b1_e->Onext();

		while (b0_e->Dest() != b0[1] && b1_e->Dest() != b1[1])
		{
			if (b0_e->Dest()->point.equals(b1_e->Dest()->point,epsilon_))
			{
				Edge* b0_next = b0_e->Sym()->Onext();
				Edge* b1_next = b1_e->Sym()->Onext();

				while (IsOn(b0_next->Dest()->point,b0_e) == -1 && b0_next != b0_e->Sym())
					b0_next = b0_next->Onext();

				while (IsOn(b1_next->Dest()->point,b1_e) == -1 && b1_next != b1_e->Sym())
					b1_next = b1_next->Onext();

				if (b0_next == b0_e->Sym() || b1_next == b1_e->Sym()) 
					return false;
				else
				{
					b0_e = b0_next;
					b1_e = b1_next;
				}
			}
			else 
				return false;
		}

		neighbors_[location] = dt;
		dt->neighbors_[dt_location] = this;

		return true;
	}

	void DelaunayTriangulation::UnlinkNeighbor(size_t location)
	{
		if (neighbors_[location])
		{
			for (size_t i = 0; i < neighbors_[location]->neighbors_.size(); i++)
			{
				if (neighbors_[location]->neighbors_[i] == this)
					neighbors_[location]->neighbors_[i] = NULL;
			}
		}
		neighbors_[location] = NULL;
	}

	bool DelaunayTriangulation::IsBoundaryVertex(Vertex* vert)
	{
		for (size_t i = 0, n = boundary_.size(); i < n; i++)
		{
			if (IsOn(vert->point, boundary_[i], boundary_[(i+1)%n]) != -1) 
				return true;
		}

		return false;
	}

	bool DelaunayTriangulation::IsBoundaryVertex(Vertex* vert, size_t location)
	{
		return IsOn(vert->point, boundary_[(location+1)%boundary_.size()], boundary_[location]) != -1;
	}
	
	bool DelaunayTriangulation::IsBoundaryEdge(Edge* edge)
	{
		return cmap_.IsEdgeBound(edge,0);
	}

	Vector DelaunayTriangulation::GetVertexNormal(Vertex* vert)
	{
		Vector normal = vert->getAreaWeightedNormal();
/*
	//	Use any nearby triangulations data as well
		for (int i = 0; i < 4; i++)
		{
			if (neighbors[i])
			{
				if (IsBoundaryVertex(vert,Neighbor(i)))
				{
					Vertex* other = neighbors[i]->InsertPoint(vert->point);
					Vector nextNormal = other->getAreaWeightedNormal();
					normal = nextNormal + normal;
				}
			}
		}
*/
		return normal.normalize();
	}

	bool DelaunayTriangulation::IsBoundaryEdge(Edge* edge, size_t location)
	{
		return IsBoundaryVertex(edge->Org(),location) && IsBoundaryVertex(edge->Dest(),location);
	}

/******************************************************************************************************
	POINT AND TRIANGLE GATHERING ALGORITHMS
*******************************************************************************************************/

//!	Recursively traverse the graph to gather all connected constrained edges
	void DelaunayTriangulation::GatherConstraintEdges(Edge* edge, std::vector<Edge*>& edges, bool* marked, ID constraintID)
	{
		Edge* start = edge;
		do 
		{
			if (cmap_.IsEdgeBound(edge,constraintID))
			{
				if (!marked[edge->getID()])
				{
					marked[edge->getID()] = true;
					edges.push_back(edge);
					GatherConstraintEdges(edge->Sym(),edges,marked,constraintID);
				}
			}
			edge = edge->Onext();
		}
		while (edge != start);
	}

//!    Gather all edges that cross the line between a and b
	std::vector<Edge*> DelaunayTriangulation::GetCrossingEdges(Vertex* a, Vertex* b, bool bound, bool unbound)
	{
		std::vector<Edge*> edges;

		bool diverged = false;
		int iteration = 0;
		int iteration_limit = GetNumEdges();

		auto EdgeNeeded = [&](Edge* e)-> bool
		{
			return e != nullptr &&
				   ((bound && unbound) ||
					(bound && cmap_.IsEdgeBound(e)) ||
					(unbound && !cmap_.IsEdgeBound(e)));
		};

		Point alpha = a->point;
		Point beta = b->point;
		Edge* e = GetLeftEdge(a,beta);
		while(e!=nullptr && e->Dest() != b)
		{
			if (iteration++ > iteration_limit)
			{
				diverged = true;
				break;
			}

		// Search
			PointLineLocation loc = ctl::LocatePointOnLine(e->Dest()->point,alpha,beta, epsilon_);
			if(loc==PL_LEFT_OF_LINE)
				e = e->Rprev();
			else if(loc==PL_RIGHT_OF_LINE)
			{
				if(EdgeNeeded(e))
					 edges.push_back(e);
				e = e->Onext();
			}
			else 
			{
				if(IsRight(e->Lnext()->Dest()->point, alpha, beta))
					e = e->Onext();
				else
					e = e->Lnext();
			}
		}

		// smart search diverged, revert to brute force
		if (diverged)
		{
			//ccl::ObjLog log;
			//log << "DelaunayTriangulation::GetCrossingEdges() Smart search failed, reverting to brute force." << log.endl;

			edges.clear();
			Point ba = beta - alpha;
			for (int i = 0; i < subdivision_->getMaxEdges(); i++)
			{
				Edge* edge = subdivision_->getEdge(i);
				if (EdgeNeeded(edge))
				{
					// If the edge vertices fall on the line, it does not cross the line
					if (IsOn(edge->Org()->point, alpha, beta)!=-1 || IsOn(edge->Dest()->point, alpha, beta)!=-1)
						continue;
					Point dc = edge->Dest()->point - edge->Org()->point;
					float denom = float(dc.x*ba.y - ba.x*dc.y);
					if (denom)
					{
						Point ca = edge->Org()->point - alpha;
						float s = float(dc.x*ca.y - ca.x*dc.y) / denom;
						if (s < 0 || s > 1)
							continue;
						float t = float(ba.x*ca.y - ca.x*ba.y) / denom;
						if (t < 0 || t > 1)
							continue;
						edges.push_back(edge);
					}
				}
			}
		}

		return edges;
	}

//!	Gather all vertices that are on the line between a and b
	std::vector<Vertex*> DelaunayTriangulation::GetTouchingVerts(Vertex* a, Vertex* b)
	{
		std::vector<Vertex*> verts;
		
		verts.push_back(a);
		
		bool diverged = false;
		int iteration = 0;
		int iteration_limit = GetNumEdges();

		Point alpha = a->point;
		Point beta = b->point;
		Edge* e = GetLeftEdge(a,b->point);
		while(e->Dest() != b)
		{
			if (iteration++ > iteration_limit)
			{
				diverged = true;
				break;
			}

		// Search
			PointLineLocation loc = ctl::LocatePointOnLine(e->Dest()->point,alpha,beta, epsilon_);
			if (loc==PL_LEFT_OF_LINE)
				e = e->Rprev();
			else if (loc==PL_RIGHT_OF_LINE)
				e = e->Onext();
			else 
			{
				if (verts.back() != e->Dest()) verts.push_back(e->Dest());
				e = e->Onext();
			}
		}
		verts.push_back(b);

		if (diverged)
		{
			std::map<double, Vertex*> vertmap;

			//ccl::ObjLog log;
			//log << "DelaunayTriangulation::GetTouchingVerts() Smart search failed, reverting to brute force." << log.endl;

			vertmap[0] = a;
			for (int i = 0; i < subdivision_->getMaxVerts(); i++)
			{
				Vertex* vert = subdivision_->getVertex(i);
				if (vert)
				{
					PointLineLocation loc = LocatePointOnLine(vert->point, a->point, b->point, epsilon_);
					if (loc == PL_BEGINS_LINE || loc == PL_ENDS_LINE || loc == PL_ON_LINE)
						vertmap[(vert->point - a->point).length2D()] = vert;
				}
			}	
			vertmap[(b->point - a->point).length2D()] = b;

			verts.clear();
			for (std::map<double, Vertex*>::iterator it = vertmap.begin(); it != vertmap.end(); it++)
				verts.push_back(it->second);
		}

		return verts;
	}

	std::vector<Vertex*> DelaunayTriangulation::GatherVertsLocal(const PointList & polygon, bool constrained, bool working)
	{
		Point minP, maxP;


		GetBoundingRect(polygon, minP, maxP);

		std::vector<Vertex*> verts;
		verts.reserve( subdivision_->getMaxVerts() );

		for (int i = 0; i < subdivision_->getMaxVerts(); i++)
		{
			Vertex* vert = subdivision_->getVertex(i);
			if (vert && 
				((constrained && working) ||
				 (constrained && cmap_.IsVertexBound(vert)) ||
				 (working && !cmap_.IsVertexBound(vert))))
			{
				if (polygon.empty())
					verts.push_back(vert);
				else if (vert->point.x < minP.x ||
					vert->point.x > maxP.x ||
					vert->point.y < minP.y ||
					vert->point.y > maxP.y)
					continue;
				if (PointInPolygon(vert->point, polygon, epsilon_))
					verts.push_back(vert);
			}
		}

		return verts;
	}

	std::vector<Vertex*> DelaunayTriangulation::GatherVerts(PointList polygon)
	{
		for (size_t i = 0; i < polygon.size(); i++)
			polygon[i] = TransformPointToLocal(polygon[i]);
		return GatherVertsLocal(polygon, true, true);
		}

	std::vector<Vertex*> DelaunayTriangulation::GatherWorkingVerts(PointList polygon)
		{
		for (size_t i = 0; i < polygon.size(); i++)
			polygon[i] = TransformPointToLocal(polygon[i]);
		return GatherVertsLocal(polygon, false, true);
		}

	std::vector<Vertex*> DelaunayTriangulation::GatherConstrainedVerts(PointList polygon)
	{
		for (size_t i = 0; i < polygon.size(); i++)
			polygon[i] = TransformPointToLocal(polygon[i]);
		return GatherVertsLocal(polygon, true, false);
		}

	std::vector<Edge*> DelaunayTriangulation::GatherTriangles(PointList polygon)
	{
		for (size_t i = 0; i < polygon.size(); i++)
			polygon[i] = TransformPointToLocal(polygon[i]);
		Point minP, maxP;
		GetBoundingRect(polygon, minP, maxP);

		std::unordered_set<Edge*> processedEdges;
		std::vector<Edge*> edges;

		edges.reserve(subdivision_->getMaxEdges()*2);
		for (int i = 0; i < 2*subdivision_->getMaxEdges(); i++)
		{
			Edge* edge = subdivision_->getEdge(i/2);
			if (!edge) continue;
			if (i % 2) edge = edge->Sym();

			Point p1 = edge->Org()->point;
			Point p2 = edge->Lnext()->Org()->point;
			Point p3 = edge->Lprev()->Org()->point;
			if (TArea2D(p1, p2, p3) < epsilon_)
				continue;

			if (processedEdges.count(edge) != 0)
				continue;
			if (!polygon.empty())
			{
				Point center = (p1 + p2 + p3) * (1.0/3.0);
				if (center.x < minP.x || center.x > maxP.x || center.y < minP.y || center.y > maxP.y)
					continue;
				if (!PointInPolygon(center, polygon, epsilon_))
					continue;
			}

			edges.push_back(edge);
			processedEdges.insert(edge);
			processedEdges.insert(edge->Lnext());
			processedEdges.insert(edge->Lprev());
		}
		return edges;
	}

/******************************************************************************************************
	BOOLEAN FUNCTIONS
*******************************************************************************************************/

	bool DelaunayTriangulation::IsRight(Vector vert, Vector a, Vector b)
	{
		return ctl::IsRight(vert, a, b, epsilon_);
	}

	bool DelaunayTriangulation::IsRight(Vector vert, Edge* edge)
	{
		return ctl::IsRight(vert, edge->Org()->point, edge->Dest()->point, epsilon_);
	}

	bool DelaunayTriangulation::IsLeft(Vector vert, Vector a, Vector b)
	{
		return ctl::IsLeft(vert, a, b, epsilon_);
	}

	bool DelaunayTriangulation::IsLeft(Vector vert, Edge* edge)
	{
		return ctl::IsLeft(vert, edge->Org()->point, edge->Dest()->point, epsilon_);
	}

	int DelaunayTriangulation::IsOn(Vector vert, Vector p0, Vector p1)
	{
		PointLineLocation result = ctl::LocatePointOnLine(vert, p0, p1, epsilon_);
		return result <= PL_AFTER_LINE ? result : -1;
	}

	int DelaunayTriangulation::IsOn(Vector vert, Edge* edge)
	{
		PointLineLocation result = ctl::LocatePointOnLine(vert, edge->Org()->point, edge->Dest()->point, epsilon_);
		return result <= PL_AFTER_LINE ? result : -1;
	}

	bool DelaunayTriangulation::InCircle(Vector a, Vector b, Vector c, Vector d)
	{
		return	(a.x*a.x + a.y*a.y)*TArea2D(b,c,d) -
				(b.x*b.x + b.y*b.y)*TArea2D(a,c,d) +
				(c.x*c.x + c.y*c.y)*TArea2D(a,b,d) - 
				(d.x*d.x + d.y*d.y)*TArea2D(a,b,c) > areaEpsilon_;
	}

	bool DelaunayTriangulation::IsFlipValid(Edge* edge)
	{
		Point a = edge->Org()->point;
		Point b = edge->Dest()->point;
		Point alpha = edge->Onext()->Dest()->point;
		Point beta = edge->Oprev()->Dest()->point;

		if (!IsRight(a,alpha,beta)) return false;
		else if (!IsLeft(b,alpha,beta)) return false;

	//	Check for collapsed triangles
		if (TArea2D(alpha,a,beta) < areaEpsilon_) 
			return false;
		else if (TArea2D(alpha,beta,b) < areaEpsilon_) 
			return false;
		else 
			return true;
	}

	bool DelaunayTriangulation::IsDelaunay(Point p, Edge* edge)
	{
		Edge* temp = edge->Oprev();
		return !(IsRight(temp->Dest()->point,edge) && InCircle(edge->Org()->point,temp->Dest()->point,edge->Dest()->point,p));
	}

	bool DelaunayTriangulation::IsInside(Point point)
	{
		return PointInPolygon(point, boundary_, epsilon_);
	}

	bool DelaunayTriangulation::IsInside(const PointList &points)
	{
		for (PointList::const_iterator it = points.begin(); it != points.end(); ++it)
		{
			if (!IsInside(*it)) return false;
		}
		return true;
	}

	bool DelaunayTriangulation::Touches(const PointList &points)
	{
		for (PointList::const_iterator it = points.begin(); it != points.end(); ++it)
		{
			if (IsInside(*it)) return true;
		}
		return false;
	}

/******************************************************************************************************
	SUBDIVISION INTERFACE
*******************************************************************************************************/
	Edge* DelaunayTriangulation::Connect(Vertex* a, Vertex* b)
	{
		return subdivision_->Connect(
			GetLeftEdge(a,b->point)->Sym(), 
			GetLeftEdge(b,a->point)->Oprev()
			);
	}

	Edge* DelaunayTriangulation::GetConnection(Vertex* a, Vertex* b)
	{
		int iteration = 0;
		int iteration_limit = GetNumEdges();

		Edge* start = a->getEdges();
		Edge* next = start;
		do
		{
			if (iteration++ > iteration_limit)
			{
#ifdef CTL_DEBUG
				if (GlobalCTLDebugEnabled)
				{
					glViewer::Clear();
					glViewer::AddPoint(a->point);
					glViewer::AddPoint(b->point);
					glViewer::AddCDT(this);
					glViewer::Display();
				}
#endif
				//ccl::ObjLog log;
				//log << ccl::LERR << "DelaunayTriangulation::GetConnection() Edges do not form cycle around vertex." << log.endl;
				error_ = CORRUPTION_DETECTED;
				break;
			}

			if (next->Dest() == b) return next;
			else next = next->Onext();
		}
		while (next != start);
		return NULL;
	}

	Edge* DelaunayTriangulation::GetLeftEdge(Vertex* a, Point p)
	{
		Edge* edge = a->getEdges();
		Edge* startEdge = edge;
		int iteration = 0;
		int iteration_limit = GetNumEdges();

		// We need to return an edge so that the point is to the right of the edge.
		// If the first point is to the right, search the previous edges.
		// If the first points is to the left, search the next edges
		// Note that we check for !IsLeft() and !IsRight() so that the function returns an edge 

		if(IsRight(p, edge))
		{
			while(!IsLeft(p, edge->Oprev()))
			{
				edge = edge->Oprev();
				if (iteration++ > iteration_limit || edge==startEdge)
				{
					//ccl::ObjLog log;
					//log << ccl::LERR << "DelaunayTriangulation::GetLeftEdge() No 'left edge' found, this indicates graph corruption." << log.endl;
#ifdef CTL_DEBUG
					if (GlobalCTLDebugEnabled)
					{
						glViewer::Clear();
						glViewer::AddPoint(a->point);
						glViewer::AddPoint(p);
						glViewer::AddCDT(this);
						glViewer::Display();
					}
#endif
					error_ = CORRUPTION_DETECTED;
					
					return edge;



				}
			}
		}
		else
		{
			while(!IsRight(p, edge->Onext()))
			{
				edge = edge->Onext();
				if (iteration++ > iteration_limit || edge==startEdge)
				{
#ifdef CTL_DEBUG
					if (GlobalCTLDebugEnabled)
					{
						glViewer::Clear();
						glViewer::AddCDT(this);
						glViewer::AddPoint(a->point);
						glViewer::AddPoint(p);
						glViewer::Display();
					}
#endif
					error_ = CORRUPTION_DETECTED;
					//ccl::ObjLog log;
					//log << ccl::LERR << "DelaunayTriangulation::GetLeftEdge() No 'left edge' found, this indicates graph corruption." << log.endl;
					return edge;
				}
			}
			edge = edge->Onext();
		}

		return edge;
	}

	void DelaunayTriangulation::Swap(Edge* edge)
	{
		Edge* a = edge->Oprev();
		Edge* b = edge->Sym()->Oprev();

		edge->Org()->removeEdge(edge);
		edge->Dest()->removeEdge(edge->Sym());

		Edge::Splice(edge,a);
		Edge::Splice(edge->Sym(),b);
		Edge::Splice(edge,a->Lnext());
		Edge::Splice(edge->Sym(),b->Lnext());
		edge->setEndPoints(a->Dest(),b->Dest());
	}

	void DelaunayTriangulation::RemoveEdge(Edge* edge)
	{
		cmap_.FreeEdge(edge);
		subdivision_->RemoveEdge(edge);
	}

	void DelaunayTriangulation::RemoveVertex(Vertex* vert)
	{
		Edge* base = vert->getEdges();
		Edge* remaining = base->Lnext();

		while (base->Onext() != base)
			RemoveEdge(base->Onext());

		RemoveEdge(base);
		RetriangulateFace(remaining);

		subdivision_->RemoveVertex(vert);
	}

/******************************************************************************************************
	SEARCH AND INTERSECTION
*******************************************************************************************************/

	LocationResult DelaunayTriangulation::LocatePoint(Point p, Edge* edge)
	{
		if (edge == NULL)
			edge = subdivision_->getRandomEdge();

		bool diverged = false;
		int iteration = 0;
		int iteration_limit = GetNumEdges();

		while (true)
		{
			/*
#ifdef CTL_DEBUG
		if (this->Enabled(VISUAL_DEBUG)){
			glViewer::Clear();
			glViewer::AddCDT(this);
			glViewer::AddPoint(p);
			glViewer::AddEdge(edge);
			glViewer::Display();
		}
#endif
		*/
			if (iteration++ > iteration_limit)
			{
				diverged = true;
				break;
			}

			PointLineLocation result = ctl::LocatePointOnLine(p, edge->Org()->point, edge->Dest()->point, epsilon_);
			if (result==PL_RIGHT_OF_LINE)
			{
				edge = edge->Sym();
				continue;
			}

			if (result == PL_BEGINS_LINE)
				return LocationResult(edge,LR_VERTEX);
			else if (result == PL_ON_LINE)
				return LocationResult(edge,LR_EDGE);
			else if (result == PL_ENDS_LINE)
				return LocationResult(edge->Sym(),LR_VERTEX);
			else if (!IsRight(p,edge->Onext()))
			{
			//	Changed by janghel on 9/21 to prevent cyclical problem when searching along a boundary.
			//	if (IsRight(edge->Onext()->Dest()->point, edge))
				if (!IsLeft(edge->Onext()->Dest()->point, edge))
					edge = edge->Rprev();
				else
					edge = edge->Onext();
			}
			else if (!IsRight(p,edge->Dprev()))    
				edge = edge->Dprev();
			else
				return LocationResult(edge,LR_FACE);
		}

		if (diverged)
		{
			//ccl::ObjLog log;
			//log << "DelaunayTriangulation::LocatePoint() Smart search failed to converge, reverting to brute force." << log.endl;
			for (int i = 0; i < subdivision_->getMaxEdges(); i++)
			{
				Edge* edge = subdivision_->getEdge(i);
				if (edge)
				{
					int result = IsOn(p,edge);
					if (result == PL_BEGINS_LINE)
						return LocationResult(edge,LR_VERTEX);
					else if (result == PL_ON_LINE)
						return LocationResult(edge,LR_EDGE);
					else if (result == PL_ENDS_LINE)
						return LocationResult(edge->Sym(),LR_VERTEX);
					else if (IsLeft(p, edge) && IsLeft(p, edge->Lnext()) && IsLeft(p, edge->Lprev()))
						return LocationResult(edge,LR_FACE);
				}
				if (edge)
				{
					edge = edge->Sym();
					int result = IsOn(p,edge);
					if (result == PL_BEGINS_LINE)
						return LocationResult(edge,LR_VERTEX);
					else if (result == PL_ON_LINE)
						return LocationResult(edge,LR_EDGE);
					else if (result == PL_ENDS_LINE)
						return LocationResult(edge->Sym(),LR_VERTEX);
					else if (IsLeft(p, edge) && IsLeft(p, edge->Lnext()) && IsLeft(p, edge->Lprev()))
						return LocationResult(edge,LR_FACE);
				}
			}
			//log << "DelaunayTriangulation::LocatePoint() Failed to locate point using brute force." << log.endl;
		}

		return LocationResult();
	}

	Point DelaunayTriangulation::Intersection(Edge* edge,Point c, Point d)
	{
		Point a = edge->Org()->point;
		Point b = edge->Dest()->point;

		Point u = b-a;
		Point v = d-c;

		Point w = a - c;

		if (Enabled(INTERPOLATE_EDGES))
		{
			double dist = ( v.x*w.y - v.y*w.x )/( v.y*u.x - v.x*u.y );
			return a + u*dist;
		}
		else
		{
			double dist = ( u.x*w.y - u.y*w.x )/( u.x*v.y - u.y*v.x );
			return c + v*dist;
		}
	}

	void DelaunayTriangulation::InterpolateZ(Vector O, Vector U, Vector V, Vertex *vert)
	{
		Point P = vert->point - O;
		double denom = V.x*U.y - V.y*U.x;
		double v = ( P.x*U.y - P.y*U.x )/denom;
		double u = ( P.y*V.x - P.x*V.y )/denom;
		vert->point.z = O.z + u*U.z + v*V.z;
	}

/******************************************************************************************************
	INSERTION
*******************************************************************************************************/

	Vertex* DelaunayTriangulation::SnapPointToVertex(const Point& p)
	{
		return bsp->findSnapVertex(p);
		//TODO - use BSP
		for (int i = 0, n = subdivision_->getMaxVerts(); i < n; ++i)
		{
			Vertex* v = subdivision_->getVertex(i);
			if (v && (v->point - p).length2D2() < 0.01)
				return v;
		}
		return NULL;
	}

	Vertex* DelaunayTriangulation::SnapPointToEdge(const Point& p)
	{
		//TODO - use BSP
		for (int i = 0, n = subdivision_->getMaxEdges(); i < n; ++i)
		{
			Edge* e = subdivision_->getEdge(i);
			if (e && cmap_.IsEdgeBound(e) && (LocatePointOnLine(p, e->Org()->point, e->Dest()->point, 0.01) == PL_ON_LINE))
				return InsertPointInEdge(p, e);
		}
		return NULL;
	}

	Vertex* DelaunayTriangulation::InsertPoint(Point p)
	{
		if (Enabled(FLATTENING)) p.z = 0;

		LocationResult location = LocatePoint(p);

		Vertex *result = NULL;
		if (location.getType() == LR_EDGE)
			result = InsertPointInEdge(p,location.getEdge());
		else if (location.getType() == LR_FACE)
			result = InsertPointInFace(p,location.getEdge());
		else if (location.getType() == LR_VERTEX)
		{
			result = location.getEdge()->Org();
			result->point.z = p.z; // Need to set the proper Z
			return result; // Do not add to bsp -already included
		}
		if(result)
			bsp->addVertex(result);
		return result;
	}

	Vertex* DelaunayTriangulation::InsertPointInEdge(Point p, Edge* edge)
	{
	//	Snap point to edge exactly. This is fast approximation.
	//	Since the point is known to be very close to the edge the distance between
	//	the point and the edge origin is very close to the distance from the
	//	edge origin to the closest point on the edge.
		Vector u = edge->Dest()->point - edge->Org()->point;
		Vector v = p - edge->Org()->point;

		double z = p.z;
		p = edge->Org()->point + u*(v.length2D()/u.length2D());
		if (!Enabled(INTERPOLATE_EDGES))
			p.z = z;

	//	Insert point
		if (IsBoundaryEdge(edge)) 
			return InsertPointInBoundary(p,edge);

		IDList constraints = cmap_.GetConstraints(edge);
		edge = edge->Oprev();
		RemoveEdge(edge->Onext());

	//	Create new edge
		Vertex* vert = subdivision_->CreateVertex(p);
		Edge* base = subdivision_->CreateEdge(edge->Org(),vert);
		cmap_.BindEdge(base,constraints);
		Edge::Splice(base,edge);

	//	Connect right side
		base = subdivision_->Connect(edge,base->Sym());
		edge = base->Oprev();

	//	Connect other side
		base = subdivision_->Connect(edge,base->Sym());
		edge = base->Oprev();
		cmap_.BindEdge(base,constraints);

	//	Connect left side
		base = subdivision_->Connect(edge,base->Sym());
		edge = base->Oprev();

		FlipEdges(p,base);
		return vert;
	}

	Vertex* DelaunayTriangulation::InsertPointInBoundary(Point p, Edge* edge)
	{
	//	Ensure edge is directed so that its left face is not outside the triangulation
		for (size_t i = 0; i < boundary_.size(); i++)
		{
			if (IsRight(boundary_[i],edge))
			{
				edge = edge->Sym();
				break;
			}
		}

		IDList constraints = cmap_.GetConstraints(edge);
		edge = edge->Lnext();
		RemoveEdge(edge->Onext());

	//	Create new edge
		Vertex* vert = subdivision_->CreateVertex(p);

	//	Create first half of boundary edge
		Edge* base = subdivision_->CreateEdge(edge->Org(),vert);
		cmap_.BindEdge(base,constraints);
		Edge::Splice(edge,base);

	//	Connect right side
		base = subdivision_->Connect(edge,base->Sym());
		edge = base->Oprev();

	//	Connect other side
		base = subdivision_->Connect(edge, base->Sym());
		cmap_.BindEdge(base, constraints);
		edge = base->Oprev();

		FlipEdges(p,edge);

	//	Insert this point as a working point in the any neighbors
		for (size_t i = 0; i < neighbors_.size(); i++)
		{
			if (neighbors_[i])
			{
				if (IsBoundaryVertex(vert,i))
				{
					size_t l;
					for (l = 0; l < neighbors_[i]->neighbors_.size(); l++)
						if (neighbors_[i]->neighbors_[l] == this)
							break;

					neighbors_[i]->neighbors_[l] = NULL;
					neighbors_[i]->InsertWorkingPoint(p);
					neighbors_[i]->neighbors_[l] = this;
				}
			}
		}

		return vert;
	}

	Vertex* DelaunayTriangulation::InsertPointInFace(Point p, Edge* edge)
	{
		Vertex* vert = subdivision_->CreateVertex(p);

	//	Interpolate Z value if needed
		if (Enabled(INTERPOLATE_FACES))
		{
			Point O = edge->Org()->point;
			Vector U = edge->Dest()->point - O;
			Vector V = edge->Onext()->Dest()->point - O;
			InterpolateZ(O,U,V,vert);
		}

		Edge* base = subdivision_->CreateEdge(edge->Org(),vert);
		Edge::Splice(base,edge);

		base = subdivision_->Connect(edge,base->Sym());

		Edge* base2 = subdivision_->Connect(base->Oprev(),base->Sym());

		FlipEdges(p,base);
		return vert;
	}

	void DelaunayTriangulation::SimplifyEdges(Vertex* vert)
	{
		INIT_CORRUPT_TEST

		if (!IsSimplifyValid(vert)) return;

		int n = cmap_.GetNumBoundEdges(vert);
		if (n == 0) 
			RemoveVertex(vert);
		else if (n == 2)
		{
		//	Gather Left and Right
			Edge* left = NULL;
			Edge* right = NULL;
			
			Edge* edge = vert->getEdges();
			Edge* start = edge;
			do
			{
				RUN_CORRUPT_TEST

				if (cmap_.IsEdgeBound(edge))
				{
					if (left) right = edge;
					else left = edge;
				}
				edge = edge->Onext();
			}
			while (edge != start);

		//	If Edge is collinear and the constraint maps are equal then merge
			IDList left_cmap = cmap_.GetConstraints(left);
			IDList right_cmap = cmap_.GetConstraints(right);
			if (left_cmap.size() != right_cmap.size())
				return;
			if (equal(left_cmap.begin(),left_cmap.end(),right_cmap.begin()))
			{
				if (IsOn(left->Dest()->point,right)!=-1)
				{
					Vertex* left_vert = left->Dest();
					Vertex* right_vert = right->Dest();

					if (!IsBoundaryEdge(left))
					{
						RemoveVertex(vert);

						Edge* edge = GetConnection(left_vert,right_vert);
						if (!edge)
						{
							InsertSegment(left_vert,right_vert,1);
							edge = GetConnection(left_vert,right_vert);
						}

						cmap_.BindEdge(edge,left_cmap);
					}
					else	// SPECIAL CASE OF BOUNDARY VERTEX REMOVAL
					{
						//	Remove all Edges about the vertex
							Edge* base = vert->getEdges();
							while (base->Onext() != base)
								RemoveEdge(base->Onext());

							RemoveEdge(base);

						//	Delete Vertex
							subdivision_->RemoveVertex(vert);

						//	Make a new connection before retriangulating
							base = Connect(left_vert,right_vert);

						//	Constrain new edge
							cmap_.BindEdge(base,left_cmap);

						//	Retriangulate inner face only
							if (!(IsLeft(boundary_[0],base) || IsLeft(boundary_[1],base))) base = base->Sym();
							RetriangulateFace(base);

						//	Flip Edges to ensure retriangulation is Delaunay

					}
				}
			}
		}
	}

	bool DelaunayTriangulation::IsSimplifyValid(Vertex* vert)
	{
		if (cmap_.IsVertexBound(vert)) return false;
		
		for (size_t i = 0; i < neighbors_.size(); i++)
		{
			if (neighbors_[i])
			{
				if (IsBoundaryVertex(vert,i))
				{
					Vertex* other = neighbors_[i]->InsertPoint(vert->point);
					if (!other)
						return false;
					int n = neighbors_[i]->cmap_.GetNumBoundEdges(other);
					if (n > 0) return false;
				}
			}
		}

		return true;
	}

	void DelaunayTriangulation::FlipEdges(Point p, Edge* base)
	{
		if (Enabled(DISABLE_EDGE_FLIPS))
			return;

		Edge *e = base->Lprev();

		int count = 0;

		do
		{
			if (count > maxEdgeFlips_) return;
			count++;

			if (!IsDelaunay(p,e) && !cmap_.IsEdgeBound(e) && IsFlipValid(e))
			{
				Swap(e);
				e = e->Oprev();
			}
			else if (e->Onext() == base) return;
			else e = e->Onext()->Lprev();

		} while (true);
	}

	void DelaunayTriangulation::InsertSegment(Vertex* a, Vertex* b, ID constraintID)
	{
		if (a == b) return;

		std::vector<Edge*>		edges;
		std::vector<Vertex*>	verts;

	// Add intersection Points to constrained edges.
		edges = GetCrossingEdges(a,b,true,false);
		for (std::vector<Edge*>::iterator it = edges.begin(); it != edges.end(); ++it)
		{
			InsertPointInEdge(Intersection(*it,a->point,b->point),*it);
		}
		//edges.clear();

	// Remove all non-constrained edges. Must research for crossing edges because of changes from inserting new points
		edges = GetCrossingEdges(a,b,false,true);
		for (std::vector<Edge*>::iterator it = edges.begin(); it != edges.end(); ++it)
		{
			RemoveEdge(*it);
		}
		//edges.clear();

	//	Handle all parallel edgese
		verts = GetTouchingVerts(a,b);
		for (int i = 1; i < int(verts.size()); i++)
		{
			Edge* connection = GetConnection(verts[i-1],verts[i]);
			if (!connection) 
			{
				connection = Connect(verts[i-1],verts[i]);
				cmap_.BindEdge(connection,constraintID);
				
				RetriangulateFace(connection);
				if (error_) break;
				
				RetriangulateFace(connection->Sym());
				if (error_) break;
			}
			else cmap_.BindEdge(connection,constraintID);
		}
	}

	void DelaunayTriangulation::RetriangulateFace(Edge* base)
	{
		if (!base) return;

		if (base->Org() != base->Lnext()->Lnext()->Dest())
		{
			Edge* SplitPoint = base->Lnext();
			Edge* Next = SplitPoint;

			int iteration = 0;
			int iteration_limit = GetNumEdges();

			while (Next->Dest() != base->Org())
			{
				if (iteration++ > iteration_limit)
				{
#ifdef CTL_DEBUG
					if (GlobalCTLDebugEnabled)
					{
						glViewer::Clear();
						glViewer::AddEdge(base);
						glViewer::AddCDT(this);
						glViewer::Display();
					}
#endif
					error_ = CORRUPTION_DETECTED;
					//ccl::ObjLog log;
					//log << ccl::LERR << "DelaunayTriangulation::RetriangulateFace() Cannot retriangulate non-closed face." << log.endl;
					return;
				}

				if (InCircle(base->Org()->point,base->Dest()->point,SplitPoint->Dest()->point,Next->Dest()->point))
					SplitPoint = Next;
				Next = Next->Lnext();
			}

			Edge* left = NULL;
			Edge* right = NULL;

			//	Create new edges only if they are valid (don't already exist)
			if ( base->Lprev()->Lprev() != SplitPoint ) 
				left = subdivision_->Connect(base->Lprev(),SplitPoint->Lnext());
			if ( SplitPoint != base->Lnext() ) 
				right = subdivision_->Connect(SplitPoint,base->Lnext());

			RetriangulateFace(left);
			RetriangulateFace(right);
		}
	}

	Point DelaunayTriangulation::TransformPointToLocal(const Point& p) const
	{
		return p - origin_;
	}

	Point DelaunayTriangulation::TransformPointToGlobal(const Point& p) const
	{
		return p + origin_;
	}

/******************************************************************************************************
	Mesh Simplification
*******************************************************************************************************/
	float DelaunayTriangulation::ComputeVertexError(Vertex* vert) const
	{
#if 1 // Average Plane Computation
	//	Compute the average plane
		Vector origin = vert->point;
		Vector b_vector;
		Vector n_vector;
		double total_area = 0;
		Edge* edge = vert->getEdges();
		Edge* end = edge->Oprev();
		while (edge != end)
		{
			Vector p1 = edge->Dest()->point;
			Vector p2 = edge->Oprev()->Dest()->point;

			Vector c = (origin + p1 + p2) * (1.0/3.0);
			Vector n = (p1 - origin).cross(p2 - origin);
			n.normalize();
			double a = TArea3D(origin, p1, p2);

			b_vector += c*a;
			n_vector += n*a;
			total_area += a;

			edge = edge->Onext();
		}

	//	The base point and normal of the plane
		b_vector *= (1/total_area);
		n_vector *= (1/total_area);
		n_vector.normalize();

	//	Compute point to plane distance
		return abs((origin - b_vector).dot(n_vector));
#endif
		return 0;
	}

#if 0
	void DelaunayTriangulation::Simplify(int targetPoints, float threshold)
	{
	//	Create space to hold errors
		std::multimap<float, Vertex*> errors;

	//	Compute initial error
		for (int i=0; i<subdivision_->getMaxVerts(); i++)
		{
			Vertex* vert = subdivision_->getVertex(i);
			if (vert && !cmap_.IsVertexBound(vert))
				errors.insert(std::pair<float, Vertex*>(ComputeVertexError(vert), vert));
		}

	//	Main loop
	//	Remove the necessary points
		int pointsRemoved = 0;
		int pointsToRemove = GetNumVertices() - targetPoints;
		for (std::multimap<float, Vertex*>::iterator it = errors.begin(); it != errors.end(); it++)
		{
			if (pointsRemoved++ >= pointsToRemove)
				return;
			if (it->first > threshold)
				return;

			RemoveVertex(it->second);
		}
	}
#else
	void DelaunayTriangulation::Simplify(int targetFaces, float threshold)
	{
	//	Create space to hold errors
		std::multimap<float, Vertex*> errors;

	//	Compute initial error
		for (int i=0; i<subdivision_->getMaxVerts(); i++)
		{
			Vertex* vert = subdivision_->getVertex(i);
			if (vert && !cmap_.IsVertexBound(vert))
				errors.insert(std::pair<float, Vertex*>(ComputeVertexError(vert), vert));
		}

	//	Main loop
	//	Remove the necessary points
		for (std::multimap<float, Vertex*>::iterator it = errors.begin(); it != errors.end(); it++)
		{
			if (GetNumTriangles(true) < targetFaces)
				return;
			if (it->first > threshold)
				return;

			RemoveVertex(it->second);
		}
	}
#endif



	DelaunayBSP::~DelaunayBSP(void)
	{
		if(minChild)
			delete minChild;
		if(maxChild)
			delete maxChild;
	}

	DelaunayBSP::DelaunayBSP(const PointList &boundary) : depth(0), vertical(true), minChild(NULL), maxChild(NULL)
	{
		GetBoundingRect(boundary, minPoint, maxPoint);
	}

	DelaunayBSP::DelaunayBSP(const Point &minPoint, const Point &maxPoint, size_t depth, bool vertical) : minPoint(minPoint), maxPoint(maxPoint), depth(depth + 1), vertical(vertical), minChild(NULL), maxChild(NULL)
	{
	}

	void DelaunayBSP::addVertex(Vertex *vertex)
	{
		if(minChild)
		{
			double split = vertical ? (minPoint.x + maxPoint.x) / 2 : (minPoint.y + maxPoint.y) / 2;
			double axis = vertical ? vertex->point.x : vertex->point.y;
			if(axis < split)
				minChild->addVertex(vertex);
			else if(axis > split)
				maxChild->addVertex(vertex);
			else
				vertices.push_back(vertex);
			return;
		}
		vertices.push_back(vertex);
		if(depth >= 10)
			return;
		if(vertices.size() <= 10)
			return;

		Point minMinPoint(minPoint);
		Point minMaxPoint(maxPoint);
		Point maxMinPoint(minPoint);
		Point maxMaxPoint(maxPoint);
		if(vertical)
		{
			double split = (minPoint.x + maxPoint.x) / 2;
			minMaxPoint.x = split;
			maxMinPoint.x = split;
		}
		else
		{
			double split = (minPoint.y + maxPoint.y) / 2;
			minMaxPoint.y = split;
			maxMinPoint.y = split;
		}
		minChild = new DelaunayBSP(minMinPoint, minMaxPoint, depth, !vertical);
		maxChild = new DelaunayBSP(maxMinPoint, maxMaxPoint, depth, !vertical);

		std::list<Vertex *> tmp = vertices;
		vertices.clear();
		for(std::list<Vertex *>::iterator it = tmp.begin(), end = tmp.end(); it != end; ++it)
			addVertex(*it);
	}

	Vertex *DelaunayBSP::findSnapVertex(const Point &point, double distance)
	{
		for(std::list<Vertex *>::iterator it = vertices.begin(), end = vertices.end(); it != end; ++it)
		{
			if(((*it)->point - point).length2D() < 0.01)
				return *it;
		}
		double split = vertical ? (minPoint.x + maxPoint.x) / 2 : (minPoint.y + maxPoint.y) / 2;
		double axis = vertical ? point.x : point.y;
		if(minChild && (axis < split))
			return minChild->findSnapVertex(point, distance);
		if(maxChild && (axis > split))
			return maxChild->findSnapVertex(point, distance);
		return NULL;
	}



}

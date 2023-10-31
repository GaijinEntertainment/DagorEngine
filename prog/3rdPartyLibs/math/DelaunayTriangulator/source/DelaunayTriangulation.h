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
/*!	\file ctl/DelaunayTriangulation.h
\headerfile ctl/DelaunayTriangulation.h
\brief Provides ctl::DelaunayTriangulation.
\author Joshua Anghel
*/
#pragma once

#include "Subdivision.h"
#include "LocationResult.h"
#include "ConstraintMap.h"
#include "CGrid.h"
#include <list>


namespace ctl {

	class DelaunayBSP
	{
	public:
		size_t depth;
		bool vertical;
		ctl::Point minPoint;
		ctl::Point maxPoint;
		DelaunayBSP *minChild;
		DelaunayBSP *maxChild;
		std::list<Vertex *> vertices;

		~DelaunayBSP(void);
		DelaunayBSP(const PointList &boundary);
		DelaunayBSP(const Point &minPoint, const Point &maxPoint, size_t depth, bool vertical);

		void addVertex(Vertex *vertex);
		Vertex *findSnapVertex(const Point &point, double distance = 0.01);
	};


/*!	\class ctl::DelaunayTriangulation ctl/DelaunayTriangulation.h ctl/DelaunayTriangulation.h
	\brief DelaunayTriangulation
	
	Provides a Dynamically Constrained Delaunay Triangulation system. Based on an incrimental Delaunay Triangulation algorithm
	with the added feature of constraint insertion and removal. Capable of creating a point only Delaunay Triangulation in O(n^4/3) time
	( provided the points are inserted randomly - worst case time is n^2 and is encounted when inserting points of a grid in order, to counter
	this worst case behaviour an even faster algorithm for generating DelaunayTringulations from a grid is provided).



	DelaunayTriangulation Construction:

	When creating a DelaunayTriangulation a few options must be specified.

	BoundingRegion:		A DelaunayTriangulation must be initially constrainted to some bounding region (in this implementation a quad).
						This must be specified by the user upon construction.
						The user can specify 4 points of a bounding QUAD upon construction.
						The user can specify a CGrid (which contains a bounding QUAD) upon construction.
						Defaults to (NO DEFAULT - MUST BE SPECIFIED).
						Must be counter-clockwise in orientation.

	DataBlockSize:		The DelaunayTriangulation needs to know how large to make new Vertex and Edge data blocks if more Edges or Verts are needed.
						This value will set the size (as in number of Edges or Verts to a block, not the number of bytes) per block. A larger size per
						block will result in fewer blocks overall and thus less time spent on memory management, while a smaller block size will
						result in less unused allocated memory at any given time, but result in longer processing times since more blocks must be managed.
						Default to (100)
 
	Epsilon:			A custom epsilon can be defined for comparisons in this DelaunayTriangulation. If the epsilon is set too small, some rounding
						errors can lead to crashes or infinite loops. The default has been tested to work for a large number of cases. If undesired
						results are present, try increasing the epsilon slightly.
						Default to (3e-13).
 
	MaxEdgeFlips:		The maximum number of Edge flips that will be performed for any call to FlipEdges by the DelaunayTriangulation.
						This is important because a Constrained Delaunay Triangulation is not garunteed to be Delaunay since many Edges are
						restricted from Edge flips and no extra Vertices (other than intersection points) are created. With this in mind
						it is possible for the EdgeFlips to ripple outward from a new constraint for many itterations (which get increasingly
						worse with a DelaunayTriangulation with many Edges). To avoid this, MaxEdgeFlips is specified which will constrain
						the number of flips the DelaunayTriangulation will use in an attempt to correct the triangulation after inserting a new
						constraint.
						Defaults to (10000).
	
	Options:			Additional Options may be specified or enabled for this DelaunayTriangulation (several options may be | together). 
						The Options allowed are:

						-	CLIPPING	
								When Enabled, if a portion of a constraint lies outside of the bounding region, the DelaunayTriangulation will first try clipping
								the constraint to see if any portion is inside the DelaunayTriangulation. When Disabled, if ANY portion of the constraint is
								outside of the bouning region, the entire constraint will be ignored.

						-	FLATTENING 
								When Enabled, all Z values of points added to this Triangulation will become 0.

						-	INTERPOLATE_EDGES 
								When Enabled, any intersecting constrained Edges will create a new point with a Z value intepolated by the currently inserted 
								Edge.
						
						-	INTERPOLATE_FACES 
								When Enabled, any Point inserted into a face, will have its Z value adjusted to match the face.


	Constraints:
 
		A constraint is defined as a single Point or a single list of Points that form a line string. When a constraint is inserted into the DT it will force 
		all the segments of the constraint to appear in the DT regardless if they are Delaunay or not.
	 
		If two constraints are found to intersect or overlap one another, a new Vertex point will be generated at the intersection and placed into the DT to 
		ensure that both constraints are satisfied. Note that this new Vertex will have a z value determined based on the ZInterpolationOption settings. Upon
		removing one of the overlapping constraints, if it is found that the newly created Vertex is no longer necessary, the Vertex will be removed and the
		DelaunayTriangulation will be simplified.


	Insertion:

	WorkingPoint - 
		
		When a working point is inserted, it is a temporary point that will only be gaurunteed to last until the next insertion or removal.

	ConstrainedPoint - 

		When a constrained Point is inserted, the only way that point will ever be removed is by calling RemoveConstraint with its unique ID. Note that
		ID values are unique even between Point constraints and LineString constraints, so no ID assigned to a Point will ever be given to a LineString or
		vice versa.

	ConstrainedLineString -

		A constrained LineString inserts a series of constrained Edges into the DelaunayTriangualtion. The Z values of the constraints are handled by the
		settings.

	ConstrainedPolygon -

		A constrained Polygon inserts a series of constrained Edges that form a single closed/simple area. After inserting a Polygon, all points and edges
		inside the Polygon are simplified and removed if possible or else interpolated to the Z value of the Polygon by force. In this regard, the Z
		interpolation settings are disregarded in this method and the Z values of the Polygon are used always. This method is great for inserting roads
		if they already have the z values you want.


	ConstraintID values:

		When inserting a Constraint, an ID value will always be returned. If this ID is 0, the insertion failed for some reason (the constraint may be outside of 
		this DelaunayTriangulation). The ID 0 is reserved internally for the boundary of the DelaunayTriangulation and will never be assigned to any constraint
		that a user has any control over.

		ID values are unique within this DelaunayTriangulation. No ID assigned to a Point will be shared by any other Point OR LineString OR Polygon. Because of this
		there is only one method to remove a constraint. It will determine what type the constraint is based on the ID.


	Exporting the Data from a DelaunayTriangulation:

		A DelaunayTriangulation is capable of exporting the triangulation at any given moment as either a Subdivision or TIN.

		When retrieving the Subdivision of a DelaunayTriangulation, the actual raw data-structure used by the DelaunayTriangulation system itself will be returned.
		This data is fragile and should not be changed outside the scope of the DelaunayTriangulation class itself. If using the Subdivision of a DelaunayTriangulation,
		be careful not to change and data within a Vertex or Edge or the DelaunayTriangulation might (and most probably will) break.

		A TIN can also be retrieved. Note that all needed data is copied to the TIN and therefore no access to the original Subdivision is given to the TIN. A TIN can 
		be used if no topology information is needed. All normals are computed before hand for the TIN while the data is still within the Subdivision. See ctl::TIN for 
		more information on Creating a TIN out of a DelaunayTriangulation.


	Linking multiple DelaunayTriangulations together:

		An interface for linking multiple neighbooring DelaunayTriangulations is provided. Note that no linker passes ownership and no memory is shared by either DelaunayTriangulation
		(meaning no neighboors are deleted upong this DelaunayTriangulations destruction). Each DelaunayTriangulation has a left, right, up and down neighboor. These neighboors are
		used when computing vertex normals

		
		This implementation uses the quad-edge data structure presented by Guibas and Stolfi (1985) (See ctl::Subdivision and ctl::Edge for more information), and used
		Graphics Gems IV (Paul S. Heckbert) as a reference for some of the basic Delaunay Triangulation code, while referencing a paper by Marcelo Kallmann for strategies
		and algorithms for handling constrained edges within a DelaunayTriangulation.


	Other Notes:

		A DelaunayTriangualtion cannot be copied or assigned due to the delicate data structures that hold the Edges and Vertices. No shared or referance pointer is 
		typedefed for a DelaunayTriangulation, but it is best to create some form of managed pointer if you plan on using a DelaunayTriangulation outside of a single
		scope.

	Usage Examples:

	\code

//	Create the boundary of your triangulation	
	PointList boundary;
	boundary.push_back(Point(0,0));
	boundary.push_back(Point(10,0));
	boundary.push_back(Point(10,10));
	boundary.push_back(Point(0,10));

//	Create the triangulation
	DelaunayTriangulation* DT = new DelaunayTriangulation(boundary);

//	Altering settings
//	Disable and Enable clipping
	DT->Disable( DelaunayTriangualtion::CLIPPING );
	DT->Enable( DelaunayTriangualtion::CLIPPING );

//	Insert a working Point
	DT->InsertWorkingPoint(Point(5,7,8));

//	Insert a constrained Point
	ID p0_id = DT->InsertConstrainedPoint(Point(5,7,8));
	if (p0_id == 0)
	{
		//... Handle failed point insertion
	}

//	Inserting a constrained LineString or Polygon -> For Polygon the LineString MUST be defined CCW.
	PointList points;
	points.push_back(1,1,5);
	points.push_back(2,1,5);
	points.push_back(2,2,5);
	points.push_back(2,1,5);

	ID l0_id = DT->InsertConstrainedLineString(points);
	if (l0_id == 0) 
	{
		//... Handle failed linestring insertion
	}

//	a DT will close your polygon for you if you fail to close it yourself
	ID poly0_id = DT->InsertConstrainedPolygon(points);
	if (poly0_id == 0)
	{
		//...	Handle failed polygon insertion
	}

//	Removing the constraints
	DT->RemoveConstraint(p0_id);
	DT->RemoveConstraint(l0_id);
	DT->RemoveConstraint(poly0_id);


//	Linking a neighbor DelaunayTriangulation
	DelaunayTriangulation *DT_2 = new DelaunayTriangulation( ...some boundary lines up with the right side of DT... );
	bool linked = DT->LinkNeighbor( DelaunayTriangulation::EAST, DT_2 );
	if (!linked)
	{
		//... Handle failed DelaunayTriangulation linking
	}

//	Unlink
	DT->UnlinkNeighbor( DelaunayTriangulation::EAST );

//	Or you can simply wait till one of the DelaunayTriangulations is deleted and the link will be automatically broken.

//	Data Exporting
	TIN* tin_all = new TIN(DT);		//	For entire DelaunayTriangulation ...or...
	TIN* tin_some = new TIN( DT, DT->GetBoundedTriangled( ...some area... ) );	// For only the triangles within a given area (tested by point in polygon of their centroids

	delete tin_all;
	delete tin_some;

//	Finished using DelaunayTriangulation
	delete DT;	//	If the link still exists, it will be removed now
	delete DT_2;

	\endcode

	\sa Fully Dynamic Constrained Delaunay Triangulations (Marcelo Kallmann et. al. 2003)
	\sa Graphics Gems IV (Paul S. Heckbert 1994)
	\sa Guibas and Stolfi (1985)
 */
	class DelaunayTriangulation 
	{
	public:

		enum Option
		{
			CLIPPING				= 0x01,			//	Enable Constraint Clipping
			FLATTENING				= 0x02,			//	Enable Flattening
			INTERPOLATE_EDGES		= 0x04,			//	Interpolate Intersections between Constraints using current Constraint.
			INTERPOLATE_FACES		= 0x08,			//	Interpolate new face Vertex points using the current face.
			VISUAL_DEBUG			= 0x10,			//	Use visual debugging if possible
			DISABLE_EDGE_FLIPS		= 0x20			//	Disable edge flipping. This prevents delaunay enforcement, but saves computations.
		};

		enum Error
		{
			INVALID_BOUNDARY		= 0x01,			//	An invalid boundary was used to construct this DelaunayTriangulation
			CORRUPTION_DETECTED		= 0x02,			//	An unknown error has cased the graph to become corrupt
		};

	protected:
		Point								origin_;
		PointList							boundary_;
		std::vector<DelaunayTriangulation*> neighbors_;

		std::vector<Vertex *> boundary_verts;

		double								epsilon_;
		double								areaEpsilon_;
		int									maxEdgeFlips_;
		int									settings_;
		Subdivision*						subdivision_;
		ConstraintMap						cmap_;
		int									error_;

		DelaunayBSP *bsp;



	public:

/******************************************************************************************************
	INITIALIZATION
*******************************************************************************************************/
		DelaunayTriangulation
		(
			PointList			boundary,
			int					resizeIncriment	=	100000,
			double				epsilon			=	1e-6,
			double				areaEpsilon		=	3e-5,
			int					maxEdgeFlips	=	10000,
			int					settings		=	CLIPPING
		);

		~DelaunayTriangulation(void);

		// this removes the boundary vertices so that we can simplify to working points
		// adding working points after this may be problematic
		void removeBoundaryVerts(void);

		int error(void) const;

/******************************************************************************************************
	BOUNDARY QUERIES
*******************************************************************************************************/

//!	\return Returns a Point with the lowest x and y of this DelaunayTriangualtion. Effectively the lower envelope.
		Point GetLowerBound(void);

//!	\return Returns a Point with the highest x and y of this DelaunayTriangulation. Effectively the upper envelope.
		Point GetUpperBound(void);

//!	\return Returns a PointList of all 4 points ordered counter-clockwise starting at the bottom left.
		PointList GetBoundary(void);

/******************************************************************************************************
	SETTINGS CONTROL
*******************************************************************************************************/

//!	\return Returns the current epsilon used by this DelaunayTriangulation.
		double GetEpsilon(void) const;

//!	\brief Set the current epsilon used by this DelaunayTriangulation.
		void SetEpsilon(double newEpsilon);

//!	\brief Returns the current epsilon used to define a collapsed triangle.
		double GetAreaEpsilon(void) const;

//!	\brief Set the current epsilon used to define a collapsed triangle.
		void SetAreaEpsilon(double newEpsilon);

//!	\return Returns the maximum number of Edge flips allowed per retriangulation in this DelaunayTriangulation.
		int GetMaxEdgeFlips(void) const;

//!	\brief Set the maximum number of Edge flips allowed per retriangulation in this DelaunayTriangulation.
		void SetMaxEdgeFlips(int MaxEdgeFlips);

//!	\brief Check if an Option is enabled by this DelaunayTriangulation.
		bool Enabled(Option option) const;

//!	\brief Enable an Option in this DelaunayTriangulation.
		void Enable(Option option);

//!	\brief Disable an Option in this DelaunayTriangulation.
		void Disable(Option option);

/******************************************************************************************************
	SIZE QUERIES
*******************************************************************************************************/

//!	\return Return the number of Edges in this DelaunayTriangulation.
		int	GetNumEdges(void);

//!	\return Return the number of Constrained Edges in this DelaunayTriangulation.
		int	GetNumConstrainedEdges(void);

//!	\return Return the number of Vertices in this DelaunayTriangulation.
		int	GetNumVertices(void);

//!	\return Return the number of Constrained Vertices in this DelaunayTriangulation.
		int GetNumConstrainedVertices(void);

//!	\return Return the number of triangles in this DelaunayTriangulation.
//!	Uses a simlpe base computation that is O(1) time to compute an approximation. In order to be exact,
//!	the number of boundary edges must be counted, which causes a O(E) check. Avoid calling frequently without rough
		int GetNumTriangles(bool rough = true);

/******************************************************************************************************
	POINT QUERIES
*******************************************************************************************************/

//!	\return The nearest Point in this DelaunayTriagulation. Returns the same point is outside of this DelaunayTriangulation
		Point GetNearestPoint(Point point);

//!    \return The z value of this Point within this DelaunayTriangulation. Interpolated based on the closest Edge or Face. Default if outside
		double GetZValue(Point point, double outsideZ = 0);

/******************************************************************************************************
	INSERTION AND REMOVAL
*******************************************************************************************************/
	protected:
		ID InsertConstrainedLineStringPrivate(const PointList& constraint);
		ID InsertConstrainedPolygonPrivate(const PointList& constraint);

	public:
//!	\brief Inserts a new Point into the DelaunayTriangulation. A Working Point might be "simplified" away and thus its lifetime is unknown.
		void InsertWorkingPoint(Point point);

//!	\brief Remove all working points that are inside the given polygon. Removes ALL working points in the polygon is empty.
		void RemoveWorkingPoints(const PointList &polygon);

/*!	\brief Inserts a new Point into the DelaunayTriangulation and Constrains the Point.
	\return ConstraintID of the inserted Point. 0 if the point could not be inserted.
*/
		ID InsertConstrainedPoint(Point point);

/*!	\brief Inserts a new cosntrained line string list into the DelaunayTriangulation.

	A constrained line string is defined as a collection of ordered points and the segments created by their ordering.
	Also adds any needed intersection Points if and only if this constrained line string intersects another already 
	inserted in the DelaunayTriangulation. The intersection will be handled using the specified Z Interpolation Settings.

	If any portion of the constrained line string is outside of the DelaunayTriangulation, the constrained line string will
	be handled using the Constraint Clipping Settings.
	\param constraint PointList representing constrained line string to insert.
	\return A unique ID representing the newly inserted constraint. 0 if the constraint failed to be inserted for some reason.
*/
		ID InsertConstrainedLineString(PointList constraint);

/*!	\brief Inserts a new constrained polygon into the DelaunayTriangulation.

	A constrained polygon is defined as a closed and simple collection of ordered points. This method is similar to inserting
	a constrained line string, only after the boundary of the polygon is inserted, all points found to be inside the polygon
	will be either removed or have their z values interpolated and forced by this polygon. 
	\param constraint PointList representing constrained polygon to insert (the first and last point should be equal).
	\return A unique ID representing the newly inserted constraint. 0 if the constraint failed to be inserted for some reason.
*/
		ID InsertConstrainedPolygon(PointList constraint);

/*!	\brief Removes a constraint with the ID given. 

	Will do nothing if no constraint with the given ID is found.
	\param constraintID ID of the constraint to remove from this DelaunayTriangulation.
*/
		void RemoveConstraint(ID constraintID);

/******************************************************************************************************
	DATA STRUCTURE EXPORTING
*******************************************************************************************************/
	public:
//!	\return Returns the ConstraintMap of this DelaunayTriangulation.
		const ConstraintMap* GetConstraintMap(void) const;

//!	\return Returns the Subdivision of this DelaunayTriangulation.
		const Subdivision* GetSubdivision(void) const;

/******************************************************************************************************
	NEIGHBOOR OPERATIONS
*******************************************************************************************************/
	public:
/*!	\brief Link two DelaunayTriangulations together along their boundaries.

	Two Neighboor boundaries can only be linked if they both contain identical boundaries.
		-	The boundaries extend along the same line through space (including z values).
		-	The boundaries must have the same points along each other (including z values).

	Once linked, any changes along the boundary of one DelaunayTriangulation will be reflected in the other.
	Note that this also means that a working point will never be deleted from one DelaunayTriangulation if it
	is a Constrained point of the other DelaunayTriangulation.

	Destroying a DelaunayTriangulation will automatically undo any links that have been created to it.
	\param location Location on this triangulation to link.
	\param dt DelaunayTriangulation to link to this one.
	\param dt_location Location on other triangulation to link to.
	\return TRUE if linking was a success, FALSE otherwise.
*/
		bool LinkNeighbor(size_t location, DelaunayTriangulation* dt, size_t dt_location);

//!	\brief Remove any neighbor linking along the passed location.
		void UnlinkNeighbor(size_t Location);

//!	\return TRUE if the Vertex lies on the boundary.
		bool IsBoundaryVertex(Vertex* vert);

//!	\return TRUE if the Vertex is on the passed boundary. (If NONE, TRUE is returned).
		bool IsBoundaryVertex(Vertex* vert, size_t Location);

//!	\return Returns TRUE if an Edge is part of the boundary.
		bool IsBoundaryEdge(Edge* edge);

//!	\return Returns TRUE if an Edge is part of the passed boundary. (If NONE, TRUE is returned).
		bool IsBoundaryEdge(Edge* edge, size_t Location);

//!	\return Returns the true Vertex normal of a Vertex in this DelaunayTriangulation. Will factor in linked DelaunayTriangulations if present.
		Vector GetVertexNormal(Vertex* vert);

/******************************************************************************************************
	POINT AND TRIANGLE GATHERING ALGORITHMS
*******************************************************************************************************/
	protected:
		void GatherConstraintEdges(Edge* edge, std::vector<Edge*>& edges, bool* marked, ID constraintID);
//!  \brief Collects all the edges that cross a line between vertics a and b
//         It can select constrained and/or unconstrained edges
		std::vector<Edge*> GetCrossingEdges(Vertex* a, Vertex* b, bool bound, bool unbound);
		std::vector<Vertex*> GetTouchingVerts(Vertex* a, Vertex* b);

//!  \brief Collects all the vertices, it can select the working and/or the constrained vertices.
		std::vector<Vertex*> GatherVertsLocal(const PointList &polygon, bool constrained, bool working);
	public:

//!	\return Returns all vertices. If a non-empty polygon is given, only vertices inside it will be returned.
		std::vector<Vertex*> GatherVerts(PointList polygon);

//!	\return Returns all working Points. If a non-empty polygon is given, only vertices inside it will be returned.
		std::vector<Vertex*> GatherWorkingVerts(PointList polygon);

//!	\return Returns all bound vertices. If a non-empty polygon is given, only vertices inside it will be returned.
		std::vector<Vertex*> GatherConstrainedVerts(PointList polygon);

//! \return Returns all triangles. If a non-empty polygon is given, only triangles with centroids inside the polygon will be returned. (Triangles are on the left of each Edge)
		std::vector<Edge*> GatherTriangles(PointList polygon);

/******************************************************************************************************
	BOOLEAN FUNCTIONS
*******************************************************************************************************/
	protected:
		bool IsRight(Vector vert, Vector a, Vector b);
		bool IsRight(Vector vert, Edge* edge);
		bool IsLeft(Vector vert, Vector a, Vector b);
		bool IsLeft(Vector vert, Edge* edge);
		int	IsOn(Vector vert, Vector p0, Vector p1);
		int IsOn(Vector vert, Edge* edge);
		bool InCircle(Vector a, Vector b, Vector c, Vector d);
		bool IsFlipValid(Edge* edge);
		bool IsDelaunay(Vector p, Edge* edge);
		bool IsInside(Point point);
		bool IsInside(const PointList &points);
		bool Touches(const PointList &points);

/******************************************************************************************************
	SUBDIVISION INTERFACE
*******************************************************************************************************/
		Edge* Connect(Vertex* a, Vertex* b);
		Edge* GetConnection(Vertex* a, Vertex* b);
		Edge* GetLeftEdge(Vertex* a, Vector p);
		void Swap(Edge* edge);
		void RemoveEdge(Edge* edge);
		void RemoveVertex(Vertex* vert);

/******************************************************************************************************
	SEARCH AND INTERSECTION
*******************************************************************************************************/
		LocationResult LocatePoint(Point p, Edge* edge = NULL);
		Point Intersection(Edge* edge, Point c, Point d);
		void InterpolateZ(Vector O, Vector U, Vector V, Vertex* vert);
		
/******************************************************************************************************
	INSERTION
*******************************************************************************************************/
		Vertex* SnapPointToVertex(const Point& p);
		Vertex* SnapPointToEdge(const Point& p);
		Vertex* InsertPoint(Point p);
		Vertex*	InsertPointInEdge(Point p, Edge* edge);
		Vertex*	InsertPointInBoundary(Point p, Edge* edge);
		Vertex*	InsertPointInFace(Point p, Edge* edge);
		void SimplifyEdges(Vertex* vert);
		bool IsSimplifyValid(Vertex* vert);
		void FlipEdges(Point p, Edge* base);
		void InsertSegment(Vertex* a, Vertex* b, ID constraintID);
		void RetriangulateFace(Edge* base);

/******************************************************************************************************
	PROJECTION
*******************************************************************************************************/
	public:
//! Transforms a point from the global coordinate system to the local coordinate system
		Point TransformPointToLocal(const Point& p) const;

//!	Transforms a point from the local coordinate system to the global coordinate system
		Point TransformPointToGlobal(const Point& p) const;

/******************************************************************************************************
	Mesh Simplification
*******************************************************************************************************/
	private:
		float ComputeVertexError(Vertex* vert) const;

	public:
//!	Simplify this DelaunayTriangulation by removing working points until the targetFaces have been reached
//!	or until no other points can be removed. (No points with a saliency greater than threshold will be removed).
		void Simplify(int targetFaces, float threshold);

	};

}
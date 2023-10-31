# DelaunayTriangulator
Provides a Dynamically Constrained Delaunay Triangulation system. Based on an incremental Delaunay Triangulation algorithm with the added feature of constraint insertion and removal. Capable of creating a point only Delaunay Triangulation in O(n<sup>4/3</sup>) time ( provided the points are inserted randomly - worst case time is n<sup>2</sup> and is encounted when inserting points of a grid in order, to counter this worst case behaviour an even faster algorithm for generating DelaunayTringulations from a grid is provided).

## License
The Delaunay Triangulator is provided under the [MIT License](#license-text)

## DelaunayTriangulation Construction:
When creating a DelaunayTriangulation a few options must be specified.

|Option|Description|
|------|-----------------------------------------------------------------|
|BoundingRegion|A DelaunayTriangulation must be initially constrainted to some bounding region (in this implementation a quad).<ul><li>This must be specified by the user upon construction.</li><li>The user can specify 4 points of a bounding QUAD upon construction.</li><li>The user can specify a CGrid (which contains a bounding QUAD) upon construction.</li><li>Defaults to (NO DEFAULT - MUST BE SPECIFIED).</li><li>Must be counter-clockwise in orientation.</li></ul>|
|DataBlockSize|The DelaunayTriangulation needs to know how large to make new Vertex and Edge data blocks if more Edges or Verts are needed.<br>This value will set the size (as in number of Edges or Verts to a block, not the number of bytes) per block. A larger size per block will result in fewer blocks overall and thus less time spent on memory management, while a smaller block size will result in less unused allocated memory at any given time, but result in longer processing times since more blocks must be managed.<br>**Default to (100)**
|Epsilon|A custom epsilon can be defined for comparisons in this DelaunayTriangulation. If the epsilon is set too small, some rounding errors can lead to crashes or infinite loops. The default has been tested to work for a large number of cases. If undesired results are present, try increasing the epsilon slightly.<br>**Default to (3e-13)**|
|MaxEdgeFlips|The maximum number of Edge flips that will be performed for any call to FlipEdges by the DelaunayTriangulation. This is important because a Constrained Delaunay Triangulation is not garunteed to be Delaunay since many Edges are restricted from Edge flips and no extra Vertices (other than intersection points) are created. With this in mind it is possible for the EdgeFlips to ripple outward from a new constraint for many itterations (which get increasingly worse with a DelaunayTriangulation with many Edges). To avoid this, MaxEdgeFlips is specified which will constrain the number of flips the DelaunayTriangulation will use in an attempt to correct the triangulation after inserting a new constraint. <br>**Defaults to (10000)**|
|Options|Additional Options may be specified or enabled for this DelaunayTriangulation (several options may be &#124; together). The Options allowed are:<table><tbody><tr><td>CLIPPING	</td><td>When Enabled, if a portion of a constraint lies outside of the bounding region, the DelaunayTriangulation will first try clipping the constraint to see if any portion is inside the DelaunayTriangulation. When Disabled, if ANY portion of the constraint is outside of the bouning region, the entire constraint will be ignored.</td></tr><tr><td>FLATTENING</td><td>When Enabled, all Z values of points added to this Triangulation will become 0.</td></tr><tr><td>INTERPOLATE_EDGES</td><td>When Enabled, any intersecting constrained Edges will create a new point with a Z value intepolated by the currently inserted Edge.</td></tr><tr><td>INTERPOLATE_FACES</td><td>When Enabled, any Point inserted into a face, will have its Z value adjusted to match the face.</td></tr></tbody></table>|

## Constraints:
A constraint is defined as a single Point or a single list of Points that form a line string. When a constraint is inserted into the DT it will force all the segments of the constraint to appear in the DT regardless if they are Delaunay or not. If two constraints are found to intersect or overlap one another, a new Vertex point will be generated at the intersection and placed into the DT to ensure that both constraints are satisfied. Note that this new Vertex will have a z value determined based on the ZInterpolationOption settings. Upon removing one of the overlapping constraints, if it is found that the newly created Vertex is no longer necessary, the Vertex will be removed and the DelaunayTriangulation will be simplified.

###	Insertion

|Constraint Type|Description|
|---------------|-------------------------------------------|
|WorkingPoint|When a working point is inserted, it is a temporary point that will only be gaurunteed to last until the next insertion or removal.|
|ConstrainedPoint|When a constrained Point is inserted, the only way that point will ever be removed is by calling RemoveConstraint with its unique ID. Note that ID values are unique even between Point constraints and LineString constraints, so no ID assigned to a Point will ever be given to a LineString or vice versa.|
|ConstrainedLineString|A constrained LineString inserts a series of constrained Edges into the DelaunayTriangualtion. The Z values of the constraints are handled by the settings.|
|ConstrainedPolygon|A constrained Polygon inserts a series of constrained Edges that form a single closed/simple area. After inserting a Polygon, all points and edges inside the Polygon are simplified and removed if possible or else interpolated to the Z value of the Polygon by force. In this regard, the Z interpolation settings are disregarded in this method and the Z values of the Polygon are used always. This method is great for inserting roads	if they already have the z values you want.|

### ConstraintID values 
When inserting a Constraint, an ID value will always be returned. If this ID is 0, the insertion failed for some reason (the constraint may be outside of this DelaunayTriangulation). The ID 0 is reserved internally for the boundary of the DelaunayTriangulation and will never be assigned to any constraint	that a user has any control over. ID values are unique within this DelaunayTriangulation. No ID assigned to a Point will be shared by any other Point OR LineString OR Polygon. Because of this	there is only one method to remove a constraint. It will determine what type the constraint is based on the ID.
	
 ## Exporting the Data from a DelaunayTriangulation
A DelaunayTriangulation is capable of exporting the triangulation at any given moment as either a Subdivision or TIN.
When retrieving the Subdivision of a DelaunayTriangulation, the actual raw data-structure used by the DelaunayTriangulation system itself will be returned.
This data is fragile and should not be changed outside the scope of the DelaunayTriangulation class itself. If using the Subdivision of a DelaunayTriangulation, be careful not to change and data within a Vertex or Edge or the DelaunayTriangulation might (and most probably will) break.

A TIN can also be retrieved. Note that all needed data is copied to the TIN and therefore no access to the original Subdivision is given to the TIN. A TIN can be used if no topology information is needed. All normals are computed before hand for the TIN while the data is still within the Subdivision. See ctl::TIN for more information on Creating a TIN out of a DelaunayTriangulation.
	
## Linking multiple DelaunayTriangulations together
An interface for linking multiple neighbooring DelaunayTriangulations is provided. Note that no linker passes ownership and no memory is shared by either DelaunayTriangulation (meaning no neighboors are deleted upong this DelaunayTriangulations destruction). Each DelaunayTriangulation has a left, right, up and down neighboor. These neighboors are used when computing vertex normals.
		
## Notes
This implementation uses the quad-edge data structure presented by Guibas and Stolfi (1985) (See ctl::Subdivision and ctl::Edge for more information), and used Graphics Gems IV (Paul S. Heckbert) as a reference for some of the basic Delaunay Triangulation code, while referencing a paper by Marcelo Kallmann for strategies and algorithms for handling constrained edges within a DelaunayTriangulation.

A DelaunayTriangualtion cannot be copied or assigned due to the delicate data structures that hold the Edges and Vertices. No shared or reference pointer is typedefed for a DelaunayTriangulation, but it is best to create some form of managed pointer if you plan on using a DelaunayTriangulation outside of a single scope.
## Usage Examples:
  
```cpp
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
```

## License Text

**Copyright 2017, Cognitics Inc.**

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

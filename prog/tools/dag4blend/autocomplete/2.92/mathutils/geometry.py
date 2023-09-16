import sys
import typing
import mathutils


def area_tri(v1: 'mathutils.Vector', v2: 'mathutils.Vector',
             v3: 'mathutils.Vector'):
    ''' Returns the area size of the 2D or 3D triangle defined.

    :param v1: Point1
    :type v1: 'mathutils.Vector'
    :param v2: Point2
    :type v2: 'mathutils.Vector'
    :param v3: Point3
    :type v3: 'mathutils.Vector'
    '''

    pass


def barycentric_transform(
        point: 'mathutils.Vector', tri_a1: 'mathutils.Vector',
        tri_a2: 'mathutils.Vector', tri_a3: 'mathutils.Vector',
        tri_b1: 'mathutils.Vector', tri_b2: 'mathutils.Vector',
        tri_b3: 'mathutils.Vector') -> 'mathutils.Vector':
    ''' Return a transformed point, the transformation is defined by 2 triangles.

    :param point: The point to transform.
    :type point: 'mathutils.Vector'
    :param tri_a1: source triangle vertex.
    :type tri_a1: 'mathutils.Vector'
    :param tri_a2: source triangle vertex.
    :type tri_a2: 'mathutils.Vector'
    :param tri_a3: source triangle vertex.
    :type tri_a3: 'mathutils.Vector'
    :param tri_b1: target triangle vertex.
    :type tri_b1: 'mathutils.Vector'
    :param tri_b2: target triangle vertex.
    :type tri_b2: 'mathutils.Vector'
    :param tri_b3: target triangle vertex.
    :type tri_b3: 'mathutils.Vector'
    :return: The transformed point
    '''

    pass


def box_fit_2d(points: list) -> float:
    ''' Returns an angle that best fits the points to an axis aligned rectangle

    :param points: list of 2d points.
    :type points: list
    :return: angle
    '''

    pass


def box_pack_2d(boxes: list) -> tuple:
    ''' Returns a tuple with the width and height of the packed bounding box.

    :param boxes: list of boxes, each box is a list where the first 4 items are [x, y, width, height, ...] other items are ignored.
    :type boxes: list
    :return: the width and height of the packed bounding box
    '''

    pass


def closest_point_on_tri(pt: 'mathutils.Vector', tri_p1: 'mathutils.Vector',
                         tri_p2: 'mathutils.Vector',
                         tri_p3: 'mathutils.Vector') -> 'mathutils.Vector':
    ''' Takes 4 vectors: one is the point and the next 3 define the triangle.

    :param pt: Point
    :type pt: 'mathutils.Vector'
    :param tri_p1: First point of the triangle
    :type tri_p1: 'mathutils.Vector'
    :param tri_p2: Second point of the triangle
    :type tri_p2: 'mathutils.Vector'
    :param tri_p3: Third point of the triangle
    :type tri_p3: 'mathutils.Vector'
    :return: The closest point of the triangle.
    '''

    pass


def convex_hull_2d(points: list) -> list:
    ''' Returns a list of indices into the list given

    :param points: list of 2d points.
    :type points: list
    :return: a list of indices
    '''

    pass


def delaunay_2d_cdt(
        vert_coords: typing.List['mathutils.Vector'], edges: typing.List[int],
        faces: typing.List[int], output_type, epsilon: float
) -> typing.Union[typing.List[int], typing.List['mathutils.Vector']]:
    ''' Computes the Constrained Delaunay Triangulation of a set of vertices, with edges and faces that must appear in the triangulation. Some triangles may be eaten away, or combined with other triangles, according to output type. The returned verts may be in a different order from input verts, may be moved slightly, and may be merged with other nearby verts. The three returned orig lists give, for each of verts, edges, and faces, the list of input element indices corresponding to the positionally same output element. For edges, the orig indices start with the input edges and then continue with the edges implied by each of the faces (n of them for an n-gon).

    :param vert_coords: Vertex coordinates (2d)
    :type vert_coords: typing.List['mathutils.Vector']
    :param edges: Edges, as pairs of indices in vert_coords
    :type edges: typing.List[int]
    :param faces: Faces, each sublist is a face, as indices in vert_coords (CCW oriented)
    :type faces: typing.List[int]
    :param output_type: What output looks like. 0 => triangles with convex hull. 1 => triangles inside constraints. 2 => the input constraints, intersected. 3 => like 2 but with extra edges to make valid BMesh faces.
    :type epsilon: float
    :return: Output tuple, (vert_coords, edges, faces, orig_verts, orig_edges, orig_faces)
    '''

    pass


def distance_point_to_plane(pt: 'mathutils.Vector',
                            plane_co: 'mathutils.Vector',
                            plane_no: 'mathutils.Vector'):
    ''' Returns the signed distance between a point and a plane (negative when below the normal).

    :param pt: Point
    :type pt: 'mathutils.Vector'
    :param plane_co: A point on the plane
    :type plane_co: 'mathutils.Vector'
    :param plane_no: The direction the plane is facing
    :type plane_no: 'mathutils.Vector'
    '''

    pass


def interpolate_bezier(knot1: 'mathutils.Vector', handle1: 'mathutils.Vector',
                       handle2: 'mathutils.Vector', knot2: 'mathutils.Vector',
                       resolution: int) -> 'mathutils.Vector':
    ''' Interpolate a bezier spline segment.

    :param knot1: First bezier spline point.
    :type knot1: 'mathutils.Vector'
    :param handle1: First bezier spline handle.
    :type handle1: 'mathutils.Vector'
    :param handle2: Second bezier spline handle.
    :type handle2: 'mathutils.Vector'
    :param knot2: Second bezier spline point.
    :type knot2: 'mathutils.Vector'
    :param resolution: Number of points to return.
    :type resolution: int
    :return: The interpolated points
    '''

    pass


def intersect_line_line(v1: 'mathutils.Vector', v2: 'mathutils.Vector',
                        v3: 'mathutils.Vector', v4: 'mathutils.Vector'):
    ''' Returns a tuple with the points on each line respectively closest to the other.

    :param v1: First point of the first line
    :type v1: 'mathutils.Vector'
    :param v2: Second point of the first line
    :type v2: 'mathutils.Vector'
    :param v3: First point of the second line
    :type v3: 'mathutils.Vector'
    :param v4: Second point of the second line
    :type v4: 'mathutils.Vector'
    '''

    pass


def intersect_line_line_2d(lineA_p1: 'mathutils.Vector',
                           lineA_p2: 'mathutils.Vector',
                           lineB_p1: 'mathutils.Vector',
                           lineB_p2: 'mathutils.Vector') -> 'mathutils.Vector':
    ''' Takes 2 segments (defined by 4 vectors) and returns a vector for their point of intersection or None.

    :param lineA_p1: First point of the first line
    :type lineA_p1: 'mathutils.Vector'
    :param lineA_p2: Second point of the first line
    :type lineA_p2: 'mathutils.Vector'
    :param lineB_p1: First point of the second line
    :type lineB_p1: 'mathutils.Vector'
    :param lineB_p2: Second point of the second line
    :type lineB_p2: 'mathutils.Vector'
    :return: The point of intersection or None when not found
    '''

    pass


def intersect_line_plane(line_a: 'mathutils.Vector',
                         line_b: 'mathutils.Vector',
                         plane_co: 'mathutils.Vector',
                         plane_no: 'mathutils.Vector',
                         no_flip=False) -> 'mathutils.Vector':
    ''' Calculate the intersection between a line (as 2 vectors) and a plane. Returns a vector for the intersection or None.

    :param line_a: First point of the first line
    :type line_a: 'mathutils.Vector'
    :param line_b: Second point of the first line
    :type line_b: 'mathutils.Vector'
    :param plane_co: A point on the plane
    :type plane_co: 'mathutils.Vector'
    :param plane_no: The direction the plane is facing
    :type plane_no: 'mathutils.Vector'
    :return: The point of intersection or None when not found
    '''

    pass


def intersect_line_sphere(line_a: 'mathutils.Vector',
                          line_b: 'mathutils.Vector',
                          sphere_co: 'mathutils.Vector',
                          sphere_radius,
                          clip=True) -> 'mathutils.Vector':
    ''' Takes a line (as 2 points) and a sphere (as a point and a radius) and returns the intersection

    :param line_a: First point of the line
    :type line_a: 'mathutils.Vector'
    :param line_b: Second point of the line
    :type line_b: 'mathutils.Vector'
    :param sphere_co: The center of the sphere
    :type sphere_co: 'mathutils.Vector'
    :param sphere_radius: Radius of the sphere
    :return: The intersection points as a pair of vectors or None when there is no intersection
    '''

    pass


def intersect_line_sphere_2d(line_a: 'mathutils.Vector',
                             line_b: 'mathutils.Vector',
                             sphere_co: 'mathutils.Vector',
                             sphere_radius,
                             clip=True) -> 'mathutils.Vector':
    ''' Takes a line (as 2 points) and a sphere (as a point and a radius) and returns the intersection

    :param line_a: First point of the line
    :type line_a: 'mathutils.Vector'
    :param line_b: Second point of the line
    :type line_b: 'mathutils.Vector'
    :param sphere_co: The center of the sphere
    :type sphere_co: 'mathutils.Vector'
    :param sphere_radius: Radius of the sphere
    :return: The intersection points as a pair of vectors or None when there is no intersection
    '''

    pass


def intersect_plane_plane(
        plane_a_co: 'mathutils.Vector', plane_a_no: 'mathutils.Vector',
        plane_b_co: 'mathutils.Vector',
        plane_b_no: 'mathutils.Vector') -> 'mathutils.Vector':
    ''' Return the intersection between two planes

    :param plane_a_co: Point on the first plane
    :type plane_a_co: 'mathutils.Vector'
    :param plane_a_no: Normal of the first plane
    :type plane_a_no: 'mathutils.Vector'
    :param plane_b_co: Point on the second plane
    :type plane_b_co: 'mathutils.Vector'
    :param plane_b_no: Normal of the second plane
    :type plane_b_no: 'mathutils.Vector'
    :return: The line of the intersection represented as a point and a vector
    '''

    pass


def intersect_point_line(pt: 'mathutils.Vector', line_p1: 'mathutils.Vector',
                         line_p2):
    ''' Takes a point and a line and returns a tuple with the closest point on the line and its distance from the first point of the line as a percentage of the length of the line.

    :param pt: Point
    :type pt: 'mathutils.Vector'
    :param line_p1: First point of the line Second point of the line
    :type line_p1: 'mathutils.Vector'
    :param line_p1: First point of the line Second point of the line
    :type line_p1: 'mathutils.Vector'
    '''

    pass


def intersect_point_quad_2d(
        pt: 'mathutils.Vector', quad_p1: 'mathutils.Vector',
        quad_p2: 'mathutils.Vector', quad_p3: 'mathutils.Vector',
        quad_p4: 'mathutils.Vector'):
    ''' Takes 5 vectors (using only the x and y coordinates): one is the point and the next 4 define the quad, only the x and y are used from the vectors. Returns 1 if the point is within the quad, otherwise 0. Works only with convex quads without singular edges.

    :param pt: Point
    :type pt: 'mathutils.Vector'
    :param quad_p1: First point of the quad
    :type quad_p1: 'mathutils.Vector'
    :param quad_p2: Second point of the quad
    :type quad_p2: 'mathutils.Vector'
    :param quad_p3: Third point of the quad
    :type quad_p3: 'mathutils.Vector'
    :param quad_p4: Fourth point of the quad
    :type quad_p4: 'mathutils.Vector'
    '''

    pass


def intersect_point_tri(pt: 'mathutils.Vector', tri_p1: 'mathutils.Vector',
                        tri_p2: 'mathutils.Vector',
                        tri_p3: 'mathutils.Vector') -> 'mathutils.Vector':
    ''' Takes 4 vectors: one is the point and the next 3 define the triangle. Projects the point onto the triangle plane and checks if it is within the triangle.

    :param pt: Point
    :type pt: 'mathutils.Vector'
    :param tri_p1: First point of the triangle
    :type tri_p1: 'mathutils.Vector'
    :param tri_p2: Second point of the triangle
    :type tri_p2: 'mathutils.Vector'
    :param tri_p3: Third point of the triangle
    :type tri_p3: 'mathutils.Vector'
    :return: Point on the triangles plane or None if its outside the triangle
    '''

    pass


def intersect_point_tri_2d(pt: 'mathutils.Vector', tri_p1: 'mathutils.Vector',
                           tri_p2: 'mathutils.Vector',
                           tri_p3: 'mathutils.Vector'):
    ''' Takes 4 vectors (using only the x and y coordinates): one is the point and the next 3 define the triangle. Returns 1 if the point is within the triangle, otherwise 0.

    :param pt: Point
    :type pt: 'mathutils.Vector'
    :param tri_p1: First point of the triangle
    :type tri_p1: 'mathutils.Vector'
    :param tri_p2: Second point of the triangle
    :type tri_p2: 'mathutils.Vector'
    :param tri_p3: Third point of the triangle
    :type tri_p3: 'mathutils.Vector'
    '''

    pass


def intersect_ray_tri(v1: 'mathutils.Vector',
                      v2: 'mathutils.Vector',
                      v3: 'mathutils.Vector',
                      ray: 'mathutils.Vector',
                      orig: 'mathutils.Vector',
                      clip: bool = True) -> 'mathutils.Vector':
    ''' Returns the intersection between a ray and a triangle, if possible, returns None otherwise.

    :param v1: Point1
    :type v1: 'mathutils.Vector'
    :param v2: Point2
    :type v2: 'mathutils.Vector'
    :param v3: Point3
    :type v3: 'mathutils.Vector'
    :param ray: Direction of the projection
    :type ray: 'mathutils.Vector'
    :param orig: Origin
    :type orig: 'mathutils.Vector'
    :param clip: When False, don't restrict the intersection to the area of the triangle, use the infinite plane defined by the triangle.
    :type clip: bool
    :return: The point of intersection or None if no intersection is found
    '''

    pass


def intersect_sphere_sphere_2d(p_a: 'mathutils.Vector', radius_a: float,
                               p_b: 'mathutils.Vector', radius_b: float):
    ''' Returns 2 points on between intersecting circles.

    :param p_a: Center of the first circle
    :type p_a: 'mathutils.Vector'
    :param radius_a: Radius of the first circle
    :type radius_a: float
    :param p_b: Center of the second circle
    :type p_b: 'mathutils.Vector'
    :param radius_b: Radius of the second circle
    :type radius_b: float
    '''

    pass


def intersect_tri_tri_2d(tri_a1, tri_a2, tri_a3, tri_b1, tri_b2, tri_b3):
    ''' Check if two 2D triangles intersect.

    '''

    pass


def normal(vectors: list):
    ''' Returns the normal of a 3D polygon.

    :param vectors: Vectors to calculate normals with
    :type vectors: list
    '''

    pass


def points_in_planes(planes: typing.List['mathutils.Vector']):
    ''' Returns a list of points inside all planes given and a list of index values for the planes used.

    :param planes: List of planes (4D vectors).
    :type planes: typing.List['mathutils.Vector']
    :return: two lists, once containing the vertices inside the planes, another containing the plane indices used
    '''

    pass


def tessellate_polygon(veclist_list):
    ''' Takes a list of polylines (each point a pair or triplet of numbers) and returns the point indices for a polyline filled with triangles. Does not handle degenerate geometry (such as zero-length lines due to consecutive identical points).

    '''

    pass


def volume_tetrahedron(v1: 'mathutils.Vector', v2: 'mathutils.Vector',
                       v3: 'mathutils.Vector', v4: 'mathutils.Vector'):
    ''' Return the volume formed by a tetrahedron (points can be in any order).

    :param v1: Point1
    :type v1: 'mathutils.Vector'
    :param v2: Point2
    :type v2: 'mathutils.Vector'
    :param v3: Point3
    :type v3: 'mathutils.Vector'
    :param v4: Point4
    :type v4: 'mathutils.Vector'
    '''

    pass

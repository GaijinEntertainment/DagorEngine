import sys
import typing
import bmesh.types


def edge_rotate(edge: 'bmesh.types.BMEdge',
                ccw: bool = False) -> 'bmesh.types.BMEdge':
    ''' Rotate the edge and return the newly created edge. If rotating the edge fails, None will be returned.

    :param edge: The edge to rotate.
    :type edge: 'bmesh.types.BMEdge'
    :param ccw: When True the edge will be rotated counter clockwise.
    :type ccw: bool
    :return: The newly rotated edge.
    '''

    pass


def edge_split(edge: 'bmesh.types.BMEdge', vert: 'bmesh.types.BMVert',
               fac: float) -> tuple:
    ''' Split an edge, return the newly created data.

    :param edge: The edge to split.
    :type edge: 'bmesh.types.BMEdge'
    :param vert: One of the verts on the edge, defines the split direction.
    :type vert: 'bmesh.types.BMVert'
    :param fac: The point on the edge where the new vert will be created [0 - 1].
    :type fac: float
    :return: The newly created (edge, vert) pair.
    '''

    pass


def face_flip(faces):
    ''' Flip the faces direction.

    :param face: Face to flip.
    :type face: 'bmesh.types.BMFace'
    '''

    pass


def face_join(faces: 'bmesh.types.BMFace',
              remove: bool = True) -> 'bmesh.types.BMFace':
    ''' Joins a sequence of faces.

    :param faces: Sequence of faces.
    :type faces: 'bmesh.types.BMFace'
    :param remove: Remove the edges and vertices between the faces.
    :type remove: bool
    :return: The newly created face or None on failure.
    '''

    pass


def face_split(face: 'bmesh.types.BMFace',
               vert_a: 'bmesh.types.BMVert',
               vert_b: 'bmesh.types.BMVert',
               coords: typing.List[float] = (),
               use_exist: bool = True,
               example: 'bmesh.types.BMEdge' = None) -> 'bmesh.types.BMLoop':
    ''' Face split with optional intermediate points.

    :param face: The face to cut.
    :type face: 'bmesh.types.BMFace'
    :param vert_a: First vertex to cut in the face (face must contain the vert).
    :type vert_a: 'bmesh.types.BMVert'
    :param vert_b: Second vertex to cut in the face (face must contain the vert).
    :type vert_b: 'bmesh.types.BMVert'
    :param coords: Optional argument to define points in between *vert_a* and *vert_b*.
    :type coords: typing.List[float]
    :param use_exist: .Use an existing edge if it exists (Only used when *coords* argument is empty or omitted)
    :type use_exist: bool
    :param example: Newly created edge will copy settings from this one.
    :type example: 'bmesh.types.BMEdge'
    :return: The newly created face or None on failure.
    '''

    pass


def face_split_edgenet(face: 'bmesh.types.BMFace',
                       edgenet: 'bmesh.types.BMEdge') -> 'bmesh.types.BMFace':
    ''' Splits a face into any number of regions defined by an edgenet.

    :param face: The face to split. The face to split.
    :type face: 'bmesh.types.BMFace'
    :param face: The face to split. The face to split.
    :type face: 'bmesh.types.BMFace'
    :param edgenet: Sequence of edges.
    :type edgenet: 'bmesh.types.BMEdge'
    :return: The newly created faces.
    '''

    pass


def face_vert_separate(face: 'bmesh.types.BMFace',
                       vert: 'bmesh.types.BMVert') -> 'bmesh.types.BMVert':
    ''' Rip a vertex in a face away and add a new vertex.

    :param face: The face to separate.
    :type face: 'bmesh.types.BMFace'
    :param vert: A vertex in the face to separate.
    :type vert: 'bmesh.types.BMVert'
    :return: The newly created vertex or None on failure.
    '''

    pass


def loop_separate(loop: 'bmesh.types.BMLoop') -> 'bmesh.types.BMVert':
    ''' Rip a vertex in a face away and add a new vertex.

    :param loop: The loop to separate.
    :type loop: 'bmesh.types.BMLoop'
    :return: The newly created vertex or None on failure.
    '''

    pass


def vert_collapse_edge(vert: 'bmesh.types.BMVert',
                       edge: 'bmesh.types.BMEdge') -> 'bmesh.types.BMEdge':
    ''' Collapse a vertex into an edge.

    :param vert: The vert that will be collapsed.
    :type vert: 'bmesh.types.BMVert'
    :param edge: The edge to collapse into.
    :type edge: 'bmesh.types.BMEdge'
    :return: The resulting edge from the collapse operation.
    '''

    pass


def vert_collapse_faces(vert: 'bmesh.types.BMVert', edge: 'bmesh.types.BMEdge',
                        fac: float, join_faces: bool) -> 'bmesh.types.BMEdge':
    ''' Collapses a vertex that has only two manifold edges onto a vertex it shares an edge with.

    :param vert: The vert that will be collapsed.
    :type vert: 'bmesh.types.BMVert'
    :param edge: The edge to collapse into.
    :type edge: 'bmesh.types.BMEdge'
    :param fac: The factor to use when merging customdata [0 - 1].
    :type fac: float
    :param join_faces: When true the faces around the vertex will be joined otherwise collapse the vertex by merging the 2 edges this vertex connects to into one.
    :type join_faces: bool
    :return: The resulting edge from the collapse operation.
    '''

    pass


def vert_dissolve(vert: 'bmesh.types.BMVert') -> bool:
    ''' Dissolve this vertex (will be removed).

    :param vert: The vert to be dissolved.
    :type vert: 'bmesh.types.BMVert'
    :return: True when the vertex dissolve is successful.
    '''

    pass


def vert_separate(vert: 'bmesh.types.BMVert',
                  edges: 'bmesh.types.BMEdge') -> 'bmesh.types.BMVert':
    ''' Separate this vertex at every edge.

    :param vert: The vert to be separated.
    :type vert: 'bmesh.types.BMVert'
    :param edges: The edges to separated.
    :type edges: 'bmesh.types.BMEdge'
    :return: The newly separated verts (including the vertex passed).
    '''

    pass


def vert_splice(vert: 'bmesh.types.BMVert', vert_target: 'bmesh.types.BMVert'):
    ''' Splice vert into vert_target.

    :param vert: The vertex to be removed.
    :type vert: 'bmesh.types.BMVert'
    :param vert_target: The vertex to use.
    :type vert_target: 'bmesh.types.BMVert'
    '''

    pass

import sys
import typing
import bpy.types
import mathutils


class BMDeformVert:
    def clear(self):
        ''' Clears all weights.

        '''
        pass

    def get(self, key: int, default=None):
        ''' Returns the deform weight matching the key or default when not found (matches pythons dictionary function of the same name).

        :param key: The key associated with deform weight.
        :type key: int
        :param default: Optional argument for the value to return if *key* is not found.
        :type default: 
        '''
        pass

    def items(self) -> list:
        ''' Return (group, weight) pairs for this vertex (matching pythons dict.items() functionality).

        :rtype: list
        :return: (key, value) pairs for each deform weight of this vertex.
        '''
        pass

    def keys(self) -> list:
        ''' Return the group indices used by this vertex (matching pythons dict.keys() functionality).

        :rtype: list
        :return: the deform group this vertex uses
        '''
        pass

    def values(self) -> list:
        ''' Return the weights of the deform vertex (matching pythons dict.values() functionality).

        :rtype: list
        :return: The weights that influence this vertex
        '''
        pass


class BMEdge:
    ''' The BMesh edge connecting 2 verts
    '''

    hide: bool = None
    ''' Hidden state of this element.

    :type: bool
    '''

    index: int = None
    ''' Index of this element.

    :type: int
    '''

    is_boundary: bool = None
    ''' True when this edge is at the boundary of a face (read-only).

    :type: bool
    '''

    is_contiguous: bool = None
    ''' True when this edge is manifold, between two faces with the same winding (read-only).

    :type: bool
    '''

    is_convex: bool = None
    ''' True when this edge joins two convex faces, depends on a valid face normal (read-only).

    :type: bool
    '''

    is_manifold: bool = None
    ''' True when this edge is manifold (read-only).

    :type: bool
    '''

    is_valid: bool = None
    ''' True when this element is valid (hasn't been removed).

    :type: bool
    '''

    is_wire: bool = None
    ''' True when this edge is not connected to any faces (read-only).

    :type: bool
    '''

    link_faces: typing.Union[typing.List['BMFace'], 'BMElemSeq'] = None
    ''' Faces connected to this edge, (read-only).

    :type: typing.Union[typing.List['BMFace'], 'BMElemSeq']
    '''

    link_loops: typing.Union[typing.List['BMLoop'], 'BMElemSeq'] = None
    ''' Loops connected to this edge, (read-only).

    :type: typing.Union[typing.List['BMLoop'], 'BMElemSeq']
    '''

    seam: bool = None
    ''' Seam for UV unwrapping.

    :type: bool
    '''

    select: bool = None
    ''' Selected state of this element.

    :type: bool
    '''

    smooth: bool = None
    ''' Smooth state of this element.

    :type: bool
    '''

    tag: bool = None
    ''' Generic attribute scripts can use for own logic

    :type: bool
    '''

    verts: typing.Union[typing.List['BMVert'], 'BMElemSeq'] = None
    ''' Verts this edge uses (always 2), (read-only).

    :type: typing.Union[typing.List['BMVert'], 'BMElemSeq']
    '''

    def calc_face_angle(self, fallback=None) -> float:
        ''' 

        :param fallback: return this when the edge doesn't have 2 faces (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: float
        :return: The angle between 2 connected faces in radians.
        '''
        pass

    def calc_face_angle_signed(self, fallback=None) -> float:
        ''' 

        :param fallback: return this when the edge doesn't have 2 faces (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: float
        :return: The angle between 2 connected faces in radians (negative for concave join).
        '''
        pass

    def calc_length(self) -> float:
        ''' 

        :rtype: float
        :return: The length between both verts.
        '''
        pass

    def calc_tangent(self, loop: 'BMLoop') -> 'mathutils.Vector':
        ''' Return the tangent at this edge relative to a face (pointing inward into the face). This uses the face normal for calculation.

        :param loop: The loop used for tangent calculation.
        :type loop: 'BMLoop'
        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def copy_from(self, other):
        ''' Copy values from another element of matching type.

        '''
        pass

    def hide_set(self, hide: bool):
        ''' Set the hide state. This is different from the *hide* attribute because it updates the selection and hide state of associated geometry.

        :param hide: Hidden or visible.
        :type hide: bool
        '''
        pass

    def normal_update(self):
        ''' Update edges vertex normals.

        '''
        pass

    def other_vert(self, vert: 'BMVert') -> 'BMVert':
        ''' Return the other vertex on this edge or None if the vertex is not used by this edge.

        :param vert: a vert in this edge.
        :type vert: 'BMVert'
        :rtype: 'BMVert'
        :return: The edges other vert.
        '''
        pass

    def select_set(self, select: bool):
        ''' Set the selection. This is different from the *select* attribute because it updates the selection state of associated geometry.

        :param select: Select or de-select.
        :type select: bool
        '''
        pass


class BMEdgeSeq:
    layers: 'BMLayerAccessEdge' = None
    ''' custom-data layers (read-only).

    :type: 'BMLayerAccessEdge'
    '''

    def ensure_lookup_table(self):
        ''' Ensure internal data needed for int subscription is initialized with verts/edges/faces, eg bm.verts[index] . This needs to be called again after adding/removing data in this sequence.

        '''
        pass

    def get(self, verts: 'BMVert', fallback=None) -> 'BMEdge':
        ''' Return an edge which uses the **verts** passed.

        :param verts: Sequence of verts.
        :type verts: 'BMVert'
        :param fallback: 
        :type fallback: 
        :rtype: 'BMEdge'
        :return: The edge found or None
        '''
        pass

    def index_update(self):
        ''' Initialize the index values of this sequence. This is the equivalent of looping over all elements and assigning the index values.

        '''
        pass

    def new(self, verts: 'BMVert', example: 'BMEdge' = None) -> 'BMEdge':
        ''' Create a new edge from a given pair of verts.

        :param verts: Vertex pair.
        :type verts: 'BMVert'
        :param example: Existing edge to initialize settings (optional argument).
        :type example: 'BMEdge'
        :rtype: 'BMEdge'
        :return: The newly created edge.
        '''
        pass

    def remove(self, edge):
        ''' Remove an edge.

        '''
        pass

    def sort(self, key=None, reverse=False):
        ''' Sort the elements of this sequence, using an optional custom sort key. Indices of elements are not changed, BMElemeSeq.index_update() can be used for that.

        :param key: The key that sets the ordering of the elements.
        :type key: 
        :param reverse: Reverse the order of the elements
        :type reverse: 
        '''
        pass


class BMEditSelIter:
    pass


class BMEditSelSeq:
    active: typing.Union['BMEdge', 'BMVert', 'BMFace'] = None
    ''' The last selected element or None (read-only).

    :type: typing.Union['BMEdge', 'BMVert', 'BMFace']
    '''

    def add(self, element):
        ''' Add an element to the selection history (no action taken if its already added).

        '''
        pass

    def clear(self):
        ''' Empties the selection history.

        '''
        pass

    def discard(self, element):
        ''' Discard an element from the selection history. Like remove but doesn't raise an error when the elements not in the selection list.

        '''
        pass

    def remove(self, element):
        ''' Remove an element from the selection history.

        '''
        pass

    def validate(self):
        ''' Ensures all elements in the selection history are selected.

        '''
        pass


class BMElemSeq:
    ''' General sequence type used for accessing any sequence of BMVert , BMEdge , BMFace , BMLoop . When accessed via BMesh.verts , BMesh.edges , BMesh.faces there are also functions to create/remomove items.
    '''

    def index_update(self):
        ''' Initialize the index values of this sequence. This is the equivalent of looping over all elements and assigning the index values.

        '''
        pass


class BMFace:
    ''' The BMesh face with 3 or more sides
    '''

    edges: typing.Union[typing.List['BMEdge'], 'BMElemSeq'] = None
    ''' Edges of this face, (read-only).

    :type: typing.Union[typing.List['BMEdge'], 'BMElemSeq']
    '''

    hide: bool = None
    ''' Hidden state of this element.

    :type: bool
    '''

    index: int = None
    ''' Index of this element.

    :type: int
    '''

    is_valid: bool = None
    ''' True when this element is valid (hasn't been removed).

    :type: bool
    '''

    loops: typing.Union[typing.List['BMLoop'], 'BMElemSeq'] = None
    ''' Loops of this face, (read-only).

    :type: typing.Union[typing.List['BMLoop'], 'BMElemSeq']
    '''

    material_index: int = None
    ''' The face's material index.

    :type: int
    '''

    normal: 'mathutils.Vector' = None
    ''' The normal for this face as a 3D, wrapped vector.

    :type: 'mathutils.Vector'
    '''

    select: bool = None
    ''' Selected state of this element.

    :type: bool
    '''

    smooth: bool = None
    ''' Smooth state of this element.

    :type: bool
    '''

    tag: bool = None
    ''' Generic attribute scripts can use for own logic

    :type: bool
    '''

    verts: typing.Union[typing.List['BMVert'], 'BMElemSeq'] = None
    ''' Verts of this face, (read-only).

    :type: typing.Union[typing.List['BMVert'], 'BMElemSeq']
    '''

    def calc_area(self) -> float:
        ''' Return the area of the face.

        :rtype: float
        :return: Return the area of the face.
        '''
        pass

    def calc_center_bounds(self) -> 'mathutils.Vector':
        ''' Return bounds center of the face.

        :rtype: 'mathutils.Vector'
        :return: a 3D vector.
        '''
        pass

    def calc_center_median(self) -> 'mathutils.Vector':
        ''' Return median center of the face.

        :rtype: 'mathutils.Vector'
        :return: a 3D vector.
        '''
        pass

    def calc_center_median_weighted(self) -> 'mathutils.Vector':
        ''' Return median center of the face weighted by edge lengths.

        :rtype: 'mathutils.Vector'
        :return: a 3D vector.
        '''
        pass

    def calc_perimeter(self) -> float:
        ''' Return the perimeter of the face.

        :rtype: float
        :return: Return the perimeter of the face.
        '''
        pass

    def calc_tangent_edge(self) -> 'mathutils.Vector':
        ''' Return face tangent based on longest edge.

        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def calc_tangent_edge_diagonal(self) -> 'mathutils.Vector':
        ''' Return face tangent based on the edge farthest from any vertex.

        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def calc_tangent_edge_pair(self) -> 'mathutils.Vector':
        ''' Return face tangent based on the two longest disconnected edges. - Tris: Use the edge pair with the most similar lengths. - Quads: Use the longest edge pair. - NGons: Use the two longest disconnected edges.

        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def calc_tangent_vert_diagonal(self) -> 'mathutils.Vector':
        ''' Return face tangent based on the two most distent vertices.

        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def copy(self, verts: bool = True, edges: bool = True) -> 'BMFace':
        ''' Make a copy of this face.

        :param verts: When set, the faces verts will be duplicated too.
        :type verts: bool
        :param edges: When set, the faces edges will be duplicated too.
        :type edges: bool
        :rtype: 'BMFace'
        :return: The newly created face.
        '''
        pass

    def copy_from(self, other):
        ''' Copy values from another element of matching type.

        '''
        pass

    def copy_from_face_interp(self, face: 'BMFace', vert: bool = True):
        ''' Interpolate the customdata from another face onto this one (faces should overlap).

        :param face: The face to interpolate data from.
        :type face: 'BMFace'
        :param vert: When True, also copy vertex data.
        :type vert: bool
        '''
        pass

    def hide_set(self, hide: bool):
        ''' Set the hide state. This is different from the *hide* attribute because it updates the selection and hide state of associated geometry.

        :param hide: Hidden or visible.
        :type hide: bool
        '''
        pass

    def normal_flip(self):
        ''' Reverses winding of a face, which flips its normal.

        '''
        pass

    def normal_update(self):
        ''' Update face's normal.

        '''
        pass

    def select_set(self, select: bool):
        ''' Set the selection. This is different from the *select* attribute because it updates the selection state of associated geometry.

        :param select: Select or de-select.
        :type select: bool
        '''
        pass


class BMFaceSeq:
    active: 'BMFace' = None
    ''' active face.

    :type: 'BMFace'
    '''

    layers: 'BMLayerAccessFace' = None
    ''' custom-data layers (read-only).

    :type: 'BMLayerAccessFace'
    '''

    def ensure_lookup_table(self):
        ''' Ensure internal data needed for int subscription is initialized with verts/edges/faces, eg bm.verts[index] . This needs to be called again after adding/removing data in this sequence.

        '''
        pass

    def get(self, verts: 'BMVert', fallback=None) -> 'BMFace':
        ''' Return a face which uses the **verts** passed.

        :param verts: Sequence of verts.
        :type verts: 'BMVert'
        :param fallback: 
        :type fallback: 
        :rtype: 'BMFace'
        :return: The face found or None
        '''
        pass

    def index_update(self):
        ''' Initialize the index values of this sequence. This is the equivalent of looping over all elements and assigning the index values.

        '''
        pass

    def new(self, verts: 'BMVert', example: 'BMFace' = None) -> 'BMFace':
        ''' Create a new face from a given set of verts.

        :param verts: Sequence of 3 or more verts.
        :type verts: 'BMVert'
        :param example: Existing face to initialize settings (optional argument).
        :type example: 'BMFace'
        :rtype: 'BMFace'
        :return: The newly created face.
        '''
        pass

    def remove(self, face):
        ''' Remove a face.

        '''
        pass

    def sort(self, key=None, reverse=False):
        ''' Sort the elements of this sequence, using an optional custom sort key. Indices of elements are not changed, BMElemeSeq.index_update() can be used for that.

        :param key: The key that sets the ordering of the elements.
        :type key: 
        :param reverse: Reverse the order of the elements
        :type reverse: 
        '''
        pass


class BMIter:
    ''' Internal BMesh type for looping over verts/faces/edges, used for iterating over BMElemSeq types.
    '''

    pass


class BMLayerAccessEdge:
    ''' Exposes custom-data layer attributes.
    '''

    bevel_weight: 'BMLayerCollection' = None
    ''' Bevel weight float in [0 - 1].

    :type: 'BMLayerCollection'
    '''

    color = None
    ''' Generic RGBA color with 8-bit precision custom-data layer. type: BMLayerCollection'''

    crease: 'BMLayerCollection' = None
    ''' Edge crease for subdivision surface - float in [0 - 1].

    :type: 'BMLayerCollection'
    '''

    float = None
    ''' Generic float custom-data layer. type: BMLayerCollection'''

    float_color = None
    ''' Generic RGBA color with float precision custom-data layer. type: BMLayerCollection'''

    float_vector = None
    ''' Generic 3D vector with float precision custom-data layer. type: BMLayerCollection'''

    freestyle = None
    ''' Accessor for Freestyle edge layer. type: BMLayerCollection'''

    int = None
    ''' Generic int custom-data layer. type: BMLayerCollection'''

    string = None
    ''' Generic string custom-data layer (exposed as bytes, 255 max length). type: BMLayerCollection'''


class BMLayerAccessFace:
    ''' Exposes custom-data layer attributes.
    '''

    color = None
    ''' Generic RGBA color with 8-bit precision custom-data layer. type: BMLayerCollection'''

    face_map = None
    ''' FaceMap custom-data layer. type: BMLayerCollection'''

    float = None
    ''' Generic float custom-data layer. type: BMLayerCollection'''

    float_color = None
    ''' Generic RGBA color with float precision custom-data layer. type: BMLayerCollection'''

    float_vector = None
    ''' Generic 3D vector with float precision custom-data layer. type: BMLayerCollection'''

    freestyle = None
    ''' Accessor for Freestyle face layer. type: BMLayerCollection'''

    int = None
    ''' Generic int custom-data layer. type: BMLayerCollection'''

    string = None
    ''' Generic string custom-data layer (exposed as bytes, 255 max length). type: BMLayerCollection'''


class BMLayerAccessLoop:
    ''' Exposes custom-data layer attributes.
    '''

    color = None
    ''' Generic RGBA color with 8-bit precision custom-data layer. type: BMLayerCollection'''

    float = None
    ''' Generic float custom-data layer. type: BMLayerCollection'''

    float_color = None
    ''' Generic RGBA color with float precision custom-data layer. type: BMLayerCollection'''

    float_vector = None
    ''' Generic 3D vector with float precision custom-data layer. type: BMLayerCollection'''

    int = None
    ''' Generic int custom-data layer. type: BMLayerCollection'''

    string = None
    ''' Generic string custom-data layer (exposed as bytes, 255 max length). type: BMLayerCollection'''

    uv = None
    ''' Accessor for BMLoopUV UV (as a 2D Vector). type: BMLayerCollection'''


class BMLayerAccessVert:
    ''' Exposes custom-data layer attributes.
    '''

    bevel_weight: 'BMLayerCollection' = None
    ''' Bevel weight float in [0 - 1].

    :type: 'BMLayerCollection'
    '''

    color = None
    ''' Generic RGBA color with 8-bit precision custom-data layer. type: BMLayerCollection'''

    deform = None
    ''' Vertex deform weight BMDeformVert (TODO). type: BMLayerCollection'''

    float = None
    ''' Generic float custom-data layer. type: BMLayerCollection'''

    float_color = None
    ''' Generic RGBA color with float precision custom-data layer. type: BMLayerCollection'''

    float_vector = None
    ''' Generic 3D vector with float precision custom-data layer. type: BMLayerCollection'''

    int = None
    ''' Generic int custom-data layer. type: BMLayerCollection'''

    paint_mask = None
    ''' Accessor for paint mask layer. type: BMLayerCollection'''

    shape: 'BMLayerCollection' = None
    ''' Vertex shapekey absolute location (as a 3D Vector).

    :type: 'BMLayerCollection'
    '''

    skin = None
    ''' Accessor for skin layer. type: BMLayerCollection'''

    string = None
    ''' Generic string custom-data layer (exposed as bytes, 255 max length). type: BMLayerCollection'''


class BMLayerCollection:
    ''' Gives access to a collection of custom-data layers of the same type and behaves like python dictionaries, except for the ability to do list like index access.
    '''

    active: 'BMLayerItem' = None
    ''' The active layer of this type (read-only).

    :type: 'BMLayerItem'
    '''

    is_singleton: bool = None
    ''' True if there can exists only one layer of this type (read-only).

    :type: bool
    '''

    def get(self, key: str, default=None):
        ''' Returns the value of the layer matching the key or default when not found (matches pythons dictionary function of the same name).

        :param key: The key associated with the layer.
        :type key: str
        :param default: Optional argument for the value to return if *key* is not found.
        :type default: 
        '''
        pass

    def items(self) -> list:
        ''' Return the identifiers of collection members (matching pythons dict.items() functionality).

        :rtype: list
        :return: (key, value) pairs for each member of this collection.
        '''
        pass

    def keys(self) -> list:
        ''' Return the identifiers of collection members (matching pythons dict.keys() functionality).

        :rtype: list
        :return: the identifiers for each member of this collection.
        '''
        pass

    def new(self, name: str) -> 'BMLayerItem':
        ''' Create a new layer

        :param name: Optional name argument (will be made unique).
        :type name: str
        :rtype: 'BMLayerItem'
        :return: The newly created layer.
        '''
        pass

    def remove(self, layer: 'BMLayerItem'):
        ''' Remove a layer

        :param layer: The layer to remove.
        :type layer: 'BMLayerItem'
        '''
        pass

    def values(self) -> list:
        ''' Return the values of collection (matching pythons dict.values() functionality).

        :rtype: list
        :return: the members of this collection.
        '''
        pass

    def verify(self) -> 'BMLayerItem':
        ''' Create a new layer or return an existing active layer

        :rtype: 'BMLayerItem'
        :return: The newly verified layer.
        '''
        pass


class BMLayerItem:
    ''' Exposes a single custom data layer, their main purpose is for use as item accessors to custom-data when used with vert/edge/face/loop data.
    '''

    name: str = None
    ''' The layers unique name (read-only).

    :type: str
    '''

    def copy_from(self, other):
        ''' Return a copy of the layer

        :param other: 
        :type other: 
        :param other: 
        :type other: 
        '''
        pass


class BMLoop:
    ''' This is normally accessed from BMFace.loops where each face loop represents a corner of the face.
    '''

    edge: 'BMEdge' = None
    ''' The loop's edge (between this loop and the next), (read-only).

    :type: 'BMEdge'
    '''

    face: 'BMFace' = None
    ''' The face this loop makes (read-only).

    :type: 'BMFace'
    '''

    index: int = None
    ''' Index of this element.

    :type: int
    '''

    is_convex: bool = None
    ''' True when this loop is at the convex corner of a face, depends on a valid face normal (read-only).

    :type: bool
    '''

    is_valid: bool = None
    ''' True when this element is valid (hasn't been removed).

    :type: bool
    '''

    link_loop_next: 'BMLoop' = None
    ''' The next face corner (read-only).

    :type: 'BMLoop'
    '''

    link_loop_prev: 'BMLoop' = None
    ''' The previous face corner (read-only).

    :type: 'BMLoop'
    '''

    link_loop_radial_next: 'BMLoop' = None
    ''' The next loop around the edge (read-only).

    :type: 'BMLoop'
    '''

    link_loop_radial_prev: 'BMLoop' = None
    ''' The previous loop around the edge (read-only).

    :type: 'BMLoop'
    '''

    link_loops: typing.Union[typing.List['BMLoop'], 'BMElemSeq'] = None
    ''' Loops connected to this loop, (read-only).

    :type: typing.Union[typing.List['BMLoop'], 'BMElemSeq']
    '''

    tag: bool = None
    ''' Generic attribute scripts can use for own logic

    :type: bool
    '''

    vert: 'BMVert' = None
    ''' The loop's vertex (read-only).

    :type: 'BMVert'
    '''

    def calc_angle(self) -> float:
        ''' Return the angle at this loops corner of the face. This is calculated so sharper corners give lower angles.

        :rtype: float
        :return: The angle in radians.
        '''
        pass

    def calc_normal(self) -> 'mathutils.Vector':
        ''' Return normal at this loops corner of the face. Falls back to the face normal for straight lines.

        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def calc_tangent(self) -> 'mathutils.Vector':
        ''' Return the tangent at this loops corner of the face (pointing inward into the face). Falls back to the face normal for straight lines.

        :rtype: 'mathutils.Vector'
        :return: a normalized vector.
        '''
        pass

    def copy_from(self, other):
        ''' Copy values from another element of matching type.

        '''
        pass

    def copy_from_face_interp(self,
                              face: 'BMFace',
                              vert: bool = True,
                              multires: bool = True):
        ''' Interpolate the customdata from a face onto this loop (the loops vert should overlap the face).

        :param face: The face to interpolate data from.
        :type face: 'BMFace'
        :param vert: When enabled, interpolate the loops vertex data (optional).
        :type vert: bool
        :param multires: When enabled, interpolate the loops multires data (optional).
        :type multires: bool
        '''
        pass


class BMLoopSeq:
    layers: 'BMLayerAccessLoop' = None
    ''' custom-data layers (read-only).

    :type: 'BMLayerAccessLoop'
    '''


class BMLoopUV:
    pin_uv: bool = None
    ''' UV pin state.

    :type: bool
    '''

    select: bool = None
    ''' UV select state.

    :type: bool
    '''

    uv: 'mathutils.Vector' = None
    ''' Loops UV (as a 2D Vector).

    :type: 'mathutils.Vector'
    '''


class BMVert:
    ''' The BMesh vertex type
    '''

    co: 'mathutils.Vector' = None
    ''' The coordinates for this vertex as a 3D, wrapped vector.

    :type: 'mathutils.Vector'
    '''

    hide: bool = None
    ''' Hidden state of this element.

    :type: bool
    '''

    index: int = None
    ''' Index of this element.

    :type: int
    '''

    is_boundary: bool = None
    ''' True when this vertex is connected to boundary edges (read-only).

    :type: bool
    '''

    is_manifold: bool = None
    ''' True when this vertex is manifold (read-only).

    :type: bool
    '''

    is_valid: bool = None
    ''' True when this element is valid (hasn't been removed).

    :type: bool
    '''

    is_wire: bool = None
    ''' True when this vertex is not connected to any faces (read-only).

    :type: bool
    '''

    link_edges: typing.Union[typing.List['BMEdge'], 'BMElemSeq'] = None
    ''' Edges connected to this vertex (read-only).

    :type: typing.Union[typing.List['BMEdge'], 'BMElemSeq']
    '''

    link_faces: typing.Union[typing.List['BMFace'], 'BMElemSeq'] = None
    ''' Faces connected to this vertex (read-only).

    :type: typing.Union[typing.List['BMFace'], 'BMElemSeq']
    '''

    link_loops: typing.Union[typing.List['BMLoop'], 'BMElemSeq'] = None
    ''' Loops that use this vertex (read-only).

    :type: typing.Union[typing.List['BMLoop'], 'BMElemSeq']
    '''

    normal: 'mathutils.Vector' = None
    ''' The normal for this vertex as a 3D, wrapped vector.

    :type: 'mathutils.Vector'
    '''

    select: bool = None
    ''' Selected state of this element.

    :type: bool
    '''

    tag: bool = None
    ''' Generic attribute scripts can use for own logic

    :type: bool
    '''

    def calc_edge_angle(self, fallback=None) -> float:
        ''' Return the angle between this vert's two connected edges.

        :param fallback: return this when the vert doesn't have 2 edges (instead of raising a :exc: ValueError ).
        :type fallback: 
        :rtype: float
        :return: Angle between edges in radians.
        '''
        pass

    def calc_shell_factor(self) -> float:
        ''' Return a multiplier calculated based on the sharpness of the vertex. Where a flat surface gives 1.0, and higher values sharper edges. This is used to maintain shell thickness when offsetting verts along their normals.

        :rtype: float
        :return: offset multiplier
        '''
        pass

    def copy_from(self, other):
        ''' Copy values from another element of matching type.

        '''
        pass

    def copy_from_face_interp(self, face: 'BMFace'):
        ''' Interpolate the customdata from a face onto this loop (the loops vert should overlap the face).

        :param face: The face to interpolate data from.
        :type face: 'BMFace'
        '''
        pass

    def copy_from_vert_interp(self, vert_pair: 'BMVert', fac):
        ''' Interpolate the customdata from a vert between 2 other verts.

        :param vert_pair: The vert to interpolate data from.
        :type vert_pair: 'BMVert'
        '''
        pass

    def hide_set(self, hide: bool):
        ''' Set the hide state. This is different from the *hide* attribute because it updates the selection and hide state of associated geometry.

        :param hide: Hidden or visible.
        :type hide: bool
        '''
        pass

    def normal_update(self):
        ''' Update vertex normal.

        '''
        pass

    def select_set(self, select: bool):
        ''' Set the selection. This is different from the *select* attribute because it updates the selection state of associated geometry.

        :param select: Select or de-select.
        :type select: bool
        '''
        pass


class BMVertSeq:
    layers: 'BMLayerAccessVert' = None
    ''' custom-data layers (read-only).

    :type: 'BMLayerAccessVert'
    '''

    def ensure_lookup_table(self):
        ''' Ensure internal data needed for int subscription is initialized with verts/edges/faces, eg bm.verts[index] . This needs to be called again after adding/removing data in this sequence.

        '''
        pass

    def index_update(self):
        ''' Initialize the index values of this sequence. This is the equivalent of looping over all elements and assigning the index values.

        '''
        pass

    def new(self, co: float = (0.0, 0.0, 0.0),
            example: 'BMVert' = None) -> 'BMVert':
        ''' Create a new vertex.

        :param co: The initial location of the vertex (optional argument).
        :type co: float
        :param example: Existing vert to initialize settings.
        :type example: 'BMVert'
        :rtype: 'BMVert'
        :return: The newly created edge.
        '''
        pass

    def remove(self, vert):
        ''' Remove a vert.

        '''
        pass

    def sort(self, key=None, reverse=False):
        ''' Sort the elements of this sequence, using an optional custom sort key. Indices of elements are not changed, BMElemeSeq.index_update() can be used for that.

        :param key: The key that sets the ordering of the elements.
        :type key: 
        :param reverse: Reverse the order of the elements
        :type reverse: 
        '''
        pass


class BMesh:
    ''' The BMesh data structure
    '''

    edges: typing.Union[typing.List['BMEdge'], 'BMEdgeSeq'] = None
    ''' This meshes edge sequence (read-only).

    :type: typing.Union[typing.List['BMEdge'], 'BMEdgeSeq']
    '''

    faces: typing.Union[typing.List['BMFace'], 'BMFaceSeq'] = None
    ''' This meshes face sequence (read-only).

    :type: typing.Union[typing.List['BMFace'], 'BMFaceSeq']
    '''

    is_valid: bool = None
    ''' True when this element is valid (hasn't been removed).

    :type: bool
    '''

    is_wrapped: bool = None
    ''' True when this mesh is owned by blender (typically the editmode BMesh).

    :type: bool
    '''

    loops: typing.Union[typing.List['BMLoop'], 'BMLoopSeq'] = None
    ''' This meshes loops (read-only).

    :type: typing.Union[typing.List['BMLoop'], 'BMLoopSeq']
    '''

    select_history: 'BMEditSelSeq' = None
    ''' Sequence of selected items (the last is displayed as active).

    :type: 'BMEditSelSeq'
    '''

    select_mode: set = None
    ''' The selection mode, values can be {'VERT', 'EDGE', 'FACE'}, can't be assigned an empty set.

    :type: set
    '''

    verts: typing.Union[typing.List['BMVert'], 'BMVertSeq'] = None
    ''' This meshes vert sequence (read-only).

    :type: typing.Union[typing.List['BMVert'], 'BMVertSeq']
    '''

    def calc_loop_triangles(self) -> 'BMLoop':
        ''' Calculate triangle tessellation from quads/ngons.

        :rtype: 'BMLoop'
        :return: The triangulated faces.
        '''
        pass

    def calc_volume(self, signed: bool = False) -> float:
        ''' Calculate mesh volume based on face normals.

        :param signed: when signed is true, negative values may be returned.
        :type signed: bool
        :rtype: float
        :return: The volume of the mesh.
        '''
        pass

    def clear(self):
        ''' Clear all mesh data.

        '''
        pass

    def copy(self) -> 'BMesh':
        ''' 

        :rtype: 'BMesh'
        :return: A copy of this BMesh.
        '''
        pass

    def free(self):
        ''' Explicitly free the BMesh data from memory, causing exceptions on further access.

        '''
        pass

    def from_mesh(self,
                  mesh: 'bpy.types.Mesh',
                  face_normals=True,
                  use_shape_key: bool = False,
                  shape_key_index: int = 0):
        ''' Initialize this bmesh from existing mesh datablock.

        :param mesh: The mesh data to load.
        :type mesh: 'bpy.types.Mesh'
        :param use_shape_key: Use the locations from a shape key.
        :type use_shape_key: bool
        :param shape_key_index: The shape key index to use.
        :type shape_key_index: int
        '''
        pass

    def from_object(self,
                    object: 'bpy.types.Object',
                    depsgraph,
                    deform: bool = True,
                    cage: bool = False,
                    face_normals: bool = True):
        ''' Initialize this bmesh from existing object datablock (currently only meshes are supported).

        :param object: The object data to load.
        :type object: 'bpy.types.Object'
        :param deform: Apply deformation modifiers.
        :type deform: bool
        :param cage: Get the mesh as a deformed cage.
        :type cage: bool
        :param face_normals: Calculate face normals.
        :type face_normals: bool
        '''
        pass

    def normal_update(self):
        ''' Update mesh normals.

        '''
        pass

    def select_flush(self, select: bool):
        ''' Flush selection, independent of the current selection mode.

        :param select: flush selection or de-selected elements.
        :type select: bool
        '''
        pass

    def select_flush_mode(self):
        ''' flush selection based on the current mode current BMesh.select_mode .

        '''
        pass

    def to_mesh(self, mesh: 'bpy.types.Mesh'):
        ''' Writes this BMesh data into an existing Mesh datablock.

        :param mesh: The mesh data to write into.
        :type mesh: 'bpy.types.Mesh'
        '''
        pass

    def transform(self, matrix: 'mathutils.Matrix', filter: set = None):
        ''' Transform the mesh (optionally filtering flagged data only).

        :param matrix: transform matrix.
        :type matrix: 'mathutils.Matrix'
        :param filter: set of values in ('SELECT', 'HIDE', 'SEAM', 'SMOOTH', 'TAG').
        :type filter: set
        '''
        pass

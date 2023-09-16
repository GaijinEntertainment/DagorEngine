import sys
import typing
import bpy.types
import mathutils


class AdjacencyIterator:
    ''' Class hierarchy: Iterator > AdjacencyIterator Class for representing adjacency iterators used in the chaining process. An AdjacencyIterator is created in the increment() and decrement() methods of a ChainingIterator and passed to the traverse() method of the ChainingIterator.
    '''

    is_incoming: bool = None
    ''' True if the current ViewEdge is coming towards the iteration vertex, and False otherwise.

    :type: bool
    '''

    object: 'ViewEdge' = None
    ''' The ViewEdge object currently pointed to by this iterator.

    :type: 'ViewEdge'
    '''

    def __init__(self):
        ''' __init__(brother) __init__(vertex, restrict_to_selection=True, restrict_to_unvisited=True) Builds an AdjacencyIterator using the default constructor, copy constructor or the overloaded constructor.

        :param brother: An AdjacencyIterator object.
        :type brother: 'AdjacencyIterator'
        :param vertex: The vertex which is the next crossing.
        :type vertex: 'ViewVertex'
        :param restrict_to_selection: Indicates whether to force the chaining to stay within the set of selected ViewEdges or not.
        :type restrict_to_selection: bool
        :param restrict_to_unvisited: Indicates whether a ViewEdge that has already been chained must be ignored ot not.
        :type restrict_to_unvisited: bool
        '''
        pass


class BBox:
    ''' Class for representing a bounding box.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class BinaryPredicate0D:
    ''' Base class for binary predicates working on Interface0D objects. A BinaryPredicate0D is typically an ordering relation between two Interface0D objects. The predicate evaluates a relation between the two Interface0D instances and returns a boolean value (true or false). It is used by invoking the __call__() method.
    '''

    name: str = None
    ''' The name of the binary 0D predicate.

    :type: str
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def __call__(self, inter1: 'Interface0D', inter2: 'Interface0D') -> bool:
        ''' Must be overload by inherited classes. It evaluates a relation between two Interface0D objects.

        :param inter1: The first Interface0D object.
        :type inter1: 'Interface0D'
        :param inter2: The second Interface0D object.
        :type inter2: 'Interface0D'
        :rtype: bool
        :return: True or false.
        '''
        pass


class BinaryPredicate1D:
    ''' Base class for binary predicates working on Interface1D objects. A BinaryPredicate1D is typically an ordering relation between two Interface1D objects. The predicate evaluates a relation between the two Interface1D instances and returns a boolean value (true or false). It is used by invoking the __call__() method.
    '''

    name: str = None
    ''' The name of the binary 1D predicate.

    :type: str
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def __call__(self, inter1: 'Interface1D', inter2: 'Interface1D') -> bool:
        ''' Must be overload by inherited classes. It evaluates a relation between two Interface1D objects.

        :param inter1: The first Interface1D object.
        :type inter1: 'Interface1D'
        :param inter2: The second Interface1D object.
        :type inter2: 'Interface1D'
        :rtype: bool
        :return: True or false.
        '''
        pass


class Chain:
    ''' Class hierarchy: Interface1D > Curve > Chain Class to represent a 1D elements issued from the chaining process. A Chain is the last step before the Stroke and is used in the Splitting and Creation processes.
    '''

    def __init__(self):
        ''' __init__(brother) __init__(id) Builds a Chain using the default constructor, copy constructor or from an Id .

        :param brother: A Chain object.
        :type brother: 'Chain'
        :param id: An Id object.
        :type id: 'Id'
        '''
        pass

    def push_viewedge_back(self, viewedge: 'ViewEdge', orientation: bool):
        ''' Adds a ViewEdge at the end of the Chain.

        :param viewedge: The ViewEdge that must be added.
        :type viewedge: 'ViewEdge'
        :param orientation: The orientation with which the ViewEdge must be processed.
        :type orientation: bool
        '''
        pass

    def push_viewedge_front(self, viewedge: 'ViewEdge', orientation: bool):
        ''' Adds a ViewEdge at the beginning of the Chain.

        :param viewedge: The ViewEdge that must be added.
        :type viewedge: 'ViewEdge'
        :param orientation: The orientation with which the ViewEdge must be processed.
        :type orientation: bool
        '''
        pass


class ChainingIterator:
    ''' Class hierarchy: Iterator > ViewEdgeIterator > ChainingIterator Base class for chaining iterators. This class is designed to be overloaded in order to describe chaining rules. It makes the description of chaining rules easier. The two main methods that need to overloaded are traverse() and init(). traverse() tells which ViewEdge to follow, among the adjacent ones. If you specify restriction rules (such as "Chain only ViewEdges of the selection"), they will be included in the adjacency iterator (i.e, the adjacent iterator will only stop on "valid" edges).
    '''

    is_incrementing: bool = None
    ''' True if the current iteration is an incrementation.

    :type: bool
    '''

    next_vertex: 'ViewVertex' = None
    ''' The ViewVertex that is the next crossing.

    :type: 'ViewVertex'
    '''

    object: 'ViewEdge' = None
    ''' The ViewEdge object currently pointed by this iterator.

    :type: 'ViewEdge'
    '''

    def __init__(self,
                 restrict_to_selection: bool = True,
                 restrict_to_unvisited: bool = True,
                 begin: 'ViewEdge' = None,
                 orientation: bool = True):
        ''' __init__(brother) Builds a Chaining Iterator from the first ViewEdge used for iteration and its orientation or by using the copy constructor.

        :param restrict_to_selection: Indicates whether to force the chaining to stay within the set of selected ViewEdges or not.
        :type restrict_to_selection: bool
        :param restrict_to_unvisited: Indicates whether a ViewEdge that has already been chained must be ignored ot not.
        :type restrict_to_unvisited: bool
        :param begin: The ViewEdge from which to start the chain.
        :type begin: 'ViewEdge'
        :param orientation: The direction to follow to explore the graph. If true, the direction indicated by the first ViewEdge is used.
        :type orientation: bool
        :param brother: 
        :type brother: 'ChainingIterator'
        '''
        pass

    def init(self):
        ''' Initializes the iterator context. This method is called each time a new chain is started. It can be used to reset some history information that you might want to keep.

        '''
        pass

    def traverse(self, it: 'AdjacencyIterator') -> 'ViewEdge':
        ''' This method iterates over the potential next ViewEdges and returns the one that will be followed next. Returns the next ViewEdge to follow or None when the end of the chain is reached.

        :param it: The iterator over the ViewEdges adjacent to the end vertex of the current ViewEdge. The adjacency iterator reflects the restriction rules by only iterating over the valid ViewEdges.
        :type it: 'AdjacencyIterator'
        :rtype: 'ViewEdge'
        :return: Returns the next ViewEdge to follow, or None if chaining ends.
        '''
        pass


class Curve:
    ''' Class hierarchy: Interface1D > Curve Base class for curves made of CurvePoints. SVertex is the type of the initial curve vertices. A Chain is a specialization of a Curve.
    '''

    is_empty: bool = None
    ''' True if the Curve doesn't have any Vertex yet.

    :type: bool
    '''

    segments_size: int = None
    ''' The number of segments in the polyline constituting the Curve.

    :type: int
    '''

    def __init__(self):
        ''' __init__(brother) __init__(id) Builds a FrsCurve using a default constructor, copy constructor or from an Id .

        :param brother: A Curve object.
        :type brother: 'bpy.types.Curve'
        :param id: An Id object.
        :type id: 'Id'
        '''
        pass

    def push_vertex_back(self, vertex: typing.Union['CurvePoint', 'SVertex']):
        ''' Adds a single vertex at the end of the Curve.

        :param vertex: A vertex object.
        :type vertex: typing.Union['CurvePoint', 'SVertex']
        '''
        pass

    def push_vertex_front(self, vertex: typing.Union['CurvePoint', 'SVertex']):
        ''' Adds a single vertex at the front of the Curve.

        :param vertex: A vertex object.
        :type vertex: typing.Union['CurvePoint', 'SVertex']
        '''
        pass


class CurvePoint:
    ''' Class hierarchy: Interface0D > CurvePoint Class to represent a point of a curve. A CurvePoint can be any point of a 1D curve (it doesn't have to be a vertex of the curve). Any Interface1D is built upon ViewEdges, themselves built upon FEdges. Therefore, a curve is basically a polyline made of a list of SVertex objects. Thus, a CurvePoint is built by linearly interpolating two SVertex instances. CurvePoint can be used as virtual points while querying 0D information along a curve at a given resolution.
    '''

    fedge: 'FEdge' = None
    ''' Gets the FEdge for the two SVertices that given CurvePoints consists out of. A shortcut for CurvePoint.first_svertex.get_fedge(CurvePoint.second_svertex).

    :type: 'FEdge'
    '''

    first_svertex: 'SVertex' = None
    ''' The first SVertex upon which the CurvePoint is built.

    :type: 'SVertex'
    '''

    second_svertex: 'SVertex' = None
    ''' The second SVertex upon which the CurvePoint is built.

    :type: 'SVertex'
    '''

    t2d: float = None
    ''' The 2D interpolation parameter.

    :type: float
    '''

    def __init__(self):
        ''' __init__(brother) __init__(first_vertex, second_vertex, t2d) __init__(first_point, second_point, t2d) Builds a CurvePoint using the default constructor, copy constructor, or one of the overloaded constructors. The over loaded constructors can either take two SVertex or two CurvePoint objects and an interpolation parameter

        :param brother: A CurvePoint object.
        :type brother: 'CurvePoint'
        :param first_vertex: The first SVertex.
        :type first_vertex: 'SVertex'
        :param second_vertex: The second SVertex.
        :type second_vertex: 'SVertex'
        :param first_point: The first CurvePoint.
        :type first_point: 'CurvePoint'
        :param second_point: The second CurvePoint.
        :type second_point: 'CurvePoint'
        :param t2d: A 2D interpolation parameter used to linearly interpolate first_vertex and second_vertex or first_point and second_point.
        :type t2d: float
        '''
        pass


class CurvePointIterator:
    ''' Class hierarchy: Iterator > CurvePointIterator Class representing an iterator on a curve. Allows an iterating outside initial vertices. A CurvePoint is instantiated and returned through the .object attribute.
    '''

    object: 'CurvePoint' = None
    ''' The CurvePoint object currently pointed by this iterator.

    :type: 'CurvePoint'
    '''

    t: float = None
    ''' The curvilinear abscissa of the current point.

    :type: float
    '''

    u: float = None
    ''' The point parameter at the current point in the stroke (0 <= u <= 1).

    :type: float
    '''

    def __init__(self):
        ''' __init__(brother) __init__(step=0.0) Builds a CurvePointIterator object using either the default constructor, copy constructor, or the overloaded constructor.

        :param brother: A CurvePointIterator object.
        :type brother: 'CurvePointIterator'
        :param step: A resampling resolution with which the curve is resampled. If zero, no resampling is done (i.e., the iterator iterates over initial vertices).
        :type step: float
        '''
        pass


class FEdge:
    ''' Class hierarchy: Interface1D > FEdge Base Class for feature edges. This FEdge can represent a silhouette, a crease, a ridge/valley, a border or a suggestive contour. For silhouettes, the FEdge is oriented so that the visible face lies on the left of the edge. For borders, the FEdge is oriented so that the face lies on the left of the edge. An FEdge can represent an initial edge of the mesh or runs across a face of the initial mesh depending on the smoothness or sharpness of the mesh. This class is specialized into a smooth and a sharp version since their properties slightly vary from one to the other.
    '''

    first_svertex: 'SVertex' = None
    ''' The first SVertex constituting this FEdge.

    :type: 'SVertex'
    '''

    id: 'Id' = None
    ''' The Id of this FEdge.

    :type: 'Id'
    '''

    is_smooth: bool = None
    ''' True if this FEdge is a smooth FEdge.

    :type: bool
    '''

    nature: 'Nature' = None
    ''' The nature of this FEdge.

    :type: 'Nature'
    '''

    next_fedge: 'FEdge' = None
    ''' The FEdge following this one in the ViewEdge. The value is None if this FEdge is the last of the ViewEdge.

    :type: 'FEdge'
    '''

    previous_fedge: 'FEdge' = None
    ''' The FEdge preceding this one in the ViewEdge. The value is None if this FEdge is the first one of the ViewEdge.

    :type: 'FEdge'
    '''

    second_svertex: 'SVertex' = None
    ''' The second SVertex constituting this FEdge.

    :type: 'SVertex'
    '''

    viewedge: 'ViewEdge' = None
    ''' The ViewEdge to which this FEdge belongs to.

    :type: 'ViewEdge'
    '''

    def FEdge(self):
        ''' FEdge(brother) Builds an FEdge using the default constructor, copy constructor, or between two SVertex objects.

        :param brother: An FEdge object.
        :type brother: 'FEdge'
        :param first_vertex: The first SVertex.
        :type first_vertex: 'SVertex'
        :param second_vertex: The second SVertex.
        :type second_vertex: 'SVertex'
        '''
        pass


class FEdgeSharp:
    ''' Class hierarchy: Interface1D > FEdge > FEdgeSharp Class defining a sharp FEdge. A Sharp FEdge corresponds to an initial edge of the input mesh. It can be a silhouette, a crease or a border. If it is a crease edge, then it is bordered by two faces of the mesh. Face a lies on its right whereas Face b lies on its left. If it is a border edge, then it doesn't have any face on its right, and thus Face a is None.
    '''

    face_mark_left: bool = None
    ''' The face mark of the face lying on the left of the FEdge.

    :type: bool
    '''

    face_mark_right: bool = None
    ''' The face mark of the face lying on the right of the FEdge. If this FEdge is a border, it has no face on the right and thus this property is set to false.

    :type: bool
    '''

    material_index_left: int = None
    ''' The index of the material of the face lying on the left of the FEdge.

    :type: int
    '''

    material_index_right: int = None
    ''' The index of the material of the face lying on the right of the FEdge. If this FEdge is a border, it has no Face on its right and therefore no material.

    :type: int
    '''

    material_left: 'bpy.types.Material' = None
    ''' The material of the face lying on the left of the FEdge.

    :type: 'bpy.types.Material'
    '''

    material_right: 'bpy.types.Material' = None
    ''' The material of the face lying on the right of the FEdge. If this FEdge is a border, it has no Face on its right and therefore no material.

    :type: 'bpy.types.Material'
    '''

    normal_left: 'mathutils.Vector' = None
    ''' The normal to the face lying on the left of the FEdge.

    :type: 'mathutils.Vector'
    '''

    normal_right: 'mathutils.Vector' = None
    ''' The normal to the face lying on the right of the FEdge. If this FEdge is a border, it has no Face on its right and therefore no normal.

    :type: 'mathutils.Vector'
    '''

    def __init__(self):
        ''' __init__(brother) __init__(first_vertex, second_vertex) Builds an FEdgeSharp using the default constructor, copy constructor, or between two SVertex objects.

        :param brother: An FEdgeSharp object.
        :type brother: 'FEdgeSharp'
        :param first_vertex: The first SVertex object.
        :type first_vertex: 'SVertex'
        :param second_vertex: The second SVertex object.
        :type second_vertex: 'SVertex'
        '''
        pass


class FEdgeSmooth:
    ''' Class hierarchy: Interface1D > FEdge > FEdgeSmooth Class defining a smooth edge. This kind of edge typically runs across a face of the input mesh. It can be a silhouette, a ridge or valley, a suggestive contour.
    '''

    face_mark: bool = None
    ''' The face mark of the face that this FEdge is running across.

    :type: bool
    '''

    material: 'bpy.types.Material' = None
    ''' The material of the face that this FEdge is running across.

    :type: 'bpy.types.Material'
    '''

    material_index: int = None
    ''' The index of the material of the face that this FEdge is running across.

    :type: int
    '''

    normal: 'mathutils.Vector' = None
    ''' The normal of the face that this FEdge is running across.

    :type: 'mathutils.Vector'
    '''

    def __init__(self):
        ''' __init__(brother) __init__(first_vertex, second_vertex) Builds an FEdgeSmooth using the default constructor, copy constructor, or between two SVertex .

        :param brother: An FEdgeSmooth object.
        :type brother: 'FEdgeSmooth'
        :param first_vertex: The first SVertex object.
        :type first_vertex: 'SVertex'
        :param second_vertex: The second SVertex object.
        :type second_vertex: 'SVertex'
        '''
        pass


class Id:
    ''' Class for representing an object Id.
    '''

    first: int = None
    ''' The first number constituting the Id.

    :type: int
    '''

    second: int = None
    ''' The second number constituting the Id.

    :type: int
    '''

    def __init__(self, brother: 'Id'):
        ''' __init__(first=0, second=0) Build the Id from two numbers or another Id using the copy constructor.

        :param brother: An Id object.
        :type brother: 'Id'
        :param first: 
        :type first: int
        :param second: The second number.
        :type second: int
        '''
        pass


class IntegrationType:
    ''' Class hierarchy: int > IntegrationType Different integration methods that can be invoked to integrate into a single value the set of values obtained from each 0D element of an 1D element: * IntegrationType.MEAN: The value computed for the 1D element is the mean of the values obtained for the 0D elements. * IntegrationType.MIN: The value computed for the 1D element is the minimum of the values obtained for the 0D elements. * IntegrationType.MAX: The value computed for the 1D element is the maximum of the values obtained for the 0D elements. * IntegrationType.FIRST: The value computed for the 1D element is the first of the values obtained for the 0D elements. * IntegrationType.LAST: The value computed for the 1D element is the last of the values obtained for the 0D elements.
    '''

    pass


class Interface0D:
    ''' Base class for any 0D element.
    '''

    id: 'Id' = None
    ''' The Id of this 0D element.

    :type: 'Id'
    '''

    name: str = None
    ''' The string of the name of this 0D element.

    :type: str
    '''

    nature: 'Nature' = None
    ''' The nature of this 0D element.

    :type: 'Nature'
    '''

    point_2d: 'mathutils.Vector' = None
    ''' The 2D point of this 0D element.

    :type: 'mathutils.Vector'
    '''

    point_3d: 'mathutils.Vector' = None
    ''' The 3D point of this 0D element.

    :type: 'mathutils.Vector'
    '''

    projected_x: float = None
    ''' The X coordinate of the projected 3D point of this 0D element.

    :type: float
    '''

    projected_y: float = None
    ''' The Y coordinate of the projected 3D point of this 0D element.

    :type: float
    '''

    projected_z: float = None
    ''' The Z coordinate of the projected 3D point of this 0D element.

    :type: float
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def get_fedge(self, inter: 'Interface0D') -> 'FEdge':
        ''' Returns the FEdge that lies between this 0D element and the 0D element given as the argument.

        :param inter: A 0D element.
        :type inter: 'Interface0D'
        :rtype: 'FEdge'
        :return: The FEdge lying between the two 0D elements.
        '''
        pass


class Interface0DIterator:
    ''' Class hierarchy: Iterator > Interface0DIterator Class defining an iterator over Interface0D elements. An instance of this iterator is always obtained from a 1D element.
    '''

    at_last: bool = None
    ''' True if the iterator points to the last valid element. For its counterpart (pointing to the first valid element), use it.is_begin.

    :type: bool
    '''

    object: 'Interface0D' = None
    ''' The 0D object currently pointed to by this iterator. Note that the object may be an instance of an Interface0D subclass. For example if the iterator has been created from the vertices_begin() method of the Stroke class, the .object property refers to a StrokeVertex object.

    :type: 'Interface0D'
    '''

    t: float = None
    ''' The curvilinear abscissa of the current point.

    :type: float
    '''

    u: float = None
    ''' The point parameter at the current point in the 1D element (0 <= u <= 1).

    :type: float
    '''

    def __init__(self, brother: 'Interface0DIterator'):
        ''' __init__(it) Construct a nested Interface0DIterator using either the copy constructor or the constructor that takes an he argument of a Function0D.

        :param brother: An Interface0DIterator object.
        :type brother: 'Interface0DIterator'
        :param it: An iterator object to be nested.
        :type it: typing.Union['StrokeVertexIterator', 'SVertexIterator', 'CurvePointIterator']
        '''
        pass


class Interface1D:
    ''' Base class for any 1D element.
    '''

    id: 'Id' = None
    ''' The Id of this Interface1D.

    :type: 'Id'
    '''

    length_2d: float = None
    ''' The 2D length of this Interface1D.

    :type: float
    '''

    name: str = None
    ''' The string of the name of the 1D element.

    :type: str
    '''

    nature: 'Nature' = None
    ''' The nature of this Interface1D.

    :type: 'Nature'
    '''

    time_stamp: int = None
    ''' The time stamp of the 1D element, mainly used for selection.

    :type: int
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def points_begin(self, t: float = 0.0) -> 'Interface0DIterator':
        ''' Returns an iterator over the Interface1D points, pointing to the first point. The difference with vertices_begin() is that here we can iterate over points of the 1D element at a any given sampling. Indeed, for each iteration, a virtual point is created.

        :param t: A sampling with which we want to iterate over points of this 1D element.
        :type t: float
        :rtype: 'Interface0DIterator'
        :return: An Interface0DIterator pointing to the first point.
        '''
        pass

    def points_end(self, t: float = 0.0) -> 'Interface0DIterator':
        ''' Returns an iterator over the Interface1D points, pointing after the last point. The difference with vertices_end() is that here we can iterate over points of the 1D element at a given sampling. Indeed, for each iteration, a virtual point is created.

        :param t: A sampling with which we want to iterate over points of this 1D element.
        :type t: float
        :rtype: 'Interface0DIterator'
        :return: An Interface0DIterator pointing after the last point.
        '''
        pass

    def vertices_begin(self) -> 'Interface0DIterator':
        ''' Returns an iterator over the Interface1D vertices, pointing to the first vertex.

        :rtype: 'Interface0DIterator'
        :return: An Interface0DIterator pointing to the first vertex.
        '''
        pass

    def vertices_end(self) -> 'Interface0DIterator':
        ''' Returns an iterator over the Interface1D vertices, pointing after the last vertex.

        :rtype: 'Interface0DIterator'
        :return: An Interface0DIterator pointing after the last vertex.
        '''
        pass


class Iterator:
    ''' Base class to define iterators.
    '''

    is_begin: bool = None
    ''' True if the iterator points to the first element.

    :type: bool
    '''

    is_end: bool = None
    ''' True if the iterator points to the last element.

    :type: bool
    '''

    name: str = None
    ''' The string of the name of this iterator.

    :type: str
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def decrement(self):
        ''' Makes the iterator point the previous element.

        '''
        pass

    def increment(self):
        ''' Makes the iterator point the next element.

        '''
        pass


class Material:
    ''' Class defining a material.
    '''

    ambient: 'mathutils.Color' = None
    ''' RGBA components of the ambient color of the material.

    :type: 'mathutils.Color'
    '''

    diffuse: 'mathutils.Vector' = None
    ''' RGBA components of the diffuse color of the material.

    :type: 'mathutils.Vector'
    '''

    emission: 'mathutils.Color' = None
    ''' RGBA components of the emissive color of the material.

    :type: 'mathutils.Color'
    '''

    line: 'mathutils.Vector' = None
    ''' RGBA components of the line color of the material.

    :type: 'mathutils.Vector'
    '''

    priority: int = None
    ''' Line color priority of the material.

    :type: int
    '''

    shininess: float = None
    ''' Shininess coefficient of the material.

    :type: float
    '''

    specular: 'mathutils.Vector' = None
    ''' RGBA components of the specular color of the material.

    :type: 'mathutils.Vector'
    '''

    def __init__(self):
        ''' __init__(brother) __init__(line, diffuse, ambient, specular, emission, shininess, priority) Creates a FrsMaterial using either default constructor, copy constructor, or an overloaded constructor

        :param brother: A Material object to be used as a copy constructor.
        :type brother: 'bpy.types.Material'
        :param line: The line color.
        :type line: typing.Union[typing.List[float], typing.List['mathutils.Vector']]
        :param diffuse: The diffuse color.
        :type diffuse: typing.Union[typing.List[float], typing.List['mathutils.Vector']]
        :param ambient: The ambient color.
        :type ambient: typing.Union[typing.List[float], typing.List['mathutils.Vector']]
        :param specular: The specular color.
        :type specular: typing.Union[typing.List[float], typing.List['mathutils.Vector']]
        :param emission: The emissive color.
        :type emission: typing.Union[typing.List[float], typing.List['mathutils.Vector']]
        :param shininess: The shininess coefficient.
        :type shininess: float
        :param priority: The line color priority.
        :type priority: int
        '''
        pass


class MediumType:
    ''' Class hierarchy: int > MediumType The different blending modes available to similate the interaction media-medium: * Stroke.DRY_MEDIUM: To simulate a dry medium such as Pencil or Charcoal. * Stroke.HUMID_MEDIUM: To simulate ink painting (color subtraction blending). * Stroke.OPAQUE_MEDIUM: To simulate an opaque medium (oil, spray...).
    '''

    pass


class Nature:
    ''' Class hierarchy: int > Nature Different possible natures of 0D and 1D elements of the ViewMap. Vertex natures: * Nature.POINT: True for any 0D element. * Nature.S_VERTEX: True for SVertex. * Nature.VIEW_VERTEX: True for ViewVertex. * Nature.NON_T_VERTEX: True for NonTVertex. * Nature.T_VERTEX: True for TVertex. * Nature.CUSP: True for CUSP. Edge natures: * Nature.NO_FEATURE: True for non feature edges (always false for 1D elements of the ViewMap). * Nature.SILHOUETTE: True for silhouettes. * Nature.BORDER: True for borders. * Nature.CREASE: True for creases. * Nature.RIDGE: True for ridges. * Nature.VALLEY: True for valleys. * Nature.SUGGESTIVE_CONTOUR: True for suggestive contours. * Nature.MATERIAL_BOUNDARY: True for edges at material boundaries. * Nature.EDGE_MARK: True for edges having user-defined edge marks.
    '''

    pass


class Noise:
    ''' Class to provide Perlin noise functionalities. Undocumented, consider contributing <https://developer.blender.org/T51061> __. Undocumented, consider contributing <https://developer.blender.org/T51061> __.
    '''

    def __init__(self, seed=' -1'):
        ''' Builds a Noise object. Seed is an optional argument. The seed value is used as a seed for random number generation if it is equal to or greater than zero; otherwise, time is used as a seed.

        :param seed: Seed for random number generation.
        :type seed: int
        '''
        pass

    def smoothNoise1(self, v: float) -> float:
        ''' Returns a smooth noise value for a 1D element.

        :param v: One-dimensional sample point.
        :type v: float
        :rtype: float
        :return: A smooth noise value.
        '''
        pass

    def smoothNoise2(self, v: typing.List['mathutils.Vector']) -> float:
        ''' Returns a smooth noise value for a 2D element.

        :param v: Two-dimensional sample point.
        :type v: typing.List['mathutils.Vector']
        :rtype: float
        :return: A smooth noise value.
        '''
        pass

    def smoothNoise3(self, v: typing.List['mathutils.Vector']) -> float:
        ''' Returns a smooth noise value for a 3D element.

        :param v: Three-dimensional sample point.
        :type v: typing.List['mathutils.Vector']
        :rtype: float
        :return: A smooth noise value.
        '''
        pass

    def turbulence1(self, v: float, freq: float, amp: float,
                    oct: int = 4) -> float:
        ''' Returns a noise value for a 1D element.

        :param v: One-dimensional sample point.
        :type v: float
        :param freq: Noise frequency.
        :type freq: float
        :param amp: Amplitude.
        :type amp: float
        :param oct: Number of octaves.
        :type oct: int
        :rtype: float
        :return: A noise value.
        '''
        pass

    def turbulence2(self,
                    v: typing.List['mathutils.Vector'],
                    freq: float,
                    amp: float,
                    oct: int = 4) -> float:
        ''' Returns a noise value for a 2D element.

        :param v: Two-dimensional sample point.
        :type v: typing.List['mathutils.Vector']
        :param freq: Noise frequency.
        :type freq: float
        :param amp: Amplitude.
        :type amp: float
        :param oct: Number of octaves.
        :type oct: int
        :rtype: float
        :return: A noise value.
        '''
        pass

    def turbulence3(self,
                    v: typing.List['mathutils.Vector'],
                    freq: float,
                    amp: float,
                    oct: int = 4) -> float:
        ''' Returns a noise value for a 3D element.

        :param v: Three-dimensional sample point.
        :type v: typing.List['mathutils.Vector']
        :param freq: Noise frequency.
        :type freq: float
        :param amp: Amplitude.
        :type amp: float
        :param oct: Number of octaves.
        :type oct: int
        :rtype: float
        :return: A noise value.
        '''
        pass


class NonTVertex:
    ''' Class hierarchy: Interface0D > ViewVertex > NonTVertex View vertex for corners, cusps, etc. associated to a single SVertex. Can be associated to 2 or more view edges.
    '''

    svertex: 'SVertex' = None
    ''' The SVertex on top of which this NonTVertex is built.

    :type: 'SVertex'
    '''

    def __init__(self):
        ''' __init__(svertex) Builds a NonTVertex using the default constructor or a SVertex .

        :param svertex: An SVertex object.
        :type svertex: 'SVertex'
        '''
        pass


class Operators:
    ''' Class defining the operators used in a style module. There are five types of operators: Selection, chaining, splitting, sorting and creation. All these operators are user controlled through functors, predicates and shaders that are taken as arguments.
    '''

    @staticmethod
    def bidirectional_chain(it: 'ChainingIterator', pred: 'UnaryPredicate1D'):
        ''' bidirectional_chain(it) Builds a set of chains from the current set of ViewEdges. Each ViewEdge of the current list potentially starts a new chain. The chaining operator then iterates over the ViewEdges of the ViewMap using the user specified iterator. This operator iterates both using the increment and decrement operators and is therefore bidirectional. This operator works with a ChainingIterator which contains the chaining rules. It is this last one which can be told to chain only edges that belong to the selection or not to process twice a ViewEdge during the chaining. Each time a ViewEdge is added to a chain, its chaining time stamp is incremented. This allows you to keep track of the number of chains to which a ViewEdge belongs to.

        :param it: The ChainingIterator on the ViewEdges of the ViewMap. It contains the chaining rule.
        :type it: 'ChainingIterator'
        :param pred: The predicate on the ViewEdge that expresses the stopping condition. This parameter is optional, you make not want to pass a stopping criterion when the stopping criterion is already contained in the iterator definition.
        :type pred: 'UnaryPredicate1D'
        '''
        pass

    @staticmethod
    def chain(it: 'ViewEdgeIterator', pred: 'UnaryPredicate1D',
              modifier: 'UnaryFunction1DVoid'):
        ''' chain(it, pred) Builds a set of chains from the current set of ViewEdges. Each ViewEdge of the current list starts a new chain. The chaining operator then iterates over the ViewEdges of the ViewMap using the user specified iterator. This operator only iterates using the increment operator and is therefore unidirectional.

        :param it: The iterator on the ViewEdges of the ViewMap. It contains the chaining rule.
        :type it: 'ViewEdgeIterator'
        :param pred: The predicate on the ViewEdge that expresses the stopping condition.
        :type pred: 'UnaryPredicate1D'
        :param modifier: A function that takes a ViewEdge as argument and that is used to modify the processed ViewEdge state (the timestamp incrementation is a typical illustration of such a modifier). If this argument is not given, the time stamp is automatically managed.
        :type modifier: 'UnaryFunction1DVoid'
        '''
        pass

    @staticmethod
    def create(pred: 'UnaryPredicate1D', shaders: typing.List['StrokeShader']):
        ''' Creates and shades the strokes from the current set of chains. A predicate can be specified to make a selection pass on the chains.

        :param pred: The predicate that a chain must verify in order to be transform as a stroke.
        :type pred: 'UnaryPredicate1D'
        :param shaders: The list of shaders used to shade the strokes.
        :type shaders: typing.List['StrokeShader']
        '''
        pass

    @staticmethod
    def get_chain_from_index(i: int) -> 'Chain':
        ''' Returns the Chain at the index in the current set of Chains.

        :param i: index (0 <= i < Operators.get_chains_size()).
        :type i: int
        :rtype: 'Chain'
        :return: The Chain object.
        '''
        pass

    @staticmethod
    def get_chains_size() -> int:
        ''' Returns the number of Chains.

        :rtype: int
        :return: The number of Chains.
        '''
        pass

    @staticmethod
    def get_stroke_from_index(i: int) -> 'Stroke':
        ''' Returns the Stroke at the index in the current set of Strokes.

        :param i: index (0 <= i < Operators.get_strokes_size()).
        :type i: int
        :rtype: 'Stroke'
        :return: The Stroke object.
        '''
        pass

    @staticmethod
    def get_strokes_size() -> int:
        ''' Returns the number of Strokes.

        :rtype: int
        :return: The number of Strokes.
        '''
        pass

    @staticmethod
    def get_view_edges_size() -> int:
        ''' Returns the number of ViewEdges.

        :rtype: int
        :return: The number of ViewEdges.
        '''
        pass

    @staticmethod
    def get_viewedge_from_index(i: int) -> 'ViewEdge':
        ''' Returns the ViewEdge at the index in the current set of ViewEdges.

        :param i: index (0 <= i < Operators.get_view_edges_size()).
        :type i: int
        :rtype: 'ViewEdge'
        :return: The ViewEdge object.
        '''
        pass

    @staticmethod
    def recursive_split(func: 'UnaryFunction0DDouble',
                        pred_1d: 'UnaryPredicate1D',
                        sampling: float = 0.0):
        ''' recursive_split(func, pred_0d, pred_1d, sampling=0.0) Splits the current set of chains in a recursive way. We process the points of each chain (with a specified sampling) to find the point minimizing a specified function. The chain is split in two at this point and the two new chains are processed in the same way. The recursivity level is controlled through a predicate 1D that expresses a stopping condition on the chain that is about to be processed. The user can also specify a 0D predicate to make a first selection on the points that can potentially be split. A point that doesn't verify the 0D predicate won't be candidate in realizing the min.

        :param func: The Unary Function evaluated at each point of the chain. The splitting point is the point minimizing this function.
        :type func: 'UnaryFunction0DDouble'
        :param pred_0d: The Unary Predicate 0D used to select the candidate points where the split can occur. For example, it is very likely that would rather have your chain splitting around its middle point than around one of its extremities. A 0D predicate working on the curvilinear abscissa allows to add this kind of constraints.
        :type pred_0d: 'UnaryPredicate0D'
        :param pred_1d: The Unary Predicate expressing the recursivity stopping condition. This predicate is evaluated for each curve before it actually gets split. If pred_1d(chain) is true, the curve won't be split anymore.
        :type pred_1d: 'UnaryPredicate1D'
        :param sampling: The resolution used to sample the chain for the predicates evaluation. (The chain is not actually resampled; a virtual point only progresses along the curve using this resolution.)
        :type sampling: float
        '''
        pass

    @staticmethod
    def reset(delete_strokes: bool = True):
        ''' Resets the line stylization process to the initial state. The results of stroke creation are accumulated if **delete_strokes** is set to False.

        :param delete_strokes: Delete the strokes that are currently stored.
        :type delete_strokes: bool
        '''
        pass

    @staticmethod
    def select(pred: 'UnaryPredicate1D'):
        ''' Selects the ViewEdges of the ViewMap verifying a specified condition.

        :param pred: The predicate expressing this condition.
        :type pred: 'UnaryPredicate1D'
        '''
        pass

    @staticmethod
    def sequential_split(starting_pred: 'UnaryPredicate0D',
                         stopping_pred: 'UnaryPredicate0D',
                         sampling: float = 0.0):
        ''' sequential_split(pred, sampling=0.0) Splits each chain of the current set of chains in a sequential way. The points of each chain are processed (with a specified sampling) sequentially. The first point of the initial chain is the first point of one of the resulting chains. The splitting ends when no more chain can start.

        :param starting_pred: The predicate on a point that expresses the starting condition. Each time this condition is verified, a new chain begins
        :type starting_pred: 'UnaryPredicate0D'
        :param stopping_pred: The predicate on a point that expresses the stopping condition. The chain ends as soon as this predicate is verified.
        :type stopping_pred: 'UnaryPredicate0D'
        :param pred: The predicate on a point that expresses the splitting condition. Each time the condition is verified, the chain is split into two chains. The resulting set of chains is a partition of the initial chain
        :type pred: 'UnaryPredicate0D'
        :param sampling: The resolution used to sample the chain for the predicates evaluation. (The chain is not actually resampled; a virtual point only progresses along the curve using this resolution.)
        :type sampling: float
        '''
        pass

    @staticmethod
    def sort(pred: 'BinaryPredicate1D'):
        ''' Sorts the current set of chains (or viewedges) according to the comparison predicate given as argument.

        :param pred: The binary predicate used for the comparison.
        :type pred: 'BinaryPredicate1D'
        '''
        pass


class SShape:
    ''' Class to define a feature shape. It is the gathering of feature elements from an identified input shape.
    '''

    bbox: 'BBox' = None
    ''' The bounding box of the SShape.

    :type: 'BBox'
    '''

    edges: typing.List['FEdge'] = None
    ''' The list of edges constituting this SShape.

    :type: typing.List['FEdge']
    '''

    id: 'Id' = None
    ''' The Id of this SShape.

    :type: 'Id'
    '''

    name: str = None
    ''' The name of the SShape.

    :type: str
    '''

    vertices: typing.List['SVertex'] = None
    ''' The list of vertices constituting this SShape.

    :type: typing.List['SVertex']
    '''

    def __init__(self):
        ''' __init__(brother) Creates a SShape class using either a default constructor or copy constructor.

        :param brother: An SShape object.
        :type brother: 'SShape'
        '''
        pass

    def add_edge(self, edge: 'FEdge'):
        ''' Adds an FEdge to the list of FEdges.

        :param edge: An FEdge object.
        :type edge: 'FEdge'
        '''
        pass

    def add_vertex(self, vertex: 'SVertex'):
        ''' Adds an SVertex to the list of SVertex of this Shape. The SShape attribute of the SVertex is also set to this SShape.

        :param vertex: An SVertex object.
        :type vertex: 'SVertex'
        '''
        pass

    def compute_bbox(self):
        ''' Compute the bbox of the SShape.

        '''
        pass


class SVertex:
    ''' Class hierarchy: Interface0D > SVertex Class to define a vertex of the embedding.
    '''

    curvatures: tuple = None
    ''' Curvature information expressed in the form of a seven-element tuple (K1, e1, K2, e2, Kr, er, dKr), where K1 and K2 are scalar values representing the first (maximum) and second (minimum) principal curvatures at this SVertex, respectively; e1 and e2 are three-dimensional vectors representing the first and second principal directions, i.e. the directions of the normal plane where the curvature takes its maximum and minimum values, respectively; and Kr, er and dKr are the radial curvature, radial direction, and the derivative of the radial curvature at this SVertex, respectively.

    :type: tuple
    '''

    id: 'Id' = None
    ''' The Id of this SVertex.

    :type: 'Id'
    '''

    normals: typing.List['mathutils.Vector'] = None
    ''' The normals for this Vertex as a list. In a sharp surface, an SVertex has exactly one normal. In a smooth surface, an SVertex can have any number of normals.

    :type: typing.List['mathutils.Vector']
    '''

    normals_size: int = None
    ''' The number of different normals for this SVertex.

    :type: int
    '''

    point_2d: 'mathutils.Vector' = None
    ''' The projected 3D coordinates of the SVertex.

    :type: 'mathutils.Vector'
    '''

    point_3d: 'mathutils.Vector' = None
    ''' The 3D coordinates of the SVertex.

    :type: 'mathutils.Vector'
    '''

    viewvertex: 'ViewVertex' = None
    ''' If this SVertex is also a ViewVertex, this property refers to the ViewVertex, and None otherwise.

    :type: 'ViewVertex'
    '''

    def __init__(self):
        ''' __init__(brother) __init__(point_3d, id) Builds a SVertex using the default constructor, copy constructor or the overloaded constructor which builds a SVertex from 3D coordinates and an Id.

        :param brother: A SVertex object.
        :type brother: 'SVertex'
        :param point_3d: A three-dimensional vector.
        :type point_3d: 'mathutils.Vector'
        :param id: An Id object.
        :type id: 'Id'
        '''
        pass

    def add_fedge(self, fedge: 'FEdge'):
        ''' Add an FEdge to the list of edges emanating from this SVertex.

        :param fedge: An FEdge.
        :type fedge: 'FEdge'
        '''
        pass

    def add_normal(self, normal: typing.List['mathutils.Vector']):
        ''' Adds a normal to the SVertex's set of normals. If the same normal is already in the set, nothing changes.

        :param normal: A three-dimensional vector.
        :type normal: typing.List['mathutils.Vector']
        '''
        pass


class SVertexIterator:
    ''' Class hierarchy: Iterator > SVertexIterator Class representing an iterator over SVertex of a ViewEdge . An instance of an SVertexIterator can be obtained from a ViewEdge by calling verticesBegin() or verticesEnd().
    '''

    object: 'SVertex' = None
    ''' The SVertex object currently pointed by this iterator.

    :type: 'SVertex'
    '''

    t: float = None
    ''' The curvilinear abscissa of the current point.

    :type: float
    '''

    u: float = None
    ''' The point parameter at the current point in the 1D element (0 <= u <= 1).

    :type: float
    '''

    def __init__(self):
        ''' __init__(brother) __init__(vertex, begin, previous_edge, next_edge, t) Build an SVertexIterator using either the default constructor, copy constructor, or the overloaded constructor that starts iteration from an SVertex object vertex.

        :param brother: An SVertexIterator object.
        :type brother: 'SVertexIterator'
        :param vertex: The SVertex from which the iterator starts iteration.
        :type vertex: 'SVertex'
        :param begin: The first SVertex of a ViewEdge.
        :type begin: 'SVertex'
        :param previous_edge: The previous FEdge coming to vertex.
        :type previous_edge: 'FEdge'
        :param next_edge: The next FEdge going out from vertex.
        :type next_edge: 'FEdge'
        :param t: The curvilinear abscissa at vertex.
        :type t: float
        '''
        pass


class Stroke:
    ''' Class hierarchy: Interface1D > Stroke Class to define a stroke. A stroke is made of a set of 2D vertices ( StrokeVertex ), regularly spaced out. This set of vertices defines the stroke's backbone geometry. Each of these stroke vertices defines the stroke's shape and appearance at this vertex position.
    '''

    id: 'Id' = None
    ''' The Id of this Stroke.

    :type: 'Id'
    '''

    length_2d: float = None
    ''' The 2D length of the Stroke.

    :type: float
    '''

    medium_type: 'MediumType' = None
    ''' The MediumType used for this Stroke.

    :type: 'MediumType'
    '''

    texture_id: int = None
    ''' The ID of the texture used to simulate th marks system for this Stroke.

    :type: int
    '''

    tips: bool = None
    ''' True if this Stroke uses a texture with tips, and false otherwise.

    :type: bool
    '''

    def Stroke(self):
        ''' Stroke(brother) Creates a Stroke using the default constructor or copy constructor

        '''
        pass

    def compute_sampling(self, n: int) -> float:
        ''' Compute the sampling needed to get N vertices. If the specified number of vertices is less than the actual number of vertices, the actual sampling value is returned. (To remove Vertices, use the RemoveVertex() method of this class.)

        :param n: The number of stroke vertices we eventually want in our Stroke.
        :type n: int
        :rtype: float
        :return: The sampling that must be used in the Resample(float) method.
        '''
        pass

    def insert_vertex(self, vertex: 'StrokeVertex',
                      next: 'StrokeVertexIterator'):
        ''' Inserts the StrokeVertex given as argument into the Stroke before the point specified by next. The length and curvilinear abscissa are updated consequently.

        :param vertex: The StrokeVertex to insert in the Stroke.
        :type vertex: 'StrokeVertex'
        :param next: A StrokeVertexIterator pointing to the StrokeVertex before which vertex must be inserted.
        :type next: 'StrokeVertexIterator'
        '''
        pass

    def remove_all_vertices(self):
        ''' Removes all vertices from the Stroke.

        '''
        pass

    def remove_vertex(self, vertex: 'StrokeVertex'):
        ''' Removes the StrokeVertex given as argument from the Stroke. The length and curvilinear abscissa are updated consequently.

        :param vertex: the StrokeVertex to remove from the Stroke.
        :type vertex: 'StrokeVertex'
        '''
        pass

    def resample(self, n: int):
        ''' resample(sampling) Resamples the stroke so using one of two methods with the goal of creating a stroke with fewer points and the same shape.

        :param n: Resamples the stroke so that it eventually has N points. That means it is going to add N-vertices_size, where vertices_size is the number of points we already have. If vertices_size >= N, no resampling is done.
        :type n: int
        :param sampling: Resamples the stroke with a given sampling value. If the sampling is smaller than the actual sampling value, no resampling is done.
        :type sampling: float
        '''
        pass

    def stroke_vertices_begin(self, t: float = 0.0) -> 'StrokeVertexIterator':
        ''' Returns a StrokeVertexIterator pointing on the first StrokeVertex of the Stroke. One can specify a sampling value to re-sample the Stroke on the fly if needed.

        :param t: The resampling value with which we want our Stroke to be resampled. If 0 is specified, no resampling is done.
        :type t: float
        :rtype: 'StrokeVertexIterator'
        :return: A StrokeVertexIterator pointing on the first StrokeVertex.
        '''
        pass

    def stroke_vertices_end(self) -> 'StrokeVertexIterator':
        ''' Returns a StrokeVertexIterator pointing after the last StrokeVertex of the Stroke.

        :rtype: 'StrokeVertexIterator'
        :return: A StrokeVertexIterator pointing after the last StrokeVertex.
        '''
        pass

    def stroke_vertices_size(self) -> int:
        ''' Returns the number of StrokeVertex constituting the Stroke.

        :rtype: int
        :return: The number of stroke vertices.
        '''
        pass

    def update_length(self):
        ''' Updates the 2D length of the Stroke.

        '''
        pass


class StrokeAttribute:
    ''' Class to define a set of attributes associated with a StrokeVertex . The attribute set stores the color, alpha and thickness values for a Stroke Vertex.
    '''

    alpha: float = None
    ''' Alpha component of the stroke color.

    :type: float
    '''

    color: 'mathutils.Color' = None
    ''' RGB components of the stroke color.

    :type: 'mathutils.Color'
    '''

    thickness: 'mathutils.Vector' = None
    ''' Right and left components of the stroke thickness. The right (left) component is the thickness on the right (left) of the vertex when following the stroke.

    :type: 'mathutils.Vector'
    '''

    visible: bool = None
    ''' The visibility flag. True if the StrokeVertex is visible.

    :type: bool
    '''

    def __init__(self):
        ''' __init__(brother) __init__(red, green, blue, alpha, thickness_right, thickness_left) __init__(attribute1, attribute2, t) Creates a StrokeAttribute object using either a default constructor, copy constructor, overloaded constructor, or and interpolation constructor to interpolate between two StrokeAttribute objects.

        :param brother: A StrokeAttribute object to be used as a copy constructor.
        :type brother: 'StrokeAttribute'
        :param red: Red component of a stroke color.
        :type red: float
        :param green: Green component of a stroke color.
        :type green: float
        :param blue: Blue component of a stroke color.
        :type blue: float
        :param alpha: Alpha component of a stroke color.
        :type alpha: float
        :param thickness_right: Stroke thickness on the right.
        :type thickness_right: float
        :param thickness_left: Stroke thickness on the left.
        :type thickness_left: float
        :param attribute1: The first StrokeAttribute object.
        :type attribute1: 'StrokeAttribute'
        :param attribute2: The second StrokeAttribute object.
        :type attribute2: 'StrokeAttribute'
        :param t: The interpolation parameter (0 <= t <= 1).
        :type t: float
        '''
        pass

    def get_attribute_real(self, name: str) -> float:
        ''' Returns an attribute of float type.

        :param name: The name of the attribute.
        :type name: str
        :rtype: float
        :return: The attribute value.
        '''
        pass

    def get_attribute_vec2(self, name: str) -> 'mathutils.Vector':
        ''' Returns an attribute of two-dimensional vector type.

        :param name: The name of the attribute.
        :type name: str
        :rtype: 'mathutils.Vector'
        :return: The attribute value.
        '''
        pass

    def get_attribute_vec3(self, name: str) -> 'mathutils.Vector':
        ''' Returns an attribute of three-dimensional vector type.

        :param name: The name of the attribute.
        :type name: str
        :rtype: 'mathutils.Vector'
        :return: The attribute value.
        '''
        pass

    def has_attribute_real(self, name: str) -> bool:
        ''' Checks whether the attribute name of float type is available.

        :param name: The name of the attribute.
        :type name: str
        :rtype: bool
        :return: True if the attribute is available.
        '''
        pass

    def has_attribute_vec2(self, name: str) -> bool:
        ''' Checks whether the attribute name of two-dimensional vector type is available.

        :param name: The name of the attribute.
        :type name: str
        :rtype: bool
        :return: True if the attribute is available.
        '''
        pass

    def has_attribute_vec3(self, name: str) -> bool:
        ''' Checks whether the attribute name of three-dimensional vector type is available.

        :param name: The name of the attribute.
        :type name: str
        :rtype: bool
        :return: True if the attribute is available.
        '''
        pass

    def set_attribute_real(self, name: str, value: float):
        ''' Adds a user-defined attribute of float type. If there is no attribute of the given name, it is added. Otherwise, the new value replaces the old one.

        :param name: The name of the attribute.
        :type name: str
        :param value: The attribute value.
        :type value: float
        '''
        pass

    def set_attribute_vec2(self, name: str,
                           value: typing.List['mathutils.Vector']):
        ''' Adds a user-defined attribute of two-dimensional vector type. If there is no attribute of the given name, it is added. Otherwise, the new value replaces the old one.

        :param name: The name of the attribute.
        :type name: str
        :param value: The attribute value.
        :type value: typing.List['mathutils.Vector']
        '''
        pass

    def set_attribute_vec3(self, name: str,
                           value: typing.List['mathutils.Vector']):
        ''' Adds a user-defined attribute of three-dimensional vector type. If there is no attribute of the given name, it is added. Otherwise, the new value replaces the old one.

        :param name: The name of the attribute.
        :type name: str
        :param value: The attribute value.
        :type value: typing.List['mathutils.Vector']
        '''
        pass


class StrokeShader:
    ''' Base class for stroke shaders. Any stroke shader must inherit from this class and overload the shade() method. A StrokeShader is designed to modify stroke attributes such as thickness, color, geometry, texture, blending mode, and so on. The basic way for this operation is to iterate over the stroke vertices of the Stroke and to modify the StrokeAttribute of each vertex. Here is a code example of such an iteration:: it = ioStroke.strokeVerticesBegin() while not it.is_end: att = it.object.attribute ## perform here any attribute modification it.increment()
    '''

    name: str = None
    ''' The name of the stroke shader.

    :type: str
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def shade(self, stroke: 'Stroke'):
        ''' The shading method. Must be overloaded by inherited classes.

        :param stroke: A Stroke object.
        :type stroke: 'Stroke'
        '''
        pass


class StrokeVertex:
    ''' Class hierarchy: Interface0D > CurvePoint > StrokeVertex Class to define a stroke vertex.
    '''

    attribute: 'StrokeAttribute' = None
    ''' StrokeAttribute for this StrokeVertex.

    :type: 'StrokeAttribute'
    '''

    curvilinear_abscissa: float = None
    ''' Curvilinear abscissa of this StrokeVertex in the Stroke.

    :type: float
    '''

    point: 'mathutils.Vector' = None
    ''' 2D point coordinates.

    :type: 'mathutils.Vector'
    '''

    stroke_length: float = None
    ''' Stroke length (it is only a value retained by the StrokeVertex, and it won't change the real stroke length).

    :type: float
    '''

    u: float = None
    ''' Curvilinear abscissa of this StrokeVertex in the Stroke.

    :type: float
    '''

    def __init__(self):
        ''' __init__(brother) __init__(first_vertex, second_vertex, t3d) __init__(point) __init__(svertex) __init__(svertex, attribute) Builds a StrokeVertex using the default constructor, copy constructor, from 2 StrokeVertex and an interpolation parameter, from a CurvePoint, from a SVertex, or a SVertex and a StrokeAttribute object.

        :param brother: A StrokeVertex object.
        :type brother: 'StrokeVertex'
        :param first_vertex: The first StrokeVertex.
        :type first_vertex: 'StrokeVertex'
        :param second_vertex: The second StrokeVertex.
        :type second_vertex: 'StrokeVertex'
        :param t3d: An interpolation parameter.
        :type t3d: float
        :param point: A CurvePoint object.
        :type point: 'CurvePoint'
        :param svertex: An SVertex object. An SVertex object.
        :type svertex: 'SVertex'
        :param svertex: An SVertex object. An SVertex object.
        :type svertex: 'SVertex'
        :param attribute: A StrokeAttribute object.
        :type attribute: 'StrokeAttribute'
        '''
        pass


class StrokeVertexIterator:
    ''' Class hierarchy: Iterator > StrokeVertexIterator Class defining an iterator designed to iterate over the StrokeVertex of a Stroke . An instance of a StrokeVertexIterator can be obtained from a Stroke by calling iter(), stroke_vertices_begin() or stroke_vertices_begin(). It is iterating over the same vertices as an Interface0DIterator . The difference resides in the object access: an Interface0DIterator only allows access to an Interface0D while one might need to access the specialized StrokeVertex type. In this case, one should use a StrokeVertexIterator. To call functions of the UnaryFuntion0D type, a StrokeVertexIterator can be converted to an Interface0DIterator by by calling Interface0DIterator(it).
    '''

    at_last: bool = None
    ''' True if the iterator points to the last valid element. For its counterpart (pointing to the first valid element), use it.is_begin.

    :type: bool
    '''

    object: 'StrokeVertex' = None
    ''' The StrokeVertex object currently pointed to by this iterator.

    :type: 'StrokeVertex'
    '''

    t: float = None
    ''' The curvilinear abscissa of the current point.

    :type: float
    '''

    u: float = None
    ''' The point parameter at the current point in the stroke (0 <= u <= 1).

    :type: float
    '''

    def __init__(self):
        ''' __init__(brother) Creates a StrokeVertexIterator using either the default constructor or the copy constructor.

        :param brother: A StrokeVertexIterator object.
        :type brother: 'StrokeVertexIterator'
        '''
        pass

    def decremented(self) -> 'StrokeVertexIterator':
        ''' Returns a copy of a decremented StrokeVertexIterator.

        :rtype: 'StrokeVertexIterator'
        :return: A StrokeVertexIterator pointing the previous StrokeVertex.
        '''
        pass

    def incremented(self) -> 'StrokeVertexIterator':
        ''' Returns a copy of an incremented StrokeVertexIterator.

        :rtype: 'StrokeVertexIterator'
        :return: A StrokeVertexIterator pointing the next StrokeVertex.
        '''
        pass

    def reversed(self) -> 'StrokeVertexIterator':
        ''' Returns a StrokeVertexIterator that traverses stroke vertices in the reversed order.

        :rtype: 'StrokeVertexIterator'
        :return: A StrokeVertexIterator traversing stroke vertices backward.
        '''
        pass


class TVertex:
    ''' Class hierarchy: Interface0D > ViewVertex > TVertex Class to define a T vertex, i.e. an intersection between two edges. It points towards two SVertex and four ViewEdges. Among the ViewEdges, two are front and the other two are back. Basically a front edge hides part of a back edge. So, among the back edges, one is of invisibility N and the other of invisibility N+1.
    '''

    back_svertex: 'SVertex' = None
    ''' The SVertex that is further away from the viewpoint.

    :type: 'SVertex'
    '''

    front_svertex: 'SVertex' = None
    ''' The SVertex that is closer to the viewpoint.

    :type: 'SVertex'
    '''

    id: 'Id' = None
    ''' The Id of this TVertex.

    :type: 'Id'
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def get_mate(self, viewedge: 'ViewEdge') -> 'ViewEdge':
        ''' Returns the mate edge of the ViewEdge given as argument. If the ViewEdge is frontEdgeA, frontEdgeB is returned. If the ViewEdge is frontEdgeB, frontEdgeA is returned. Same for back edges.

        :param viewedge: A ViewEdge object.
        :type viewedge: 'ViewEdge'
        :rtype: 'ViewEdge'
        :return: The mate edge of the given ViewEdge.
        '''
        pass

    def get_svertex(self, fedge: 'FEdge') -> 'SVertex':
        ''' Returns the SVertex (among the 2) belonging to the given FEdge.

        :param fedge: An FEdge object.
        :type fedge: 'FEdge'
        :rtype: 'SVertex'
        :return: The SVertex belonging to the given FEdge.
        '''
        pass


class UnaryFunction0D:
    ''' Base class for Unary Functions (functors) working on Interface0DIterator . A unary function will be used by invoking __call__() on an Interface0DIterator. In Python, several different subclasses of UnaryFunction0D are used depending on the types of functors' return values. For example, you would inherit from a UnaryFunction0DDouble if you wish to define a function that returns a double value. Available UnaryFunction0D subclasses are: * UnaryFunction0DDouble * UnaryFunction0DEdgeNature * UnaryFunction0DFloat * UnaryFunction0DId * UnaryFunction0DMaterial * UnaryFunction0DUnsigned * UnaryFunction0DVec2f * UnaryFunction0DVec3f * UnaryFunction0DVectorViewShape * UnaryFunction0DViewShape
    '''

    name: str = None
    ''' The name of the unary 0D function.

    :type: str
    '''


class UnaryFunction0DDouble:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DDouble Base class for unary functions (functors) that work on Interface0DIterator and return a float value.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DEdgeNature:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DEdgeNature Base class for unary functions (functors) that work on Interface0DIterator and return a Nature object.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DFloat:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DFloat Base class for unary functions (functors) that work on Interface0DIterator and return a float value.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DId:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DId Base class for unary functions (functors) that work on Interface0DIterator and return an Id object.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DMaterial:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DMaterial Base class for unary functions (functors) that work on Interface0DIterator and return a Material object.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DUnsigned:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DUnsigned Base class for unary functions (functors) that work on Interface0DIterator and return an int value.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DVec2f:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DVec2f Base class for unary functions (functors) that work on Interface0DIterator and return a 2D vector.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DVec3f:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DVec3f Base class for unary functions (functors) that work on Interface0DIterator and return a 3D vector.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DVectorViewShape:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DVectorViewShape Base class for unary functions (functors) that work on Interface0DIterator and return a list of ViewShape objects.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction0DViewShape:
    ''' Class hierarchy: UnaryFunction0D > UnaryFunction0DViewShape Base class for unary functions (functors) that work on Interface0DIterator and return a ViewShape object.
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass


class UnaryFunction1D:
    ''' Base class for Unary Functions (functors) working on Interface1D . A unary function will be used by invoking __call__() on an Interface1D. In Python, several different subclasses of UnaryFunction1D are used depending on the types of functors' return values. For example, you would inherit from a UnaryFunction1DDouble if you wish to define a function that returns a double value. Available UnaryFunction1D subclasses are: * UnaryFunction1DDouble * UnaryFunction1DEdgeNature * UnaryFunction1DFloat * UnaryFunction1DUnsigned * UnaryFunction1DVec2f * UnaryFunction1DVec3f * UnaryFunction1DVectorViewShape * UnaryFunction1DVoid
    '''

    name: str = None
    ''' The name of the unary 1D function.

    :type: str
    '''


class UnaryFunction1DDouble:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DDouble Base class for unary functions (functors) that work on Interface1D and return a float value.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DEdgeNature:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DEdgeNature Base class for unary functions (functors) that work on Interface1D and return a Nature object.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DFloat:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DFloat Base class for unary functions (functors) that work on Interface1D and return a float value.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DUnsigned:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DUnsigned Base class for unary functions (functors) that work on Interface1D and return an int value.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DVec2f:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DVec2f Base class for unary functions (functors) that work on Interface1D and return a 2D vector.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DVec3f:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DVec3f Base class for unary functions (functors) that work on Interface1D and return a 3D vector.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DVectorViewShape:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DVectorViewShape Base class for unary functions (functors) that work on Interface1D and return a list of ViewShape objects.
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using the default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryFunction1DVoid:
    ''' Class hierarchy: UnaryFunction1D > UnaryFunction1DVoid Base class for unary functions (functors) working on Interface1D .
    '''

    integration_type: 'IntegrationType' = None
    ''' The integration method.

    :type: 'IntegrationType'
    '''

    def __init__(self):
        ''' __init__(integration_type) Builds a unary 1D function using either a default constructor or the integration method given as an argument.

        :param integration_type: An integration method.
        :type integration_type: 'IntegrationType'
        '''
        pass


class UnaryPredicate0D:
    ''' Base class for unary predicates that work on Interface0DIterator . A UnaryPredicate0D is a functor that evaluates a condition on an Interface0DIterator and returns true or false depending on whether this condition is satisfied or not. The UnaryPredicate0D is used by invoking its __call__() method. Any inherited class must overload the __call__() method.
    '''

    name: str = None
    ''' The name of the unary 0D predicate.

    :type: str
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def __call__(self, it: 'Interface0DIterator') -> bool:
        ''' Must be overload by inherited classes.

        :param it: The Interface0DIterator pointing onto the Interface0D at which we wish to evaluate the predicate.
        :type it: 'Interface0DIterator'
        :rtype: bool
        :return: True if the condition is satisfied, false otherwise.
        '''
        pass


class UnaryPredicate1D:
    ''' Base class for unary predicates that work on Interface1D . A UnaryPredicate1D is a functor that evaluates a condition on a Interface1D and returns true or false depending on whether this condition is satisfied or not. The UnaryPredicate1D is used by invoking its __call__() method. Any inherited class must overload the __call__() method.
    '''

    name: str = None
    ''' The name of the unary 1D predicate.

    :type: str
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def __call__(self, inter: 'Interface1D') -> bool:
        ''' Must be overload by inherited classes.

        :param inter: The Interface1D on which we wish to evaluate the predicate.
        :type inter: 'Interface1D'
        :rtype: bool
        :return: True if the condition is satisfied, false otherwise.
        '''
        pass


class ViewEdge:
    ''' Class hierarchy: Interface1D > ViewEdge Class defining a ViewEdge. A ViewEdge in an edge of the image graph. it connects two ViewVertex objects. It is made by connecting a set of FEdges.
    '''

    chaining_time_stamp: int = None
    ''' The time stamp of this ViewEdge.

    :type: int
    '''

    first_fedge: 'FEdge' = None
    ''' The first FEdge that constitutes this ViewEdge.

    :type: 'FEdge'
    '''

    first_viewvertex: 'ViewVertex' = None
    ''' The first ViewVertex.

    :type: 'ViewVertex'
    '''

    id: 'Id' = None
    ''' The Id of this ViewEdge.

    :type: 'Id'
    '''

    is_closed: bool = None
    ''' True if this ViewEdge forms a closed loop.

    :type: bool
    '''

    last_fedge: 'FEdge' = None
    ''' The last FEdge that constitutes this ViewEdge.

    :type: 'FEdge'
    '''

    last_viewvertex: 'ViewVertex' = None
    ''' The second ViewVertex.

    :type: 'ViewVertex'
    '''

    nature: 'Nature' = None
    ''' The nature of this ViewEdge.

    :type: 'Nature'
    '''

    occludee: 'ViewShape' = None
    ''' The shape that is occluded by the ViewShape to which this ViewEdge belongs to. If no object is occluded, this property is set to None.

    :type: 'ViewShape'
    '''

    qi: int = None
    ''' The quantitative invisibility.

    :type: int
    '''

    viewshape: 'ViewShape' = None
    ''' The ViewShape to which this ViewEdge belongs to.

    :type: 'ViewShape'
    '''

    def __init__(self):
        ''' __init__(brother) Builds a ViewEdge using the default constructor or the copy constructor.

        :param brother: A ViewEdge object.
        :type brother: 'ViewEdge'
        '''
        pass

    def update_fedges(self):
        ''' Sets Viewedge to this for all embedded fedges.

        '''
        pass


class ViewEdgeIterator:
    ''' Class hierarchy: Iterator > ViewEdgeIterator Base class for iterators over ViewEdges of the ViewMap Graph. Basically the increment() operator of this class should be able to take the decision of "where" (on which ViewEdge) to go when pointing on a given ViewEdge.
    '''

    begin: 'ViewEdge' = None
    ''' The first ViewEdge used for the iteration.

    :type: 'ViewEdge'
    '''

    current_edge: 'ViewEdge' = None
    ''' The ViewEdge object currently pointed by this iterator.

    :type: 'ViewEdge'
    '''

    object: 'ViewEdge' = None
    ''' The ViewEdge object currently pointed by this iterator.

    :type: 'ViewEdge'
    '''

    orientation: bool = None
    ''' The orientation of the pointed ViewEdge in the iteration. If true, the iterator looks for the next ViewEdge among those ViewEdges that surround the ending ViewVertex of the "begin" ViewEdge. If false, the iterator searches over the ViewEdges surrounding the ending ViewVertex of the "begin" ViewEdge.

    :type: bool
    '''

    def __init__(self, begin: 'ViewEdge' = None, orientation: bool = True):
        ''' __init__(brother) Builds a ViewEdgeIterator from a starting ViewEdge and its orientation or the copy constructor.

        :param begin: The ViewEdge from where to start the iteration.
        :type begin: 'ViewEdge'
        :param orientation: If true, we'll look for the next ViewEdge among the ViewEdges that surround the ending ViewVertex of begin. If false, we'll search over the ViewEdges surrounding the ending ViewVertex of begin.
        :type orientation: bool
        :param brother: A ViewEdgeIterator object.
        :type brother: 'ViewEdgeIterator'
        '''
        pass

    def change_orientation(self):
        ''' Changes the current orientation.

        '''
        pass


class ViewMap:
    ''' Class defining the ViewMap.
    '''

    scene_bbox: 'BBox' = None
    ''' The 3D bounding box of the scene.

    :type: 'BBox'
    '''

    def __init__(self):
        ''' Default constructor.

        '''
        pass

    def get_closest_fedge(self, x: float, y: float) -> 'FEdge':
        ''' Gets the FEdge nearest to the 2D point specified as arguments.

        :param x: X coordinate of a 2D point.
        :type x: float
        :param y: Y coordinate of a 2D point.
        :type y: float
        :rtype: 'FEdge'
        :return: The FEdge nearest to the specified 2D point.
        '''
        pass

    def get_closest_viewedge(self, x: float, y: float) -> 'ViewEdge':
        ''' Gets the ViewEdge nearest to the 2D point specified as arguments.

        :param x: X coordinate of a 2D point.
        :type x: float
        :param y: Y coordinate of a 2D point.
        :type y: float
        :rtype: 'ViewEdge'
        :return: The ViewEdge nearest to the specified 2D point.
        '''
        pass


class ViewShape:
    ''' Class gathering the elements of the ViewMap (i.e., ViewVertex and ViewEdge ) that are issued from the same input shape.
    '''

    edges: typing.List['ViewEdge'] = None
    ''' The list of ViewEdge objects contained in this ViewShape.

    :type: typing.List['ViewEdge']
    '''

    id: 'Id' = None
    ''' The Id of this ViewShape.

    :type: 'Id'
    '''

    library_path: typing.Union[str, 'ViewShape'] = None
    ''' The library path of the ViewShape.

    :type: typing.Union[str, 'ViewShape']
    '''

    name: str = None
    ''' The name of the ViewShape.

    :type: str
    '''

    sshape: 'SShape' = None
    ''' The SShape on top of which this ViewShape is built.

    :type: 'SShape'
    '''

    vertices: typing.List['ViewVertex'] = None
    ''' The list of ViewVertex objects contained in this ViewShape.

    :type: typing.List['ViewVertex']
    '''

    def __init__(self):
        ''' __init__(brother) __init__(sshape) Builds a ViewShape using the default constructor, copy constructor, or from a SShape .

        :param brother: A ViewShape object.
        :type brother: 'ViewShape'
        :param sshape: An SShape object.
        :type sshape: 'SShape'
        '''
        pass

    def add_edge(self, edge: 'ViewEdge'):
        ''' Adds a ViewEdge to the list of ViewEdge objects.

        :param edge: A ViewEdge object.
        :type edge: 'ViewEdge'
        '''
        pass

    def add_vertex(self, vertex: 'ViewVertex'):
        ''' Adds a ViewVertex to the list of the ViewVertex objects.

        :param vertex: A ViewVertex object.
        :type vertex: 'ViewVertex'
        '''
        pass


class ViewVertex:
    ''' Class hierarchy: Interface0D > ViewVertex Class to define a view vertex. A view vertex is a feature vertex corresponding to a point of the image graph, where the characteristics of an edge (e.g., nature and visibility) might change. A ViewVertex can be of two kinds: A TVertex when it corresponds to the intersection between two ViewEdges or a NonTVertex when it corresponds to a vertex of the initial input mesh (it is the case for vertices such as corners for example). Thus, this class can be specialized into two classes, the TVertex class and the NonTVertex class.
    '''

    nature: 'Nature' = None
    ''' The nature of this ViewVertex.

    :type: 'Nature'
    '''

    def edges_begin(self) -> 'orientedViewEdgeIterator':
        ''' Returns an iterator over the ViewEdges that goes to or comes from this ViewVertex pointing to the first ViewEdge of the list. The orientedViewEdgeIterator allows to iterate in CCW order over these ViewEdges and to get the orientation for each ViewEdge (incoming/outgoing).

        :rtype: 'orientedViewEdgeIterator'
        :return: An orientedViewEdgeIterator pointing to the first ViewEdge.
        '''
        pass

    def edges_end(self) -> 'orientedViewEdgeIterator':
        ''' Returns an orientedViewEdgeIterator over the ViewEdges around this ViewVertex, pointing after the last ViewEdge.

        :rtype: 'orientedViewEdgeIterator'
        :return: An orientedViewEdgeIterator pointing after the last ViewEdge.
        '''
        pass

    def edges_iterator(self, edge: 'ViewEdge') -> 'orientedViewEdgeIterator':
        ''' Returns an orientedViewEdgeIterator pointing to the ViewEdge given as argument.

        :param edge: A ViewEdge object.
        :type edge: 'ViewEdge'
        :rtype: 'orientedViewEdgeIterator'
        :return: An orientedViewEdgeIterator pointing to the given ViewEdge.
        '''
        pass


class orientedViewEdgeIterator:
    ''' Class hierarchy: Iterator > orientedViewEdgeIterator Class representing an iterator over oriented ViewEdges around a ViewVertex . This iterator allows a CCW iteration (in the image plane). An instance of an orientedViewEdgeIterator can only be obtained from a ViewVertex by calling edges_begin() or edges_end().
    '''

    object: typing.Union[bool, 'ViewEdge'] = None
    ''' The oriented ViewEdge (i.e., a tuple of the pointed ViewEdge and a boolean value) currently pointed to by this iterator. If the boolean value is true, the ViewEdge is incoming.

    :type: typing.Union[bool, 'ViewEdge']
    '''

    def __init__(self):
        ''' __init__(iBrother) Creates an orientedViewEdgeIterator using either the default constructor or the copy constructor.

        :param iBrother: An orientedViewEdgeIterator object.
        :type iBrother: 'orientedViewEdgeIterator'
        '''
        pass

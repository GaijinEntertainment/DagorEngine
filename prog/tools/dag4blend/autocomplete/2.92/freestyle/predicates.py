import sys
import typing
import freestyle.types


class AndBP1D:
    pass


class AndUP1D:
    pass


class ContourUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > ContourUP1D
    '''

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the Interface1D is a contour. An Interface1D is a contour if it is bordered by a different shape on each of its sides.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if the Interface1D is a contour, false otherwise.
        '''
        pass


class DensityLowerThanUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > DensityLowerThanUP1D
    '''

    def __init__(self, threshold: float, sigma: float = 2.0):
        ''' Builds a DensityLowerThanUP1D object.

        :param threshold: The value of the threshold density. Any Interface1D having a density lower than this threshold will match.
        :type threshold: float
        :param sigma: The sigma value defining the density evaluation window size used in the freestyle.functions.DensityF0D functor.
        :type sigma: float
        '''
        pass

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the density evaluated for the Interface1D is less than a user-defined density value.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if the density is lower than a threshold.
        '''
        pass


class EqualToChainingTimeStampUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > freestyle.types.EqualToChainingTimeStampUP1D
    '''

    def __init__(self, ts: int):
        ''' Builds a EqualToChainingTimeStampUP1D object.

        :param ts: A time stamp value.
        :type ts: int
        '''
        pass

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the Interface1D's time stamp is equal to a certain user-defined value.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if the time stamp is equal to a user-defined value.
        '''
        pass


class EqualToTimeStampUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > EqualToTimeStampUP1D
    '''

    def __init__(self, ts: int):
        ''' Builds a EqualToTimeStampUP1D object.

        :param ts: A time stamp value.
        :type ts: int
        '''
        pass

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the Interface1D's time stamp is equal to a certain user-defined value.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if the time stamp is equal to a user-defined value.
        '''
        pass


class ExternalContourUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > ExternalContourUP1D
    '''

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the Interface1D is an external contour. An Interface1D is an external contour if it is bordered by no shape on one of its sides.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if the Interface1D is an external contour, false otherwise.
        '''
        pass


class FalseBP1D:
    ''' Class hierarchy: freestyle.types.BinaryPredicate1D > FalseBP1D
    '''

    def __call__(self, inter1: 'freestyle.types.Interface1D',
                 inter2: 'freestyle.types.Interface1D') -> bool:
        ''' Always returns false.

        :param inter1: The first Interface1D object.
        :type inter1: 'freestyle.types.Interface1D'
        :param inter2: The second Interface1D object.
        :type inter2: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: False.
        '''
        pass


class FalseUP0D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate0D > FalseUP0D
    '''

    def __call__(self, it: 'freestyle.types.Interface0DIterator') -> bool:
        ''' Always returns false.

        :param it: An Interface0DIterator object.
        :type it: 'freestyle.types.Interface0DIterator'
        :rtype: bool
        :return: False.
        '''
        pass


class FalseUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > FalseUP1D
    '''

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Always returns false.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: False.
        '''
        pass


class Length2DBP1D:
    ''' Class hierarchy: freestyle.types.BinaryPredicate1D > Length2DBP1D
    '''

    def __call__(self, inter1: 'freestyle.types.Interface1D',
                 inter2: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the 2D length of inter1 is less than the 2D length of inter2.

        :param inter1: The first Interface1D object.
        :type inter1: 'freestyle.types.Interface1D'
        :param inter2: The second Interface1D object.
        :type inter2: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True or false.
        '''
        pass


class MaterialBP1D:
    ''' Checks whether the two supplied ViewEdges have the same material.
    '''

    pass


class NotBP1D:
    pass


class NotUP1D:
    pass


class ObjectNamesUP1D:
    pass


class OrBP1D:
    pass


class OrUP1D:
    pass


class QuantitativeInvisibilityRangeUP1D:
    pass


class QuantitativeInvisibilityUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > QuantitativeInvisibilityUP1D
    '''

    def __init__(self, qi: int = 0):
        ''' Builds a QuantitativeInvisibilityUP1D object.

        :param qi: The Quantitative Invisibility you want the Interface1D to have.
        :type qi: int
        '''
        pass

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the Quantitative Invisibility evaluated at an Interface1D, using the freestyle.functions.QuantitativeInvisibilityF1D functor, equals a certain user-defined value.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if Quantitative Invisibility equals a user-defined value.
        '''
        pass


class SameShapeIdBP1D:
    ''' Class hierarchy: freestyle.types.BinaryPredicate1D > SameShapeIdBP1D
    '''

    def __call__(self, inter1: 'freestyle.types.Interface1D',
                 inter2: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if inter1 and inter2 belong to the same shape.

        :param inter1: The first Interface1D object.
        :type inter1: 'freestyle.types.Interface1D'
        :param inter2: The second Interface1D object.
        :type inter2: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True or false.
        '''
        pass


class ShapeUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > ShapeUP1D
    '''

    def __init__(self, first: int, second: int = 0):
        ''' Builds a ShapeUP1D object.

        :param first: The first Id component.
        :type first: int
        :param second: The second Id component.
        :type second: int
        '''
        pass

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the shape to which the Interface1D belongs to has the same freestyle.types.Id as the one specified by the user.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True if Interface1D belongs to the shape of the user-specified Id.
        '''
        pass


class TrueBP1D:
    ''' Class hierarchy: freestyle.types.BinaryPredicate1D > TrueBP1D
    '''

    def __call__(self, inter1: 'freestyle.types.Interface1D',
                 inter2: 'freestyle.types.Interface1D') -> bool:
        ''' Always returns true.

        :param inter1: The first Interface1D object.
        :type inter1: 'freestyle.types.Interface1D'
        :param inter2: The second Interface1D object.
        :type inter2: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True.
        '''
        pass


class TrueUP0D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate0D > TrueUP0D
    '''

    def __call__(self, it: 'freestyle.types.Interface0DIterator') -> bool:
        ''' Always returns true.

        :param it: An Interface0DIterator object.
        :type it: 'freestyle.types.Interface0DIterator'
        :rtype: bool
        :return: True.
        '''
        pass


class TrueUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > TrueUP1D
    '''

    def __call__(self, inter: 'freestyle.types.Interface1D') -> bool:
        ''' Always returns true.

        :param inter: An Interface1D object.
        :type inter: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True.
        '''
        pass


class ViewMapGradientNormBP1D:
    ''' Class hierarchy: freestyle.types.BinaryPredicate1D > ViewMapGradientNormBP1D
    '''

    def __init__(self,
                 level: int,
                 integration_type:
                 'freestyle.types.IntegrationType' = 'IntegrationType.MEAN',
                 sampling: float = 2.0):
        ''' Builds a ViewMapGradientNormBP1D object.

        :param level: The level of the pyramid from which the pixel must be read.
        :type level: int
        :param integration_type: The integration method used to compute a single value from a set of values.
        :type integration_type: 'freestyle.types.IntegrationType'
        :param sampling: GetViewMapGradientNormF0D is evaluated at each sample point and the result is obtained by combining the resulting values into a single one, following the method specified by integration_type.
        :type sampling: float
        '''
        pass

    def __call__(self, inter1: 'freestyle.types.Interface1D',
                 inter2: 'freestyle.types.Interface1D') -> bool:
        ''' Returns true if the evaluation of the Gradient norm Function is higher for inter1 than for inter2.

        :param inter1: The first Interface1D object.
        :type inter1: 'freestyle.types.Interface1D'
        :param inter2: The second Interface1D object.
        :type inter2: 'freestyle.types.Interface1D'
        :rtype: bool
        :return: True or false.
        '''
        pass


class WithinImageBoundaryUP1D:
    ''' Class hierarchy: freestyle.types.UnaryPredicate1D > WithinImageBoundaryUP1D
    '''

    def __init__(self, xmin: float, ymin: float, xmax: float, ymax: float):
        ''' Builds an WithinImageBoundaryUP1D object.

        :param xmin: X lower bound of the image boundary.
        :type xmin: float
        :param ymin: Y lower bound of the image boundary.
        :type ymin: float
        :param xmax: X upper bound of the image boundary.
        :type xmax: float
        :param ymax: Y upper bound of the image boundary.
        :type ymax: float
        '''
        pass

    def __call__(self, inter):
        ''' Returns true if the Interface1D intersects with image boundary.

        '''
        pass


class pyBackTVertexUP0D:
    ''' Check whether an Interface0DIterator references a TVertex and is the one that is hidden (inferred from the context).
    '''

    pass


class pyClosedCurveUP1D:
    pass


class pyDensityFunctorUP1D:
    pass


class pyDensityUP1D:
    pass


class pyDensityVariableSigmaUP1D:
    pass


class pyHighDensityAnisotropyUP1D:
    pass


class pyHighDirectionalViewMapDensityUP1D:
    pass


class pyHighSteerableViewMapDensityUP1D:
    pass


class pyHighViewMapDensityUP1D:
    pass


class pyHighViewMapGradientNormUP1D:
    pass


class pyHigherCurvature2DAngleUP0D:
    pass


class pyHigherLengthUP1D:
    pass


class pyHigherNumberOfTurnsUP1D:
    pass


class pyIsInOccludersListUP1D:
    pass


class pyIsOccludedByIdListUP1D:
    pass


class pyIsOccludedByItselfUP1D:
    pass


class pyIsOccludedByUP1D:
    pass


class pyLengthBP1D:
    pass


class pyLowDirectionalViewMapDensityUP1D:
    pass


class pyLowSteerableViewMapDensityUP1D:
    pass


class pyNFirstUP1D:
    pass


class pyNatureBP1D:
    pass


class pyNatureUP1D:
    pass


class pyParameterUP0D:
    pass


class pyParameterUP0DGoodOne:
    pass


class pyProjectedXBP1D:
    pass


class pyProjectedYBP1D:
    pass


class pyShapeIdListUP1D:
    pass


class pyShapeIdUP1D:
    pass


class pyShuffleBP1D:
    pass


class pySilhouetteFirstBP1D:
    pass


class pyUEqualsUP0D:
    pass


class pyVertexNatureUP0D:
    pass


class pyViewMapGradientNormBP1D:
    pass


class pyZBP1D:
    pass


class pyZDiscontinuityBP1D:
    pass


class pyZSmallerUP1D:
    pass

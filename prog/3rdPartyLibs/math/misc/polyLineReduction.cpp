#include "polyLineReduction.h"
#include <math.h>
#include <stdlib.h>

namespace polylinereduction
{
  struct STACK_RECORD
  {
    int nAnchorIndex, nFloaterIndex;
    STACK_RECORD *precPrev;
  };

  static STACK_RECORD *m_pStack = NULL;
  static void StackPush( int nAnchorIndex, int nFloaterIndex );
  static int StackPop( int *pnAnchorIndex, int *pnFloaterIndex );
}

using namespace polylinereduction;

void ReducePoints(double *pPointsX, double *pPointsY, int nPointsCount,
                  int *pnUseFlag,
                  double dTolerance)
{
  int nVertexIndex, nAnchorIndex, nFloaterIndex;
  double dSegmentVecLength;
  double dAnchorVecX, dAnchorVecY;
  double dAnchorUnitVecX, dAnchorUnitVecY;
  double dVertexVecLength;
  double dVertexVecX, dVertexVecY;
  double dProjScalar;
  double dVertexDistanceToSegment;
  double dMaxDistThisSegment;
  int nVertexIndexMaxDistance;
   
  nAnchorIndex = 0;
  nFloaterIndex = nPointsCount - 1;
  StackPush( nAnchorIndex, nFloaterIndex );
  while ( StackPop( &nAnchorIndex, &nFloaterIndex ) ) {
    // initialize line segment
    dAnchorVecX = pPointsX[ nFloaterIndex ] - pPointsX[ nAnchorIndex ];
    dAnchorVecY = pPointsY[ nFloaterIndex ] - pPointsY[ nAnchorIndex ];
    dSegmentVecLength = sqrt( dAnchorVecX * dAnchorVecX
          + dAnchorVecY * dAnchorVecY );
    if (dSegmentVecLength > 1e-12)
    {
      dAnchorUnitVecX = dAnchorVecX / dSegmentVecLength;
      dAnchorUnitVecY = dAnchorVecY / dSegmentVecLength;
    } else
    {
      dAnchorUnitVecX = dAnchorVecX;
      dAnchorUnitVecY = dAnchorVecY;
    }

    // inner loop:
    dMaxDistThisSegment = 0.0;
    nVertexIndexMaxDistance = nAnchorIndex + 1;
    for ( nVertexIndex = nAnchorIndex + 1; nVertexIndex < nFloaterIndex; nVertexIndex++ )
    {
      //compare to anchor
      dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nAnchorIndex ];
      dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nAnchorIndex ];
      dVertexVecLength = sqrt( dVertexVecX * dVertexVecX + dVertexVecY * dVertexVecY );
      //dot product:
      dProjScalar = dVertexVecX * dAnchorUnitVecX + dVertexVecY * dAnchorUnitVecY;
      if ( dProjScalar < 0.0 )
        dVertexDistanceToSegment = dVertexVecLength;
      else
      {
        //compare to floater
        dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nFloaterIndex ];
        dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nFloaterIndex ];
        dVertexVecLength = sqrt( dVertexVecX * dVertexVecX + dVertexVecY * dVertexVecY );
        //dot product:
        dProjScalar = dVertexVecX * (-dAnchorUnitVecX) + dVertexVecY * (-dAnchorUnitVecY);
        if ( dProjScalar < 0.0 )
          dVertexDistanceToSegment = dVertexVecLength;
        else //calculate perpendicular distance to line (pythagorean theorem):
          dVertexDistanceToSegment =
            sqrt( fabs( dVertexVecLength * dVertexVecLength - dProjScalar * dProjScalar ) );
      } //else
      if ( dMaxDistThisSegment < dVertexDistanceToSegment )
      {
        dMaxDistThisSegment = dVertexDistanceToSegment;
        nVertexIndexMaxDistance = nVertexIndex;
      } //if
    } //for (inner loop)
    if ( dMaxDistThisSegment <= dTolerance ) 
    { //use line segment
      pnUseFlag[ nAnchorIndex ] = 1;
      pnUseFlag[ nFloaterIndex ] = 1;
    } //if use points
    else
    {
      StackPush( nAnchorIndex, nVertexIndexMaxDistance );
      StackPush( nVertexIndexMaxDistance, nFloaterIndex );
    } //else
  } //while (outer loop)
} //end of ReducePoints() function
 
static void polylinereduction::StackPush( int nAnchorIndex, int nFloaterIndex )
{
  STACK_RECORD *precPrev = m_pStack;
  m_pStack = (STACK_RECORD *)malloc( sizeof(STACK_RECORD) );
  m_pStack->nAnchorIndex = nAnchorIndex;
  m_pStack->nFloaterIndex = nFloaterIndex;
  m_pStack->precPrev = precPrev;
} //end of StackPush() function
 
static int polylinereduction::StackPop( int *pnAnchorIndex, int *pnFloaterIndex )
{
  STACK_RECORD *precStack = m_pStack;
  if ( precStack == NULL )
    return 0; //false
  *pnAnchorIndex = precStack->nAnchorIndex;
  *pnFloaterIndex = precStack->nFloaterIndex;
  m_pStack = precStack->precPrev;
  free( precStack );
  return 1; //true
} //end of StackPop() function

//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

#include <float.h>
#include <string.h>
#include "detourNavMeshQuery.h"
#include "detourNavMesh.h"
#include "detourNode.h"
#include "detourCommon.h"
#include "detourMath.h"
#include "detourAlloc.h"
#include "detourAssert.h"
#include <new>

dtStatus dtNavMeshQuery::getPathToNode(dtNode* endNode, dtPolyRef* path, int* pathCount, int maxPath, float maxWeight) const
{
	// Find the length of the entire path.
	dtNode* curNode = endNode;
	int length = 0;
	do
	{
		length++;
		curNode = m_nodePool->getNodeAtIdx(curNode->pidx);
	} while (curNode);

	// If the path cannot be fully stored then advance to the last node we will be able to store.
	curNode = endNode;
	int writeCount;
	for (writeCount = length; writeCount > maxPath; writeCount--)
	{
		dtAssert(curNode);

		curNode = m_nodePool->getNodeAtIdx(curNode->pidx);
	}

	// If path weights too much then advance to the last node we will be able to store (start node is always stored).
	for (; writeCount > 1; writeCount--)
	{
		dtAssert(curNode);

		if (curNode->total <= maxWeight)
			break;

		curNode = m_nodePool->getNodeAtIdx(curNode->pidx);
	}

	// if last node after pruning is start of jump link then drop it
	if (writeCount > 1)
	{
		const dtMeshTile* tile = 0;
		const dtPoly* poly = 0;
		m_nav->getTileAndPolyByRefUnsafe(curNode->id, &tile, &poly);
		if (poly->flags & 0x08)
		{
			--writeCount;
			curNode = m_nodePool->getNodeAtIdx(curNode->pidx);
		}
	}

	// Write path
	for (int i = writeCount - 1; i >= 0; i--)
	{
		dtAssert(curNode);

		path[i] = curNode->id;
		curNode = m_nodePool->getNodeAtIdx(curNode->pidx);
	}

	dtAssert(!curNode);

	*pathCount = writeCount;

	dtStatus status = DT_SUCCESS;

	if (length > maxPath)
		status |= DT_BUFFER_TOO_SMALL;
	if (writeCount < dtMin(length, maxPath))
		status |= DT_PARTIAL_RESULT;

	return status;
}

/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "omm_platform.hlsli"
#include "omm.hlsli"
#include "omm_global_cb.hlsli"
#include "omm_global_samplers.hlsli"
#include "omm_common.hlsli"

namespace hashTable
{
	enum class Result
	{
		Null, // not initialized

		Found,
		Inserted,
		ReachedMaxAttemptCount, //
	};

	// returns offset in hash table for a given hash,
	// if Inserted, return value will be input value.
	// if Found, return value will be value at hash table entry location
	// if ReachedMaxAttemptCount, return value is undefined.
	Result FindOrInsertValue(uint hash, out uint hashTableEntryIndex)
	{
		hashTableEntryIndex = hash % g_GlobalConstants.TexCoordHashTableEntryCount;

		const uint kMaxNumAttempts		= 16;	// Completely arbitrary.
		const uint kInvalidEntryHash	= 0;	// Buffer must be cleared before dispatch.

		ALLOW_UAV_CONDITION
		for (uint attempts = 0; attempts < kMaxNumAttempts; ++attempts)
		{
			uint existingHash = 0;
			OMM_SUBRESOURCE_CAS(HashTableBuffer, 8 * hashTableEntryIndex, kInvalidEntryHash, hash, existingHash); // Each entry consists of [hash|primitiveId]

			// Inserted.
			if (existingHash == kInvalidEntryHash)
				return Result::Inserted;

			// Entry was already inserted.
			if (existingHash == hash)
				return Result::Found;

			// Conflict, keep searching using lienar probing 
			hashTableEntryIndex = (hashTableEntryIndex + 1) % g_GlobalConstants.TexCoordHashTableEntryCount;
		}

		return Result::ReachedMaxAttemptCount;
	}

	void Store(uint hashTableEntryIndex, uint value)
	{
		OMM_SUBRESOURCE_STORE(HashTableBuffer, 8 * hashTableEntryIndex + 4, value);
	}
}

hashTable::Result FindOrInsertOMMEntry(TexCoords texCoords, uint subdivisionLevel, out uint hashTableEntryIndex)
{
	hashTableEntryIndex = 0;
	if (g_GlobalConstants.EnableTexCoordDeduplication)
	{
		const uint hash = GetHash(texCoords, subdivisionLevel);

		return hashTable::FindOrInsertValue(hash, hashTableEntryIndex);
	}
	else
	{
		return hashTable::Result::Null;
	}
}
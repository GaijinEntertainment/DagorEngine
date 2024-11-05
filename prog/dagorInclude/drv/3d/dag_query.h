//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class D3dEventQuery;

namespace d3d
{

/**
 * @brief Opaque type, a pointer to which represents an event query
 */
using EventQuery = D3dEventQuery;

/**
 * @brief Creates a new event query.
 * @return A pointer to the created event query, or nullptr if not supported or failed (device reset e.g.).
 */
EventQuery *create_event_query();

/**
 * @brief Releases the specified event query.
 * @param query The event query to release.
 */
void release_event_query(EventQuery *query);

/**
 * @brief Issues the specified event query.
 * @param query The event query to issue.
 * @return True if the query was successfully issued, false otherwise.
 */
bool issue_event_query(EventQuery *query);

/**
 * @brief Gets the status of the specified event query.
 * @param query The event query to check.
 * @param force_flush Whether to force a flush before checking the status.
 * @return False if the query is issued but not yet signaled, true otherwise (signaled, not issued, or bad query).
 */
bool get_event_query_status(EventQuery *query, bool force_flush);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline EventQuery *create_event_query() { return d3di.create_event_query(); }
inline void release_event_query(EventQuery *q) { return d3di.release_event_query(q); }
inline bool issue_event_query(EventQuery *q) { return d3di.issue_event_query(q); }
inline bool get_event_query_status(EventQuery *q, bool force_flush) { return d3di.get_event_query_status(q, force_flush); }
} // namespace d3d
#endif

#include "nodeScheduler.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/bitvector.h>
#include <EASTL/stack.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>

#include <debug/dag_assert.h>
#include <debug/backendDebug.h>

#include <id/idRange.h>


namespace dabfg
{

auto NodeScheduler::schedule(const intermediate::Graph &graph) -> NodePermutation
{
  NodePermutation outTime(graph.nodes.size());

  // Simple non-recursive DFS to find a toplogical sort using out times.

  // As per classical algorithms literature,
  // white = unvisited, gray = current path, black = visited
  enum class Color
  {
    WHITE,
    GRAY,
    BLACK
  };
  dag::Vector<Color, framemem_allocator> colors(graph.nodes.size(), Color::WHITE);

  // NOTE: This is a crutch that forces different eyes to be run in-order.
  // Saves us while we have global state not tracked by framegraph yet.
  // TODO: Remove when DNG no longer has any global state (so basically never)
  auto orderRange = IdRange<intermediate::NodeIndex>(graph.nodes.size());
  dag::Vector<intermediate::NodeIndex, framemem_allocator> dfsLaunchOrder(orderRange.begin(), orderRange.end());

  // We can use the same lambda to order descending using .rbegin()/.rend().
  auto orderComp = [&graph](intermediate::NodeIndex l, intermediate::NodeIndex r) {
    if (graph.nodes[l].multiplexingIndex != graph.nodes[r].multiplexingIndex)
      return graph.nodes[l].multiplexingIndex < graph.nodes[r].multiplexingIndex;
    return graph.nodes[l].priority < graph.nodes[r].priority;
  };

  eastl::sort(dfsLaunchOrder.begin(), dfsLaunchOrder.end(), orderComp);

  uint32_t timer = 0;
  for (auto i : dfsLaunchOrder)
  {
    if (colors[i] != Color::WHITE)
      continue;

    // This stack will be on top of the framemem stack throughout the dfs,
    // so expanding is FREE
    eastl::stack<intermediate::NodeIndex, dag::Vector<intermediate::NodeIndex, framemem_allocator>> stack;
    stack.push(i);

    // Loop until all reachable nodes were visited
    while (!stack.empty())
    {
      auto curr = stack.top();

      // If this is a fresh node that we haven't seen yet,
      // mark it gray and enqueue all predecessors
      // Note that we do NOT pop the curr node from the stack!!!
      if (colors[curr] == Color::WHITE)
      {
        colors[curr] = Color::GRAY;

        const auto &unsortedPreds = graph.nodes[curr].predecessors;

        // This is a very important reservation, it makes frameemem
        // allocation scopes nest. We first pre-allocate enough space
        // in the stack for all predecessors and only then allocate a
        // the `predecessors` vector. This guarantees that the following
        // pushes don't need to allocate more memory inside the stack.
        // Without this, pushing could cause a reallocation, which would
        // force framemem to completely move the stack to different memory :(
        stack.get_container().reserve(stack.size() + unsortedPreds.size());

        // Our current hack for priorities. I don't like it, but can't
        // think of anything better.
        dag::Vector<intermediate::NodeIndex, framemem_allocator> predecessors(unsortedPreds.begin(), unsortedPreds.end());
        eastl::sort(predecessors.rbegin(), predecessors.rend(), orderComp);

        for (intermediate::NodeIndex next : predecessors)
        {
          if (DAGOR_UNLIKELY(colors[next] == Color::GRAY))
          {
            // We've found a cycle, the world is sort of broken, complain.
            const auto &stackRaw = stack.get_container();
            const auto it = eastl::find(stackRaw.begin(), stackRaw.end(), next);

            PerCompString path;
            eastl::bitvector<framemem_allocator> visited{graph.nodes.size(), false};
            for (auto i = it; i != stackRaw.end(); ++i)
              if (colors[*i] == Color::GRAY && !visited.test(*i, false))
              {
                visited.set(*i, true);
                path += graph.nodes[*i].name + " -> ";
              }
            path += graph.nodes[*it].name;
            graphDumper.dumpRawUserGraph();
            logerr("Cyclic dependency detected in framegraph IR: %s", path);
            logerr("For resource dependency dump look at debug log!");
          }

          if (colors[next] == Color::WHITE)
            stack.push(next);
        }

        continue;
      }

      // After all children of a node have been completely visited,
      // we will find this node at the top of the stack again,
      // this time with gray mark. Mark it black and record out time.
      // Conceptually our algorithm is "leaving" this node,
      // so we record the leaving time.
      if (colors[curr] == Color::GRAY)
      {
        outTime[curr] = static_cast<intermediate::NodeIndex>(timer++);
        colors[curr] = Color::BLACK;
      }

      // The node might have been added multiple time to the stack,
      // but only the last entry got "visited", so ignore others. E.g.:
      //       B
      //      ^ ^
      //     /   \
      //    /     \
      //   /       \
      //  A-------->C
      // Stack after each iteration:
      // A
      // ABC
      // ABCB
      // ABCB
      // ABC
      // AB  <- B is black here, because it was already visited
      // A
      stack.pop();
    }
  }

  return outTime;
}

} // namespace dabfg

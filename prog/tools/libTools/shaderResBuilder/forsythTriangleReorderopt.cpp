//-----------------------------------------------------------------------------
//  This is an implementation of Tom Forsyth's "Linear-Speed Vertex Cache
//  Optimization" algorithm as described here:
//  http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
//
//  This code was authored and released into the public domain by
//  Adrian Stone (stone@gameangst.com).
//
//  THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
//  SHALL ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER
//  LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
//  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include <assert.h>
#include <math.h>
#include <vector>
#include <limits>
#include <algorithm>
#include "forsythTriangleReorderopt.h"
#include "forsythTriangleReorderoptClass.h"

namespace Forsyth
{
//-----------------------------------------------------------------------------
//  OptimizeFaces
//-----------------------------------------------------------------------------
//  Parameters:
//      indexList
//          input index list
//      indexCount
//          the number of indices in the list
//      vertexCount
//          the largest index value in indexList
//      newIndexList
//          a pointer to a preallocated buffer the same size as indexList to
//          hold the optimized index list
//      lruCacheSize
//          the size of the simulated post-transform cache (max:64)
//-----------------------------------------------------------------------------
template <class Indices>
void OptimizeFaces(const Indices *indexList, unsigned indexCount, unsigned max_index, Indices *newIndexList, unsigned lruCacheSize)
{
  CacheOptimizer<Indices> optimizer;
  optimizer.OptimizeFaces(indexList, indexCount, max_index, newIndexList, lruCacheSize);
}
template void OptimizeFaces<unsigned short>(const unsigned short *indexList, unsigned indexCount, unsigned vertexCount,
  unsigned short *newIndexList, unsigned lruCacheSize);
template void OptimizeFaces<unsigned int>(const unsigned int *indexList, unsigned indexCount, unsigned vertexCount,
  unsigned int *newIndexList, unsigned lruCacheSize);


} // namespace Forsyth

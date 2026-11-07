// the probes ask for cells they missed, then for refreshes: the counts of both lists, of the screen probe misses, of the voxel list,
// the records (position, packed normal) of each list, a bit per hashed cell, the screen probe misses, the feed's cell bits
#define DAGI_SC_REQUESTS_CAPACITY 8192
#define DAGI_SC_REFRESH_CAPACITY 4096
#define DAGI_SC_REQUESTS_HEADER 16
#define DAGI_SC_REQUESTS_RECORD 16
#define DAGI_SC_REQUESTS_BITS 65536
#define DAGI_SC_REQUESTS_BITS_OFFSET \
  (DAGI_SC_REQUESTS_HEADER + (DAGI_SC_REQUESTS_CAPACITY + DAGI_SC_REFRESH_CAPACITY) * DAGI_SC_REQUESTS_RECORD)
#define DAGI_SC_SCREEN_MISSES_COUNT 8
#define DAGI_SC_SCREEN_MISSES_CAPACITY 32768
#define DAGI_SC_SCREEN_MISSES_OFFSET (DAGI_SC_REQUESTS_BITS_OFFSET + DAGI_SC_REQUESTS_BITS / 8)
#define DAGI_SC_FEED_BITS (1 << 17)
#define DAGI_SC_FEED_BITS_OFFSET (DAGI_SC_SCREEN_MISSES_OFFSET + DAGI_SC_SCREEN_MISSES_CAPACITY * 4)
#define DAGI_SC_REQUESTS_SIZE (DAGI_SC_FEED_BITS_OFFSET + DAGI_SC_FEED_BITS / 8)
#define DAGI_SC_VOXELS_LIST_COUNT 12
// more voxels wait for the next frame
#define DAGI_SC_VOXELS_LIST_CAPACITY 32768
// threads in a row of the voxel dispatch: a dispatch takes at most 65535 groups a dimension
#define DAGI_SC_VOXELS_ROW 65536
// a new cell evicts only an entry that no store wrote for this many frames
#define DAGI_SC_EVICT_AGE 64

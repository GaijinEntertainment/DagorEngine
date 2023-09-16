#ifndef WAVE_UTILS_INCLUDED
#define WAVE_UTILS_INCLUDED 1

#if WAVE_INTRINSICS

//wave voting interlocked adds. Use instead of InterlockedAdd
// *Raw* is for RWByteAddressedBuffer
//Original returns original value (no overloads are possible in macro)

#define WaveInterlockedAddRawUint( AT, DEST, VALUE ) \
{ \
  uint addTotal = WaveActiveCountBits( true ) * VALUE; \
  if ( WaveIsHelperLane() ) \
    AT . InterlockedAdd( DEST, addTotal ); \
}

#define WaveInterlockedAddRawUintOriginal( AT, DEST, VALUE, ORIGINAL_VALUE ) \
{ \
  uint addTotal = WaveActiveCountBits( true ) * VALUE; \
  if ( WaveIsHelperLane() ) \
    AT . InterlockedAdd( DEST, addTotal, ORIGINAL_VALUE ); \
  ORIGINAL_VALUE = WaveReadFirstLane( ORIGINAL_VALUE ) + WavePrefixCountBits( true ) * VALUE; \
}

#define WaveInterlockedAddRaw( AT, DEST, VALUE ) \
{ \
  uint addTotal = WaveActiveSum( VALUE ); \
  if ( WaveIsHelperLane() ) \
    AT . InterlockedAdd( DEST, addTotal ); \
}

#define WaveInterlockedAddRawOriginal( AT, DEST, VALUE, ORIGINAL_VALUE ) \
{ \
  uint localOfs = WavePrefixSum( VALUE ); \
  uint addTotal = WaveReadLaneLast( localOfs + VALUE ); \
  if ( WaveIsHelperLane() ) \
    AT . InterlockedAdd( DEST, addTotal, ORIGINAL_VALUE ); \
  ORIGINAL_VALUE = WaveReadFirstLane( ORIGINAL_VALUE ) + localOfs; \
}


#define WaveInterlockedAddUint( DEST, VALUE ) \
{ \
  uint addTotal = WaveActiveCountBits( true ) * VALUE; \
  if ( WaveIsHelperLane() ) \
    InterlockedAdd( DEST, addTotal ); \
}

#define WaveInterlockedAddUintOriginal( DEST, VALUE, ORIGINAL_VALUE ) \
{ \
  uint addTotal = WaveActiveCountBits( true ) * VALUE; \
  if ( WaveIsHelperLane() ) \
    InterlockedAdd( DEST, addTotal, ORIGINAL_VALUE ); \
  ORIGINAL_VALUE = WaveReadFirstLane( ORIGINAL_VALUE ) + WavePrefixCountBits( true ) * VALUE; \
}

#define WaveInterlockedAdd( DEST, VALUE ) \
{ \
  uint addTotal = WaveActiveSum( VALUE ); \
  if ( WaveIsHelperLane() ) \
    InterlockedAdd( DEST, addTotal ); \
}

#define WaveInterlockedAddOriginal( DEST, VALUE, ORIGINAL_VALUE ) \
{ \
  uint localOfs = WavePrefixSum( VALUE ); \
  uint addTotal = WaveReadLaneLast( localOfs + VALUE ); \
  if ( WaveIsHelperLane() ) \
    InterlockedAdd( DEST, addTotal, ORIGINAL_VALUE ); \
  ORIGINAL_VALUE = WaveReadFirstLane( ORIGINAL_VALUE ) + localOfs; \
}

#else

#define WaveInterlockedAddRawUint( AT, DEST, VALUE ) AT.InterlockedAdd( DEST, (uint)(VALUE) )
#define WaveInterlockedAddRawUintOriginal( AT, DEST, VALUE, ORIGINAL_VALUE ) \
  AT.InterlockedAdd( DEST, (uint)(VALUE), ORIGINAL_VALUE )
#define WaveInterlockedAddRaw( AT, DEST, VALUE ) AT.InterlockedAdd( DEST, VALUE )
#define WaveInterlockedAddRawOriginal( AT, DEST, VALUE, ORIGINAL_VALUE ) AT.InterlockedAdd( DEST, VALUE, ORIGINAL_VALUE )

#define WaveInterlockedAddUint( DEST, VALUE ) InterlockedAdd( DEST, (uint)(VALUE) )
#define WaveInterlockedAddUintOriginal( DEST, VALUE, ORIGINAL_VALUE ) \
  InterlockedAdd( DEST, (uint)(VALUE), ORIGINAL_VALUE )
#define WaveInterlockedAdd( DEST, VALUE ) InterlockedAdd( DEST, VALUE )
#define WaveInterlockedAddOriginal( DEST, VALUE, ORIGINAL_VALUE ) InterlockedAdd( DEST, VALUE, ORIGINAL_VALUE )

#endif //WAVE_INTRINSICS

#endif //WAVE_UTILS_INCLUDED

#pragma once

#include <assert.h>
#include <string.h>


namespace NetImgui { namespace Internal
{

template <typename TType, typename... Args>
TType* netImguiNew(Args... args)
{
	return new( ImGui::MemAlloc(sizeof(TType)) ) TType(args...);
}

template <typename TType>
TType* netImguiSizedNew(size_t placementSize)
{
	return new( ImGui::MemAlloc(placementSize > sizeof(TType) ? placementSize : sizeof(TType)) ) TType();
}

template <typename TType>
void netImguiDelete(TType* pData)
{
	if( pData )
	{
		pData->~TType();
		ImGui::MemFree(pData);
	}
}

template <typename TType>
void netImguiDeleteSafe(TType*& pData)
{
	netImguiDelete(pData);
	pData = nullptr;
}

//=============================================================================
// Acquire ownership of the resource
//=============================================================================
template <typename TType>
TType* ExchangePtr<TType>::Release()
{
	return mpData.exchange(nullptr);
}

//-----------------------------------------------------------------------------
// Take ownership of the provided data.
// If there's a previous unclaimed pointer to some data, release it
//-----------------------------------------------------------------------------
template <typename TType>
void ExchangePtr<TType>::Assign(TType*& pNewData)
{
	netImguiDelete( mpData.exchange(pNewData) );
	pNewData = nullptr;
}

template <typename TType>
void ExchangePtr<TType>::Free()
{
	TType* pNull(nullptr);
	Assign(pNull);
}

template <typename TType>
ExchangePtr<TType>::~ExchangePtr()
{
	Free();
}

//=============================================================================
//
//=============================================================================
template <typename TType>
OffsetPointer<TType>::OffsetPointer()
: mOffset(0)
{
	SetOff(0);
}

template <typename TType>
OffsetPointer<TType>::OffsetPointer(TType* pPointer)
{
	SetPtr(pPointer);
}

template <typename TType>
OffsetPointer<TType>::OffsetPointer(uint64_t offset)
{
	SetOff(offset);
}

template <typename TType>
void OffsetPointer<TType>::SetPtr(TType* pPointer)
{
	mOffset		= 0; // Remove 'offset flag' that can be left active on non 64bits pointer
	mPointer	= pPointer;
}

template <typename TType>
void OffsetPointer<TType>::SetComDataPtr(ComDataType* pPointer)
{
	SetPtr(reinterpret_cast<TType*>(pPointer));
}

template <typename TType>
void OffsetPointer<TType>::SetOff(uint64_t offset)
{
	mOffset = offset | 0x0000000000000001u;
}

template <typename TType>
uint64_t OffsetPointer<TType>::GetOff()const
{
	return mOffset & ~0x0000000000000001u;
}

template <typename TType>
bool OffsetPointer<TType>::IsOffset()const
{
	return (mOffset & 0x0000000000000001u) != 0;
}

template <typename TType>
bool OffsetPointer<TType>::IsPointer()const
{
	return !IsOffset();
}

template <typename TType>
TType* OffsetPointer<TType>::ToPointer()
{
	assert(IsOffset());
	SetPtr( reinterpret_cast<TType*>( reinterpret_cast<uint64_t>(&mPointer) + GetOff() ) );
	return mPointer;
}

template <typename TType>
uint64_t OffsetPointer<TType>::ToOffset()
{
	assert(IsPointer());
	SetOff( reinterpret_cast<uint64_t>(mPointer) - reinterpret_cast<uint64_t>(&mPointer) );
	return mOffset;
}

template <typename TType>
TType* OffsetPointer<TType>::operator->()
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const TType* OffsetPointer<TType>::operator->()const
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
TType* OffsetPointer<TType>::Get()
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const TType* OffsetPointer<TType>::Get()const
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const ComDataType* OffsetPointer<TType>::GetComData()const
{
	return reinterpret_cast<const ComDataType*>(Get());
}

template <typename TType>
TType& OffsetPointer<TType>::operator[](size_t index)
{
	assert(IsPointer());
	return mPointer[index];
}

template <typename TType>
const TType& OffsetPointer<TType>::operator[](size_t index)const
{
	assert(IsPointer());
	return mPointer[index];
}

//=============================================================================
template <typename TType, size_t TCount>
void Ringbuffer<TType,TCount>::AddData(const TType* pData, size_t& count)
//=============================================================================
{
	size_t i(0);
	while (i < count && (mPosLast - mPosCur < TCount)) {
		mBuffer[mPosLast % TCount] = pData[i];
		mPosLast++;
		i++;
	}
	count = i;
}

//=============================================================================
template <typename TType, size_t TCount>
bool Ringbuffer<TType,TCount>::ReadData(TType* pData)
//=============================================================================
{
	if (mPosCur < mPosLast)
	{
		*pData = mBuffer[mPosCur % TCount];
		mPosCur++;
		return true;
	}
	return false;
}


//=============================================================================
// The _s string functions are a mess. There's really no way to do this right
// in a cross-platform way. Best solution I've found is to set just use
// strncpy, infer the buffer length, and null terminate. Still need to suppress
// the warning on Windows.
// See https://randomascii.wordpress.com/2013/04/03/stop-using-strncpy-already/
// and many other discussions online on the topic.
//=============================================================================
template <size_t charCount>
void StringCopy(char (&output)[charCount], const char* pSrc, size_t srcCharCount)
{
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
	#pragma warning (push)
	#pragma warning (disable: 4996)	// warning C4996: 'strncpy': This function or variable may be unsafe.
#endif

	size_t charToCopyCount = charCount < srcCharCount + 1 ? charCount : srcCharCount + 1;
	strncpy(output, pSrc, charToCopyCount - 1);
	output[charCount - 1] = 0;

#if defined(_MSC_VER) && defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning (pop)
#endif
}

template <size_t charCount>
int StringFormat(char(&output)[charCount], char const* const format, ...)
{
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

	va_list args;
    va_start(args, format);
	int w = vsnprintf(output, charCount, format, args);
	va_end(args);
	output[charCount - 1] = 0;
	return w;

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif
}

//=============================================================================
//=============================================================================
template <typename IntType>
IntType DivUp(IntType Value, IntType Denominator)
{
	return (Value + Denominator - 1) / Denominator;
}

template <typename IntType>
IntType RoundUp(IntType Value, IntType Round)
{
	return DivUp(Value, Round) * Round;
}

union TextureCastHelperUnion
{
	ImTextureID TextureID;
	uint64_t	TextureUint;
	const void*	TexturePtr;
};

uint64_t TextureCastFromID(ImTextureID textureID)
{
	TextureCastHelperUnion textureUnion;
	textureUnion.TextureUint	= 0;
	textureUnion.TextureID		= textureID;
	return textureUnion.TextureUint;
}

ImTextureID TextureCastFromPtr(void* pTexture)
{
	TextureCastHelperUnion textureUnion;
	textureUnion.TextureUint	= 0;
	textureUnion.TexturePtr		= pTexture;
	return textureUnion.TextureID;
}

#ifndef IS_NETIMGUISERVER
ImTextureID TextureCastFromUInt(uint64_t textureID)
{
	TextureCastHelperUnion textureUnion;
	textureUnion.TextureUint = textureID;
	return textureUnion.TextureID;
}
#endif

}} //namespace NetImgui::Internal

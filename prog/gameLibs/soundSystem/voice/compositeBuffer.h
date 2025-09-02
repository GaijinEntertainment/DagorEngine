// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/deque.h>
#include <generic/dag_tab.h>
#include <generic/dag_span.h>


class CompositeDstBuffer
{
  struct RelativeIdx
  {
    explicit RelativeIdx(int value_) : value(value_) {}

    RelativeIdx operator+(RelativeIdx other) const { return RelativeIdx(value + other.getRel()); }
    RelativeIdx operator+(int other) const { return RelativeIdx(value + other); }
    RelativeIdx operator-(int other) const { return RelativeIdx(value - other); }

    int getRel() const { return value; }

    int operator<=>(RelativeIdx other) const { return value - other.value; }
    bool operator==(RelativeIdx other) const { return value == other.value; }
    bool operator!=(RelativeIdx other) const { return !(*this == other); }

  private:
    int value;
  };

  struct AbsoluteIdx
  {
    explicit AbsoluteIdx(int value_) : value(value_) {}

    void operator+(const AbsoluteIdx &other) const = delete;
    AbsoluteIdx operator+(RelativeIdx other) const { return AbsoluteIdx(value + other.getRel()); }
    AbsoluteIdx operator-(RelativeIdx other) const { return AbsoluteIdx(value - other.getRel()); }
    RelativeIdx operator-(AbsoluteIdx other) const { return RelativeIdx(value - other.getAbs()); }
    AbsoluteIdx operator+(int other) const { return AbsoluteIdx(value + other); }
    AbsoluteIdx operator-(int other) const { return AbsoluteIdx(value - other); }

    int getAbs() const { return value; }

    int operator<=>(AbsoluteIdx other) { return value - other.value; }
    bool operator==(AbsoluteIdx other) { return value == other.value; }
    bool operator!=(AbsoluteIdx other) { return !(*this == other); }

  private:
    int value;
  };

  struct Piece
  {
    AbsoluteIdx startIdx = AbsoluteIdx(0);
    int size = 0;
    bool initialized = false;
  };

public:
  CompositeDstBuffer(dag::Span<uint8_t> fixedBuf_, Tab<uint8_t> &resizeableBuf_, int firstSampleIdx_, int bytesPerSample_) :
    fixedBuf(fixedBuf_),
    resizeableBuf(resizeableBuf_),
    firstSampleIdx(firstSampleIdx_),
    bytesPerSample(bytesPerSample_),
    fixedBufSamples(fixedBuf_.size() / bytesPerSample_)
  {
    G_ASSERT(firstSampleIdx.getAbs() >= 0);
    G_ASSERT(bytesPerSample > 0);
    G_ASSERT(resizeableBuf.empty());
    pieces.emplace_back(Piece{.startIdx = firstSampleIdx, .size = int(fixedBuf_.size() / bytesPerSample)});
  }

  void assign(int startSampleIdx_, dag::Span<uint8_t> data)
  {
    AbsoluteIdx startSampleIdx(startSampleIdx_);
    RelativeIdx startRelativeToBuffers = startSampleIdx - firstSampleIdx;
    if (startRelativeToBuffers.getRel() < 0)
    {
      int samplesToSkip = -startRelativeToBuffers.getRel();
      if (samplesToSkip >= data.size() / bytesPerSample)
        return;
      int samplesLeft = data.size() / bytesPerSample - samplesToSkip;
      if (samplesLeft <= 0)
        return;
      startSampleIdx_ += samplesToSkip;
      data = data.subspan(samplesToSkip * bytesPerSample);
      startSampleIdx = AbsoluteIdx(startSampleIdx_);
      startRelativeToBuffers = startSampleIdx - firstSampleIdx;
      G_ASSERT(startRelativeToBuffers.getRel() == 0);
    }
    int samplesToWrite = data.size() / bytesPerSample;

    int requiredTotalSamples = startRelativeToBuffers.getRel() + samplesToWrite;
    if (requiredTotalSamples <= 0)
      return;
    int actualTotalSamples = size();
    if (requiredTotalSamples > actualTotalSamples)
      grow(requiredTotalSamples - actualTotalSamples);

    for (int i = 0; i < pieces.size(); i++)
    {
      Piece &piece = pieces[i];
      AbsoluteIdx lastSampleIdx = piece.startIdx + RelativeIdx(piece.size - 1);
      if (lastSampleIdx < startSampleIdx)
        continue;
      if (!piece.initialized && piece.startIdx != startSampleIdx)
      {
        split(i, startSampleIdx);
        continue;
      }
      RelativeIdx startOffsetIntoPiece = startSampleIdx - piece.startIdx;
      G_ASSERT(startOffsetIntoPiece.getRel() >= 0);
      G_ASSERT(startOffsetIntoPiece.getRel() < piece.size);
      RelativeIdx endOffsetIntoPiece = startOffsetIntoPiece + samplesToWrite;
      if (endOffsetIntoPiece.getRel() < piece.size && !piece.initialized)
      {
        split(i, piece.startIdx + endOffsetIntoPiece);
        i--;
        continue;
      }
      RelativeIdx actualEndOffsetIntoPiece = min(RelativeIdx(piece.size), endOffsetIntoPiece);
      int samplesToCopy = actualEndOffsetIntoPiece.getRel() - startOffsetIntoPiece.getRel();
      uint8_t *pieceData = getPieceData(piece);
      memcpy(pieceData + (startOffsetIntoPiece.getRel() * bytesPerSample), data.data(), samplesToCopy * bytesPerSample);
      piece.initialized = true;
      if (endOffsetIntoPiece == actualEndOffsetIntoPiece)
        break;
      G_ASSERT(endOffsetIntoPiece > actualEndOffsetIntoPiece);
      data = data.subspan(samplesToCopy * bytesPerSample);
      startSampleIdx = startSampleIdx + samplesToCopy;
      samplesToWrite -= samplesToCopy;
    }
  }

  bool finalize()
  {
    bool gotAnySamples = false;
    for (int i = 0; i < pieces.size(); i++)
    {
      if (pieces[i].initialized)
      {
        gotAnySamples = true;
        break;
      }
    }

    if (!gotAnySamples)
      return false;

    for (int i = 0; i < pieces.size(); i++)
    {
      Piece &piece = pieces[i];
      if (!piece.initialized)
      {
        uint8_t *pieceData = getPieceData(piece);
        memset(pieceData, 0, piece.size * bytesPerSample);
      }
    }

    return true;
  }

  void grow(int additionalSamples)
  {
    G_ASSERT(additionalSamples > 0);
    AbsoluteIdx newPieceSampleIdx = firstSampleIdx + size();
    resizeableBuf.resize(resizeableBuf.size() + additionalSamples * bytesPerSample);
    pieces.emplace_back(Piece{.startIdx = newPieceSampleIdx, .size = additionalSamples});
  }

  int size() const { return (fixedBuf.size() + resizeableBuf.size()) / bytesPerSample; }

private:
  void split(int pieceIndex, AbsoluteIdx startSampleIdx)
  {
    pieces.push_back();
    for (int i = pieces.size() - 1; i > pieceIndex + 1; i--)
      pieces[i] = pieces[i - 1];
    Piece &oldPiece = pieces[pieceIndex];
    Piece &newPiece = pieces[pieceIndex + 1];
    newPiece.size = oldPiece.size - (startSampleIdx - oldPiece.startIdx).getRel();
    G_ASSERT(newPiece.size > 0);
    oldPiece.size -= newPiece.size;
    G_ASSERT(oldPiece.size > 0);
    newPiece.startIdx = startSampleIdx;
  }

  uint8_t *getPieceData(const Piece &piece)
  {
    RelativeIdx relativeStartIdx = piece.startIdx - firstSampleIdx;
    int relativeStart = relativeStartIdx.getRel();
    if (relativeStart < fixedBufSamples)
      return fixedBuf.data() + relativeStart * bytesPerSample;
    return resizeableBuf.data() + (relativeStart - fixedBufSamples) * bytesPerSample;
  }

private:
  dag::Span<uint8_t> fixedBuf;
  Tab<uint8_t> &resizeableBuf;
  eastl::deque<Piece> pieces;
  AbsoluteIdx firstSampleIdx;
  int bytesPerSample;
  const int fixedBufSamples;
};

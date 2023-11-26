
macro INIT_ETC2_COMPRESSION()

  hlsl(cs)
  {
    RWTexture2D<uint4> compressedTexture : register( u2 );

    #define BLOCK_MODE_INDIVIDUAL                0
    #define BLOCK_MODE_DIFFERENTIAL              1

    static const float4 ALPHA_DISTANCE_TABLES[16] = {
            float4(2, 5, 8, 14),
            float4(2, 6, 9, 12),
            float4(1, 4, 7, 12),
            float4(1, 3, 5, 12),
            float4(2, 5, 7, 11),
            float4(2, 6, 8, 10),
            float4(3, 6, 7, 10),
            float4(2, 4, 7, 10),
            float4(1, 5, 7, 9),
            float4(1, 4, 7, 9),
            float4(1, 3, 7, 9),
            float4(1, 4, 6, 9),
            float4(2, 3, 6, 9),
            float4(0, 1, 2, 9),
            float4(3, 5, 7, 8),
            float4(2, 4, 6, 8) };

    static const float4 RGB_DISTANCE_TABLES[8] = {
            float4(-8, -2, 2, 8),
            float4(-17, -5, 5, 17),
            float4(-29, -9, 9, 29),
            float4(-42, -13, 13, 42),
            float4(-60, -18, 18, 60),
            float4(-80, -24, 24, 80),
            float4(-106, -33, 33, 106),
            float4(-183, -47, 47, 183) };

    float Luminance(float3 LinearColor)
    {
      return dot(LinearColor, float3(0.3, 0.59, 0.11 ));
    }

    int3 FloatColorToUint555(float3 FloatColor)
    {
      float3 Scale = float3(31.f, 31.f, 31.f);
      float3 ColorScaled = round(saturate(FloatColor) * Scale);
      return int3((int)ColorScaled.r, (int)ColorScaled.g, (int)ColorScaled.b);
    }

    float3 ExpandColor444(int3 Color444)
    {
      int3 Color888 = (Color444 << 4) + Color444;
      return Color888 / 255.f;
    }

    float3 ExpandColor555(int3 Color555)
    {
      int3 Color888 = (Color555 << 3) + (Color555 >> 2);
      return Color888 / 255.f;
    }

    uint SwapEndian32(uint x)
    {
      return ((x & 0x0000ff) << 24) | ((x & 0x00ff00) <<  8) | ((x & 0xff0000) >>  8) | (x >> 24);
    }

    uint FindPixelWeights(const float4 Block[16], float3 BaseColor, uint TableIdx, int StartX)
    {
      uint SubBlockWeights = 0;

      float TableRangeMax = RGB_DISTANCE_TABLES[TableIdx].w / 255.f / 1.58;
      float BaseLum = Luminance(BaseColor);

      #define DO_STEP(X, Y) \
        { \
          float3 OrigColor = Block[4 * (Y) + (X)].rgb; \
          float Diff = Luminance(OrigColor) - BaseLum; \
          int EncIndex = (abs(Diff) > TableRangeMax) + (Diff < 0.f ? 2 : 0); \
          int IndexInBlock = (X) * 4 + (Y); \
          SubBlockWeights |= ((EncIndex & 1) << IndexInBlock) | ((EncIndex >> 1) << (16 + IndexInBlock)); \
        }
      DO_STEP(StartX+0, 0)
      DO_STEP(StartX+1, 0)
      DO_STEP(StartX+0, 1)
      DO_STEP(StartX+1, 1)
      DO_STEP(StartX+0, 2)
      DO_STEP(StartX+1, 2)
      DO_STEP(StartX+0, 3)
      DO_STEP(StartX+1, 3)
        #undef DO_STEP

       return SubBlockWeights;
    }

    uint SelectRGBTableIndex(float LuminanceR)
    {
      float Range = (LuminanceR + LuminanceR * 0.1)*255.0;
      return Range < 8.0 ? 0 : (Range < 17.0 ? 1 : (Range < 29.0 ? 2 : (Range < 42.0 ? 3 : (Range < 60.0 ? 4 : (Range < 80.0 ? 5 : (Range < 106.0 ? 6 : 7))))));
    }

    uint2 CompressBlock_ETC2_RGB(const float4 Block[16])
    {
      uint FlipBit = 0;

      float3 BaseColor1_Float = (Block[0] + Block[1] + Block[4] + Block[5] +  Block[8] +  Block[9] + Block[12] + Block[13]).rgb * 0.125;
      float3 BaseColor2_Float = (Block[2] + Block[3] + Block[6] + Block[7] + Block[10] + Block[11] + Block[14] + Block[15]).rgb * 0.125;

      int3 BaseColor1 = FloatColorToUint555(BaseColor1_Float);
      int3 BaseColor2 = FloatColorToUint555(BaseColor2_Float);
      int3 Diff = BaseColor2 - BaseColor1;

      uint ColorBits;
      float3 BaseColor1_Quant, BaseColor2_Quant;

      uint BlockMode;
      int3 MinDiff = { -4, -4, -4 }, MaxDiff = { 3, 3, 3 };
      if (all(Diff >= MinDiff) && all(Diff <= MaxDiff))
      {
        BlockMode = BLOCK_MODE_DIFFERENTIAL;
        ColorBits = ((Diff.b & 7) << 16) | (BaseColor1.b << 19) | ((Diff.g & 7) << 8) | (BaseColor1.g << 11) | (Diff.r & 7) | (BaseColor1.r << 3);
        BaseColor1_Quant = ExpandColor555(BaseColor1);
        BaseColor2_Quant = ExpandColor555(BaseColor2);
      }
      else
      {
        BlockMode = BLOCK_MODE_INDIVIDUAL;
        BaseColor1 >>= 1;
        BaseColor2 >>= 1;
        ColorBits = (BaseColor1.b << 20) | (BaseColor2.b << 16) | (BaseColor1.g << 12) | (BaseColor2.g << 8) | (BaseColor1.r << 4) | BaseColor2.r;
        BaseColor1_Quant = ExpandColor444(BaseColor1);
        BaseColor2_Quant = ExpandColor444(BaseColor2);
      }

      float l00 = Luminance(Block[0].rgb);
      float l08 = Luminance(Block[8].rgb);
      float l13 = Luminance(Block[13].rgb);
      float LuminanceR1 = (max3(l00, l08, l13) - min3(l00, l08, l13))*0.5;
      uint SubBlock1TableIdx = SelectRGBTableIndex(LuminanceR1);
      uint SubBlock1Weights = FindPixelWeights(Block, BaseColor1_Quant, SubBlock1TableIdx, 0);

      float l02 = Luminance(Block[2].rgb);
      float l10 = Luminance(Block[10].rgb);
      float l15 = Luminance(Block[15].rgb);
      float LuminanceR2 =  (max3(l02, l10, l15) - min3(l02, l10, l15))*0.5;
      uint SubBlock2TableIdx = SelectRGBTableIndex(LuminanceR2);
      uint SubBlock2Weights = FindPixelWeights(Block, BaseColor2_Quant, SubBlock2TableIdx, 2);

      uint ModeBits = (SubBlock1TableIdx << 29) | (SubBlock2TableIdx << 26) | (BlockMode << 25) | (FlipBit << 24) | ColorBits;
      uint IndexBits = SwapEndian32(SubBlock1Weights | SubBlock2Weights);

      return uint2(ModeBits, IndexBits);
    }

    void SelectAlphaMod(in float SourceAlpha, in float EncodedAlpha, int IndexInTable, inout int SelectedIndex, inout float MinDif)
    {
      float Dif = abs(EncodedAlpha - SourceAlpha);
      if (Dif < MinDif)
      {
        MinDif = Dif;
        SelectedIndex = IndexInTable;
      }
    }

    uint2 CompressBlock_ETC2_Alpha(const float4 Block[16], int selector)
    {
      float MinAlpha = 1.f;
      float MaxAlpha = 0.f;

      #define DO_STEP(A) MinAlpha = min(A, MinAlpha); MaxAlpha = max(A, MaxAlpha);
      DO_STEP(Block[0][selector]);
      DO_STEP(Block[1][selector]);
      DO_STEP(Block[2][selector]);
      DO_STEP(Block[3][selector]);
      DO_STEP(Block[4][selector]);
      DO_STEP(Block[5][selector]);
      DO_STEP(Block[6][selector]);
      DO_STEP(Block[7][selector]);
      DO_STEP(Block[8][selector]);
      DO_STEP(Block[9][selector]);
      DO_STEP(Block[10][selector]);
      DO_STEP(Block[11][selector]);
      DO_STEP(Block[12][selector]);
      DO_STEP(Block[13][selector]);
      DO_STEP(Block[14][selector]);
      DO_STEP(Block[15][selector]);
      #undef DO_STEP

      MinAlpha = round(MinAlpha*255.f);
      MaxAlpha = round(MaxAlpha*255.f);

      float AlphaRange = MaxAlpha - MinAlpha;
      const float MidRange = 21.f;// an average range in ALPHA_DISTANCE_TABLES
      float Multiplier = clamp(round(AlphaRange/MidRange), 1.f, 15.f);

      int TableIdx = 0;
      float4 TableValueNeg = float4(0,0,0,0);
      float4 TableValuePos = float4(0,0,0,0);

      const int TablesToTest[5] = {15,11,6,2,0};
      for (int i = 0; i < 5; ++i)
      {
        TableIdx = TablesToTest[i];
        TableValuePos = ALPHA_DISTANCE_TABLES[TableIdx];

        float TableRange = (TableValuePos.w*2 + 1)*Multiplier;
        float Dif = TableRange - AlphaRange;
        if (Dif > 0.f)
          break;
      }
      TableValueNeg = -(TableValuePos + float4(1,1,1,1));

      TableValuePos*=Multiplier;
      TableValueNeg*=Multiplier;

      float BaseValue = clamp(round(-TableValueNeg.w + MinAlpha), 0.f, 255.f);

      TableValueNeg = TableValueNeg + BaseValue.xxxx;
      TableValuePos = TableValuePos + BaseValue.xxxx;
      uint2 BlockWeights = 0;

      #define DO_PIXEL(PixelIndex) \
      { \
        float Alpha = Block[PixelIndex][selector]*255.f; \
        int SelectedIndex = 0; \
        float MinDif = 100000.f; \
                         \
        float4 tableRef = Alpha < TableValuePos.x ? TableValueNeg : TableValuePos; \
        int baseIndex = Alpha < TableValuePos.x ? 0 : 4; \
             \
        SelectAlphaMod(Alpha, tableRef.x, baseIndex + 0, SelectedIndex, MinDif); \
        SelectAlphaMod(Alpha, tableRef.y, baseIndex + 1, SelectedIndex, MinDif); \
        SelectAlphaMod(Alpha, tableRef.z, baseIndex + 2, SelectedIndex, MinDif); \
        SelectAlphaMod(Alpha, tableRef.w, baseIndex + 3, SelectedIndex, MinDif); \
             \
        int TransposedIndex = (PixelIndex >> 2) | ((PixelIndex & 3) << 2); \
        int StartBit = (15 - TransposedIndex) * 3; \
        BlockWeights.x |= (StartBit < 32) ? SelectedIndex << StartBit : 0; \
        int ShiftRight = (StartBit == 30) ? 2 : 0; \
        int ShiftLeft = (StartBit >= 32) ? StartBit - 32 : 0; \
        BlockWeights.y |= (StartBit >= 30) ? (SelectedIndex >> ShiftRight) << ShiftLeft : 0; \
      }
      DO_PIXEL(0)
      DO_PIXEL(1)
      DO_PIXEL(2)
      DO_PIXEL(3)
      DO_PIXEL(4)
      DO_PIXEL(5)
      DO_PIXEL(6)
      DO_PIXEL(7)
      DO_PIXEL(8)
      DO_PIXEL(9)
      DO_PIXEL(10)
      DO_PIXEL(11)
      DO_PIXEL(12)
      DO_PIXEL(13)
      DO_PIXEL(14)
      DO_PIXEL(15)
      #undef DO_PIXEL

      int MultiplierInt = Multiplier;
      int BaseValueInt = BaseValue;

      uint2 AlphaBits;
      AlphaBits.x = SwapEndian32(BlockWeights.y | (TableIdx << 16) | (MultiplierInt << 20) | (BaseValueInt << 24));
      AlphaBits.y = SwapEndian32(BlockWeights.x);

      return AlphaBits;
    }

    uint4 CompressBlock_ETC2_RGBA(float4 Block[16])
    {
      uint2 CompressedRGB = CompressBlock_ETC2_RGB(Block);
      uint2 CompressedAlpha = CompressBlock_ETC2_Alpha(Block, 3);
      return uint4(CompressedAlpha, CompressedRGB);
    }

    uint4 CompressBlock_ETC2_RG(float4 Block[16])
    {
      uint2 R = CompressBlock_ETC2_Alpha(Block, 0);
      uint2 G = CompressBlock_ETC2_Alpha(Block, 1);
      return uint4(R, G);
    }
  }

endmacro


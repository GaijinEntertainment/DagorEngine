#include <memory>
#include <cassert>
#include <vector>
#include <thread>
#include <unordered_map>
#include <ppl.h>

const float EDGE_CONSTRAINTS_WEIGHT = 5.0f;
const float COVERED_PIXELS_WEIGHT = 1.0f;
const float NONCOVERED_PIXELS_WEIGHT = 0.1f;
const float TOLERANCE = 0.01f;

#define USE_ISPC 0

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define min3(x, y, z) min((x), min((y), (z)))
#define max3(x, y, z) max((x), max((y), (z)))

// Convenience for accessing 2D regular data (e.g. images)
template <typename T>
struct array2d
{
  std::unique_ptr<T[]> data;
  int width, height;
  array2d(int width, int height, T zero = T()) : data(new T[width * height]), width(width), height(height)
  {
    for (int i = 0; i < width * height; ++i)
    {
      data[i] = zero;
    }
  }

  array2d(std::unique_ptr<T[]> data, int width, int height) : data(data.release()), width(width), height(height) {}

  T &operator()(int column, int row) { return data[row * width + column]; }

  T operator()(int column, int row) const { return data[row * width + column]; }
};

// Dense vector of arbitrary length.
struct VectorX
{
  std::unique_ptr<float[]> vec;
  size_t size;

  VectorX(const VectorX &other) : vec(new float[other.size]), size(other.size)
  {
    memcpy(vec.get(), other.vec.get(), size * sizeof(float));
  }

  VectorX(size_t size) : vec(new float[size]), size(size) { memset(vec.get(), 0, sizeof(float) * size); }

  VectorX &operator=(const VectorX &other)
  {
    if (this != &other)
    {
      if (size != other.size)
      {
        vec.reset(new float[other.size]);
        size = other.size;
      }
      memcpy(vec.get(), other.vec.get(), size * sizeof(float));
    }
    return *this;
  }

  __forceinline float operator[](size_t ix) const { return vec[ix]; }
  __forceinline float &operator[](size_t ix) { return vec[ix]; }

  static void Sub(VectorX &out, const VectorX &x, const VectorX &y)
  {
    assert(out.size == x.size);
    assert(x.size == y.size);
    for (size_t i = 0; i < x.size; ++i)
      out.vec[i] = x.vec[i] - y.vec[i];
  }

  static void Add(VectorX &out, const VectorX &x, const VectorX &y)
  {
    assert(out.size == x.size);
    assert(x.size == y.size);
    for (size_t i = 0; i < x.size; ++i)
      out.vec[i] = x.vec[i] + y.vec[i];
  }

  static void Mul(VectorX &out, const VectorX &x, float s)
  {
    assert(out.size == x.size);
    for (size_t i = 0; i < x.size; ++i)
      out.vec[i] = x.vec[i] * s;
  }

  static float Dot(const VectorX &a, const VectorX &b)
  {
    assert(a.size == b.size);
    size_t n = a.size;

    if (n > 4000)
    {
      const int NumChunks = 4;
      int chunkSize = (int)n / NumChunks;

      float dotProducts[NumChunks];
      concurrency::parallel_for(0, NumChunks, [&](auto i) {
        int count = i != NumChunks - 1 ? chunkSize : (int)n - chunkSize * (NumChunks - 1); // Treat last chunk separately
        size_t start = i * chunkSize;
        float *aArray = a.vec.get() + start;
        float *bArray = b.vec.get() + start;
#if USE_ISPC
        dotProducts[i] = ispc::dot(aArray, bArray, count);
#else
				float sum = 0;
				for (int j = 0; j < count; ++j)
				{
					sum += aArray[j] * bArray[j];
				}
				dotProducts[i] = sum;
#endif
      });

      float finalSum = 0.0f;
      for (int i = 0; i < NumChunks; ++i)
        finalSum += dotProducts[i];

      return finalSum;
    }
    else
    {
      float sum = 0.0f;
      for (size_t i = 0; i < n; ++i)
      {
        sum += a[i] * b[i];
      }
      return sum;
    }
  }

  // Returns v*a + b
  static void MulAdd(VectorX &out, const VectorX &v, float a, const VectorX &b)
  {
    assert(out.size == v.size);
#if USE_ISPC
    ispc::vmadd(v.vec.get(), a, b.vec.get(), out.vec.get(), (int)v.size);
#else
    for (size_t i = 0; i < v.size; ++i)
    {
      out[i] = v[i] * a + b[i];
    }
#endif
  }
};

// Very basic sparse matrix.
struct SparseMat
{
  SparseMat(int numRows, int numCols) : rows(new Row[numRows]), numRows(numRows), numCols(numCols) {}
  float &operator()(size_t row, size_t column)
  {
    assert(row < numRows);
    assert(row >= 0);
    assert(column < numCols);
    assert(column >= 0);
    return rows[(int)row][(int)column];
  }

  static void Mul(VectorX &out, const SparseMat &A, const VectorX &x)
  {
    assert(out.size == A.numRows);
    assert(x.size == A.numCols);
    concurrency::parallel_for<size_t>(
      0, A.numRows, [&](auto r) { out[r] = Dot(x, A.rows[r]); }, concurrency::simple_partitioner(100));
  }

private:
  struct Row
  {
    template <typename T>
    struct AlignedDeleter
    {
      void operator()(T *ptr) { _aligned_free(ptr); }
    };

    size_t n = 0;
    int capacity = 0;
    std::unique_ptr<float[], AlignedDeleter<float>> coeffs;
    std::unique_ptr<int[], AlignedDeleter<int>> indices;

    float &operator[](int column)
    {
      // Binary search to find the element
      size_t index = findClosestIndex(column);
      if (n == 0 || indices[index] != column) // Add new element
      {
        if (n == capacity)
        {
          grow();
        }

        // Put the new element in the right place, and shift existing elements down by one.
        float prevCoeff = 0;
        int prevIndex = column;
        ++n;
        for (size_t i = index; i < n; ++i)
        {
          float tmpCoeff = coeffs[i];
          int tmpIndex = indices[i];
          coeffs[i] = prevCoeff;
          indices[i] = prevIndex;
          prevCoeff = tmpCoeff;
          prevIndex = tmpIndex;
        }
      }
      return coeffs[index];
    }

    void grow()
    {
      capacity = capacity == 0 ? 16 : capacity + capacity / 2;
      float *newCoeffs = (float *)_aligned_malloc(sizeof(float) * capacity, 32);
      int *newIndices = (int *)_aligned_malloc(sizeof(int) * capacity, 32);

      // Copy existing data over
      memcpy(newCoeffs, coeffs.get(), n * sizeof(float));
      memcpy(newIndices, indices.get(), n * sizeof(int));

      coeffs.reset(newCoeffs);
      indices.reset(newIndices);
    }

    size_t findClosestIndex(int columnIndex)
    {
      // binary search
      size_t count = n;
      size_t leftIndex = 0;
      while (count > 0)
      {
        size_t half = count / 2;
        size_t mid = leftIndex + half;
        size_t otherLeft = leftIndex + (count - half);

        leftIndex = indices[mid] < columnIndex ? otherLeft : leftIndex;
        count = half;
      }
      return leftIndex;
    }
  };

  std::unique_ptr<Row[]> rows;
  int numRows, numCols;

  static float Dot(const VectorX &x, const Row &row)
  {
    float sum = 0.0f;
    for (size_t i = 0; i < row.n; ++i)
    {
      sum += x[row.indices[i]] * row.coeffs[i];
    }
    return sum;
  }
};

struct Vec3
{
  float x, y, z;

  bool operator==(const Vec3 &other) const { return other.x == x && other.y == y && other.z == z; }

  bool operator!=(const Vec3 &other) const { return !(*this == other); }

  Vec3 operator-(const Vec3 &other) const { return Vec3{x - other.x, y - other.y, z - other.z}; }

  Vec3 operator+(const Vec3 &other) const { return Vec3{x + other.x, y + other.y, z + other.z}; }

  void operator+=(const Vec3 &other) { *this = *this + other; }

  Vec3 operator*(const float &s) const { return Vec3{x * s, y * s, z * s}; }

  Vec3 operator/(const float &s) const { return *this * (1.0f / s); }
};


struct Vec4
{
  float x, y, z, w;

  bool operator==(const Vec4 &other) const { return other.x == x && other.y == y && other.z == z && other.w == w; }

  bool operator!=(const Vec4 &other) const { return !(*this == other); }

  Vec4 operator-(const Vec4 &other) const { return Vec4{x - other.x, y - other.y, z - other.z, w - other.w}; }

  Vec4 operator+(const Vec4 &other) const { return Vec4{x + other.x, y + other.y, z + other.z, w + other.w}; }

  void operator+=(const Vec4 &other) { *this = *this + other; }

  Vec4 operator*(const float &s) const { return Vec4{x * s, y * s, z * s, w * s}; }

  Vec4 operator/(const float &s) const { return *this * (1.0f / s); }
};

struct Vec2
{
  float u, v;
  bool operator==(const Vec2 &other) const { return other.u == u && other.v == v; }
  bool operator!=(const Vec2 &other) const { return !(*this == other); }
  Vec2 operator-(const Vec2 &other) const { return Vec2{u - other.u, v - other.v}; }
  Vec2 operator+(const Vec2 &other) const { return Vec2{u + other.u, v + other.v}; }
  void operator+=(const Vec2 &other)
  {
    u += other.u;
    v += other.v;
  }
  Vec2 operator*(const float &s) const { return Vec2{u * s, v * s}; }

  Vec2 operator/(const float &s) const { return *this * (1.0f / s); }

  float Length() const { return sqrtf(u * u + v * v); }
};

Vec2 operator*(float s, const Vec2 &x) { return x * s; }

struct Face
{
  int v0, v1, v2, uv0, uv1, uv2;
};

struct HalfEdge
{
  Vec2 a, b;
};

Vec2 UVToScreen(Vec2 in, int W, int H)
{
  in.v = 1.0f - in.v;
  in.u *= W;
  in.v *= H;
  return in - Vec2{0.5f, 0.5f};
}

// Do a modulo operation with the assumption that it's usually a no-op
// Note: this guarantees positive return values
int WrapCoordinate(int x, int size)
{
  // Branch predictor will hopefully skip these two loops most of the time
  while (x < 0)
  {
    x += size;
  }
  while (x >= size)
  {
    x -= size;
  }
  return x;
}

struct SeamEdge
{
  // The two half edges representing this edge (each half edge is in a different UV chart)
  HalfEdge edges[2];

  int numSamples(int W, int H) const
  {
    Vec2 e0 = UVToScreen(edges[0].b - edges[0].a, W, H);
    Vec2 e1 = UVToScreen(edges[1].b - edges[1].a, W, H);
    float len = max(e0.Length(), e1.Length());
    return (int)(len * 3);
  }
};

struct Mesh
{
  std::vector<Face> faces;
  std::vector<Vec3> verts;
  std::vector<Vec2> uvs;
};

// TODO: Write mesh loader that isn't awful.
bool LoadMesh(const char *fname, Mesh &mesh)
{
  FILE *f;
  if (fopen_s(&f, fname, "r"))
  {
    return false;
  }

  char buf[4096];
  while (!feof(f))
  {
    float x, y, z;
    int tri0, uv0, tri1, uv1, tri2, uv2;
    if (nullptr == fgets(buf, _countof(buf), f))
    {
      break;
    }

    if (sscanf_s(buf, "v %f %f %f", &x, &y, &z) == 3)
    {
      mesh.verts.push_back(Vec3{x, y, z});
    }
    else if (sscanf_s(buf, "vt %f %f", &x, &y) == 2)
    {
      mesh.uvs.push_back(Vec2{x, y});
    }
    else if (sscanf_s(buf, "f %d/%d %d/%d %d/%d", &tri0, &uv0, &tri1, &uv1, &tri2, &uv2) == 6)
    {
      // TODO: Pretty sure OBJ has some weird "index from the back" thing too with negative indices.
      assert(tri0 > 0);
      assert(uv0 > 0);
      assert(tri1 > 0);
      assert(uv1 > 0);
      assert(tri2 > 0);
      assert(uv2 > 0);
      mesh.faces.push_back(Face{tri0 - 1, tri1 - 1, tri2 - 1, uv0 - 1, uv1 - 1, uv2 - 1});
    }
    else
    {
      // printf("INFO: Ignoring line: %s", buf);
    }
  }

  fclose(f);
  return true;
}

void FindSeamEdges(const Mesh &mesh, std::vector<SeamEdge> &seamEdges, int W, int H)
{
  using namespace std;
  struct tupleHasher // Hash pairs of vectors
  {
    hash<float> h;
    size_t operator()(const tuple<Vec3, Vec3> x) const
    {
      Vec3 a = std::get<0>(x), b = std::get<1>(x);
      return h(a.x) + 1733 * h(a.y) + 43 * h(a.z) + 3257 * h(b.x) + 1091 * h(b.y) + 557 * h(b.z);
    }
  };
  unordered_map<tuple<Vec3, Vec3>, tuple<Vec2, Vec2>, tupleHasher> edgeMap;

  for (const auto &f : mesh.faces)
  {
    // Need to loop through the edges of this face, so make a list of all the edges, including their UVs
    tuple<int, int, int, int> edges[] = {
      make_tuple(f.v0, f.v1, f.uv0, f.uv1), make_tuple(f.v1, f.v2, f.uv1, f.uv2), make_tuple(f.v2, f.v0, f.uv2, f.uv0)};

    for (const auto &e : edges)
    {
      // Grab the actual verts and UVs. The mesh may not have been "welded" properly, so we have to check
      // the actual coordinates rather than just indices.
      Vec3 v0 = mesh.verts[get<0>(e)], v1 = mesh.verts[get<1>(e)];
      Vec2 uv0 = mesh.uvs[get<2>(e)], uv1 = mesh.uvs[get<3>(e)];

      // Try to find the "opposite" edge (note: reversing the order of verts here)
      auto otherEdge = edgeMap.find(make_tuple(v1, v0));
      if (otherEdge == edgeMap.end())
      {
        // First time we're seeing this edge, so add it to the map
        edgeMap[make_tuple(v0, v1)] = make_tuple(uv0, uv1);
      }
      else
      {
        // This edge has already been added once, so we have enough information to see if it's a normal edge, or a "seam edge".
        Vec2 otheruv0 = get<0>(otherEdge->second), otheruv1 = get<1>(otherEdge->second);
        if (otheruv0 != uv1 || otheruv1 != uv0)
        {
          // UV don't match, so we have a seam
          SeamEdge s = SeamEdge{HalfEdge{uv0, uv1}, HalfEdge{otheruv1, otheruv0}};
          seamEdges.push_back(s);
        }
        edgeMap.erase(otherEdge); // No longer need this edge, remove it to keep storage low
      }
    }
  }
}

struct RGB
{
  unsigned char R, G, B, A;
  unsigned char &operator[](size_t ix) { return ((unsigned char *)this)[ix]; }
};

float clamp(float x, float lo, float hi) { return min(hi, max(lo, x)); }

__forceinline Vec4 RGBToYCoCg(RGB color)
{
  return Vec4{color.R * 0.25f + color.G * 0.5f + color.B * 0.25f, -color.R * 0.25f + color.G * 0.5f - color.B * 0.25f,
    color.R * 0.5f - color.B * 0.5f, (float)color.A};
}

__forceinline RGB YCoCgToRGB(Vec4 color)
{
  float r = clamp(color.x - color.y + color.z, 0, 255);
  float g = clamp(color.x + color.y, 0, 255);
  float b = clamp(color.x - color.y - color.z, 0, 255);
  float a = color.w;
  return RGB{(unsigned char)roundf(r), (unsigned char)roundf(g), (unsigned char)roundf(b), (unsigned char)roundf(a)};
}

bool isInside(int x, int y, Vec2 ea, Vec2 eb)
{
  return (float)x * (eb.v - ea.v) - (float)y * (eb.u - ea.u) - ea.u * eb.v + ea.v * eb.u >= 0;
}

void RasterizeFace(Vec2 uv0, Vec2 uv1, Vec2 uv2, array2d<uint8_t> &coverageBuf)
{
  uv0 = UVToScreen(uv0, coverageBuf.width, coverageBuf.height);
  uv1 = UVToScreen(uv1, coverageBuf.width, coverageBuf.height);
  uv2 = UVToScreen(uv2, coverageBuf.width, coverageBuf.height);

  // Axis aligned bounds of the triangle
  int minx = (int)min3(uv0.u, uv1.u, uv2.u);
  int maxx = (int)max3(uv0.u, uv1.u, uv2.u) + 1;
  int miny = (int)min3(uv0.v, uv1.v, uv2.v);
  int maxy = (int)max3(uv0.v, uv1.v, uv2.v) + 1;

  // The three edges we will test
  Vec2 e0a = uv0, e0b = uv1;
  Vec2 e1a = uv1, e1b = uv2;
  Vec2 e2a = uv2, e2b = uv0;

  // Now just loop over a screen aligned bounding box around the triangle, and test each pixel against all three edges
  for (int y = miny; y <= maxy; ++y)
  {
    for (int x = minx; x <= maxx; ++x)
    {
      if (isInside(x, y, e0a, e0b) & isInside(x, y, e1a, e1b) & isInside(x, y, e2a, e2b))
      {
        coverageBuf(WrapCoordinate(x, coverageBuf.width), WrapCoordinate(y, coverageBuf.height)) = 1;
      }
    }
  }
}

RGB DilatePixel(int centerx, int centery, const array2d<RGB> &image, const array2d<uint8_t> &coverageBuf)
{
  int numPixels = 0;
  Vec4 sum = Vec4{0, 0, 0, 0};
  for (int yix = centery - 1; yix <= centery + 1; ++yix)
  {
    for (int xix = centerx - 1; xix <= centerx + 1; ++xix)
    {
      int x = WrapCoordinate(xix, image.width);
      int y = WrapCoordinate(yix, image.height);
      if (coverageBuf(x, y))
      {
        ++numPixels;
        RGB c = image(x, y);
        sum += Vec4{(float)c.R, (float)c.G, (float)c.B, (float)c.A};
      }
    }
  }

  if (numPixels > 0)
  {
    sum = sum / (float)numPixels;
    sum.x = min(255.0f, roundf(sum.x));
    sum.y = min(255.0f, roundf(sum.y));
    sum.z = min(255.0f, roundf(sum.z));
    sum.w = min(255.0f, roundf(sum.w));
    return RGB{(unsigned char)sum.x, (unsigned char)sum.y, (unsigned char)sum.z, (unsigned char)sum.w};
  }
  else
  {
    return RGB{0, 0, 0, 0};
  }
}

// Given a fractional sample location, compute the four integer sample locations and their weights
void CalculateSamplesAndWeights(const array2d<int> &pixelMap, Vec2 &sample, int *__restrict outIxs, float *__restrict outWeights)
{
  int truncu = (int)sample.u;
  int truncv = (int)sample.v;

  const int xs[] = {truncu, truncu + 1, truncu + 1, truncu};
  const int ys[] = {truncv, truncv, truncv + 1, truncv + 1};
  for (int i = 0; i < 4; ++i)
  {
    int x = WrapCoordinate(xs[i], pixelMap.width);
    int y = WrapCoordinate(ys[i], pixelMap.height);
    outIxs[i] = pixelMap(x, y);
  }

  float fracX = sample.u - truncu;
  float fracY = sample.v - truncv;
  outWeights[0] = (1.0f - fracX) * (1.0f - fracY);
  outWeights[1] = fracX * (1.0f - fracY);
  outWeights[2] = fracX * fracY;
  outWeights[3] = (1.0f - fracX) * fracY;
  for (int i = 0; i < 4; ++i)
  {
    outWeights[i] *= EDGE_CONSTRAINTS_WEIGHT;
  }
}

__forceinline float rcp(float x)
{
  __m128 tmp = _mm_load_ss(&x);
  tmp = _mm_rcp_ss(tmp);
  _mm_store_ss(&x, tmp);
  return x;
}

void ConjugateGradientOptimize(VectorX &out, SparseMat &A, const VectorX &guess, const VectorX &b, int numIterations, float tolerance)
{
  size_t n = guess.size;
  VectorX p(n), r(n), Ap(n), tmp(n);
  VectorX &x = out;

  // r = b - A*x;
  SparseMat::Mul(tmp, A, x);
  VectorX::Sub(r, b, tmp);

  p = r;
  float rsq = VectorX::Dot(r, r);
  for (int i = 0; i < numIterations; ++i)
  {
    SparseMat::Mul(Ap, A, p);
    float alpha = rsq / VectorX::Dot(p, Ap);
    VectorX::MulAdd(x, p, alpha, x);   // x = x + alpha*p
    VectorX::MulAdd(r, Ap, -alpha, r); // r = r - alpha*Ap
    float rsqnew = VectorX::Dot(r, r);
    if (fabs(rsqnew - rsq) < tolerance * n)
      break;
    float beta = rsqnew / rsq;
    VectorX::MulAdd(p, p, beta, r); // p = r + beta*p
    rsq = rsqnew;
  }
}

struct PixelInfo
{
  int x, y;
  bool isCovered;
  Vec4 colorYCoCg;
};

void ComputePixelInfo(const std::vector<SeamEdge> &seamEdges, const array2d<uint8_t> &coverageBuf, const array2d<RGB> &image,
  std::vector<PixelInfo> &pixelInfo, array2d<int> &pixelToPixelInfoMap)
{
  // Find the pixels we will optimize for. Use a 2D map so that we can find a unique set of
  // pixels that we need to optimize for, and a quick way to find the index of it given an (x,y) position.
  int W = coverageBuf.width;
  int H = coverageBuf.height;
  for (const auto &s : seamEdges)
  {
    // TODO: this is overkill.. Could do conservative rasterization instead of brute force sampling
    // 3x per pixel and take the union of the 2x2 sampling neighborhoods.
    int numSamples = s.numSamples(W, H);
    for (const auto &e : s.edges)
    {
      Vec2 e0 = UVToScreen(e.a, W, H);
      Vec2 e1 = UVToScreen(e.b, W, H);
      Vec2 dt = (e1 - e0) / (float)numSamples;
      Vec2 samplePoint = e0;
      for (int i = 0; i < numSamples; ++i, samplePoint += dt)
      {
        // Go through the four bilinear sample taps
        int xs[] = {(int)samplePoint.u, xs[0] + 1, xs[0] + 1, xs[0]};
        int ys[] = {(int)samplePoint.v, ys[0], ys[0] + 1, ys[0] + 1};

        for (int tap = 0; tap < 4; ++tap)
        {
          int x = WrapCoordinate(xs[tap], W);
          int y = WrapCoordinate(ys[tap], H);

          // If we haven't already seen this pixel, make sure we take this pixel into account when optimizing
          if (pixelToPixelInfoMap(x, y) == -1)
          {
            bool isCovered = !!coverageBuf(x, y);
            Vec4 colorYCoCg;
            if (isCovered)
            {
              colorYCoCg = RGBToYCoCg(image(x, y));
            }
            else
            {
              // Do dilation...
              colorYCoCg = RGBToYCoCg(DilatePixel(x, y, image, coverageBuf));
            }

            pixelInfo.push_back(PixelInfo{x, y, isCovered, colorYCoCg});
            pixelToPixelInfoMap(x, y) = (int)pixelInfo.size() - 1;
          }
        }
      }
    }
  }
}

void SetupLeastSquares(std::vector<SeamEdge> &seamEdges, const array2d<int> &pixelToPixelInfoMap,
  const std::vector<PixelInfo> &pixelInfo, SparseMat &AtA, VectorX &AtbR, VectorX &AtbG, VectorX &AtbB, VectorX &AtbA,
  VectorX &initialGuessR, VectorX &initialGuessG, VectorX &initialGuessB, VectorX &initialGuessA)
{
  int W = pixelToPixelInfoMap.width;
  int H = pixelToPixelInfoMap.height;
  for (size_t seamIndex = 0; seamIndex < seamEdges.size(); ++seamIndex)
  {
    SeamEdge s = seamEdges[seamIndex];

    // Step through the samples of this edge, and compute sample locations for each side of the seam
    int numSamples = s.numSamples(W, H);

    Vec2 firstHalfEdgeStart = UVToScreen(s.edges[0].a, W, H);
    Vec2 firstHalfEdgeEnd = UVToScreen(s.edges[0].b, W, H);

    Vec2 secondHalfEdgeStart = UVToScreen(s.edges[1].a, W, H);
    Vec2 secondHalfEdgeEnd = UVToScreen(s.edges[1].b, W, H);

    Vec2 firstHalfEdgeStep = (firstHalfEdgeEnd - firstHalfEdgeStart) / (float)numSamples;
    Vec2 secondHalfEdgeStep = (secondHalfEdgeEnd - secondHalfEdgeStart) / (float)numSamples;

    Vec2 firstHalfEdgeSample = firstHalfEdgeStart;
    Vec2 secondHalfEdgeSample = secondHalfEdgeStart;
    for (int sampleIx = 0; sampleIx < numSamples;
         ++sampleIx, firstHalfEdgeSample += firstHalfEdgeStep, secondHalfEdgeSample += secondHalfEdgeStep)
    {
      // Sample locations for the two corresponding sets of sample points
      int firstHalfEdge[4], secondHalfEdge[4];
      float firstHalfEdgeWeights[4], secondHalfEdgeWeights[4];
      CalculateSamplesAndWeights(pixelToPixelInfoMap, firstHalfEdgeSample, firstHalfEdge, firstHalfEdgeWeights);
      CalculateSamplesAndWeights(pixelToPixelInfoMap, secondHalfEdgeSample, secondHalfEdge, secondHalfEdgeWeights);

      /*
      Now, compute the covariance for the difference of these two vectors.
      If a is the first vector (first half edge) and b is the second, then we compute the covariance, without
      intermediate storage, like so:
      (a-b)*(a-b)^t = a*a^t + b*b^t - a*b^t-b*a^t
      */
      for (int i = 0; i < 4; ++i)
      {
        for (int j = 0; j < 4; ++j)
        {
          // + a*a^t
          AtA(firstHalfEdge[i], firstHalfEdge[j]) += firstHalfEdgeWeights[i] * firstHalfEdgeWeights[j];
          // + b*b^t
          AtA(secondHalfEdge[i], secondHalfEdge[j]) += secondHalfEdgeWeights[i] * secondHalfEdgeWeights[j];

          // - a*b^t
          AtA(firstHalfEdge[i], secondHalfEdge[j]) -= firstHalfEdgeWeights[i] * secondHalfEdgeWeights[j];

          // - b*a^t
          AtA(secondHalfEdge[i], firstHalfEdge[j]) -= secondHalfEdgeWeights[i] * firstHalfEdgeWeights[j];
        }
      }
    }
  }

  for (size_t i = 0; i < pixelInfo.size(); ++i)
  {
    PixelInfo pi = pixelInfo[i];

    // Set up equality cost, trying to keep the pixel at its original value
    // Note: for non-covered pixels the weight is much lower, since those are the pixels
    // we primarily want to modify (we'll want to keep it >0 though, to reduce the risk
    // of extreme values that can't fit in 8 bit color channels)
    float weight = pi.isCovered ? COVERED_PIXELS_WEIGHT : NONCOVERED_PIXELS_WEIGHT;
    AtA(i, i) += weight;

    // Set up the three right hand sides (one for R, G, and B).
    // Note AtRHS represents the transpose of the system matrix A multiplied by the RHS
    AtbR[i] += pi.colorYCoCg.x * weight;
    AtbG[i] += pi.colorYCoCg.y * weight;
    AtbB[i] += pi.colorYCoCg.z * weight;
    AtbA[i] += pi.colorYCoCg.w * weight;

    // Set up the initial guess for the solution.
    initialGuessR[i] = pi.colorYCoCg.x;
    initialGuessG[i] = pi.colorYCoCg.y;
    initialGuessB[i] = pi.colorYCoCg.z;
    initialGuessA[i] = pi.colorYCoCg.w;
  }
}

#include <image/dag_texPixel.h>
#include <image/dag_loadImage.h>
#include <image/dag_tga.h>

#if _TARGET_PC_WIN
#include <startup/dag_mainCon.inc.cpp>
#endif
void DagorWinMainInit(bool /*debugmode*/) {}
int DagorWinMain(bool /*debugmode*/)
{
  static char *const *argv = dgs_argv;
  static const int argc = dgs_argc;
  auto t0 = std::chrono::high_resolution_clock::now();

  if (argc != 4)
  {
    printf("Usage: texSeamRemover-dev.exe InputMesh.Obj InputTexture.tga OutputTexture.tga\n");
    return -1;
  }
  const char *meshFileName = argv[1];
  const char *texFileName = argv[2];
  const char *texOutFileName = argv[3];
  Mesh mesh;

  if (!LoadMesh(meshFileName, mesh))
  {
    printf("Error loading OBJ file!\n");
    return 1;
  }
  int W, H, comp;

  TexImage32 *loaded_image = load_image(texFileName, tmpmem);
  W = loaded_image->w;
  H = loaded_image->h;
  RGB *rawImg = (RGB *)(loaded_image + 1);
  // RGB* rawImg = (RGB*)stbi_load(texFileName, &W, &H, &comp, 3);
  if (rawImg == nullptr)
  {
    printf("Failed to load input texture!\n");
    return -1;
  }
  array2d<RGB> image(std::unique_ptr<RGB[]>(rawImg), W, H);


  // Find all edges that have different UVs on the two sides
  std::vector<SeamEdge> seamEdges;
  FindSeamEdges(mesh, seamEdges, W, H);

  // Produce a mask for all valid pixels
  array2d<uint8_t> coverageBuf(W, H);
  for (const auto &f : mesh.faces)
  {
    Vec2 uv0 = mesh.uvs[f.uv0], uv1 = mesh.uvs[f.uv1], uv2 = mesh.uvs[f.uv2];
    RasterizeFace(uv0, uv1, uv2, coverageBuf);
  }

#if 0
	// Set pixels that aren't covered to an obvious color, so we can see errors
	for (int j = 0; j < H; ++j)
	{
		for (int i = 0; i < W; ++i)
		{
			if (!coverageBuf[j*W + i])
			{
				outImage[j*W + i] = RGB{ 255,0,255 };
			}
		}
	}
#endif

  array2d<int> pixelToPixelInfoMap(W, H, -1);
  std::vector<PixelInfo> pixelInfo;
  ComputePixelInfo(seamEdges, coverageBuf, image, pixelInfo, pixelToPixelInfoMap);
  int numPixelsToOptimize = (int)pixelInfo.size();

  // Build up all the matrices and vectors we need to solve the problem
  SparseMat AtA(numPixelsToOptimize, numPixelsToOptimize);
  VectorX AtbR(numPixelsToOptimize), AtbG(numPixelsToOptimize), AtbB(numPixelsToOptimize), AtbA(numPixelsToOptimize);
  VectorX initialGuessR(numPixelsToOptimize), initialGuessG(numPixelsToOptimize), initialGuessB(numPixelsToOptimize),
    initialGuessA(numPixelsToOptimize);
  SetupLeastSquares(seamEdges, pixelToPixelInfoMap, pixelInfo, AtA, AtbR, AtbG, AtbB, AtbA, initialGuessR, initialGuessG,
    initialGuessB, initialGuessA);

  // Run conjugate gradient optimization one color channel at a time
  // (this is just so I don't have to implement sparse Matrix/Matrix multiplication efficiently :-))
  VectorX solutionR(numPixelsToOptimize), solutionG(numPixelsToOptimize), solutionB(numPixelsToOptimize),
    solutionA(numPixelsToOptimize);

  concurrency::parallel_invoke([&] { ConjugateGradientOptimize(solutionR, AtA, initialGuessR, AtbR, 10000, TOLERANCE); },
    [&] { ConjugateGradientOptimize(solutionG, AtA, initialGuessG, AtbG, 10000, TOLERANCE); },
    [&] { ConjugateGradientOptimize(solutionB, AtA, initialGuessB, AtbB, 10000, TOLERANCE); },
    [&] { ConjugateGradientOptimize(solutionB, AtA, initialGuessA, AtbA, 10000, TOLERANCE); });

  // Store the resuling optimized pixels and save out
  for (int i = 0; i < (int)numPixelsToOptimize; ++i)
  {
    PixelInfo pi = pixelInfo[i];
    Vec4 colorYCoCg = Vec4{solutionR[i], solutionG[i], solutionB[i], solutionA[i]};
    image(pi.x, pi.y) = YCoCgToRGB(colorYCoCg);
  }

  if (!save_tga32(texOutFileName, (TexPixel32 *)image.data.get(), W, H, sizeof(RGB) * W))
  // if (!stbi_write_png(texOutFileName, W, H, 3, image.data.get(), sizeof(RGB)*W))
  {
    printf("Failed to write output file!\n");
    return -1;
  }

  // Print out benchmarking times
  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> elapsedSeconds = t1 - t0;
  printf("Time: %.4f\n", elapsedSeconds.count());
  return 0;
}

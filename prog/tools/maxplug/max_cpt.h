#include <max.h>
#include <assert.h>


#if defined(MAX_RELEASE_R9) && MAX_RELEASE >= MAX_RELEASE_R9
#define NoRemap              DefaultRemapDir
#define MakeRefByID(x, y, z) ReplaceReference(y, z)

#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
#define STR_DEST(s) s.dataForWrite()
#else
#define STR_DEST(s) s
#endif

#else
#define STR_DEST(s) s
#define MCHAR       TCHAR
#endif


#if !defined(M_STD_OSTRINGSTREAM)
#ifdef _UNICODE
#define M_STD_OSTRINGSTREAM std::wostringstream
#define M_STD_ISTRINGSTREAM std::wistringstream
#else
#define M_STD_OSTRINGSTREAM std::ostringstream
#define M_STD_ISTRINGSTREAM std::istringstream
#endif
#endif


#if !defined(M_STD_ISTRINGSTREAM)
#ifdef _UNICODE
#define M_STD_ISTRINGSTREAM std::wistringstream
#else
#define M_STD_ISTRINGSTREAM std::istringstream
#endif
#endif

#if !defined(M_STD_STRING)
#ifdef _UNICODE
#define M_STD_STRING std::wstring
#else
#define M_STD_STRING std::string
#endif
#endif


#ifdef NDEBUG
#define verify(x) x
#else
#define verify(x) assert(x)
#endif

#if defined(MAX_RELEASE_R26) && MAX_RELEASE >= MAX_RELEASE_R26
inline BitArray &mesh_face_sel(Mesh &m) { return m.FaceSel(); }
inline BitArray &mesh_vert_sel(Mesh &m) { return m.VertSel(); }
#else
inline BitArray &mesh_face_sel(Mesh &m) { return m.faceSel; }
inline BitArray &mesh_vert_sel(Mesh &m) { return m.vertSel; }
#endif

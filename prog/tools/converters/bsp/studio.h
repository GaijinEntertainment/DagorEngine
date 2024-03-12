//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef STUDIO_H
#define STUDIO_H

#ifdef _WIN32
#pragma once
#endif
/*
#include "baseTypes.h"
#include "vector2d.h"
#include "vector.h"
#include "vector4d.h"
#include "compressed_vector.h"
#include "tier0/dbg.h"
#include "mathLib.h"
#include "utlvector.h"
*/

#ifndef offsetof
#define offsetof(s, m) (size_t) & (((s *)0)->m)
#endif


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class IMaterial;
class IMesh;
struct virtualmodel_t;

namespace OptimizedModel
{
struct StripHeader_t;
}


/*
==============================================================================

STUDIO MODELS

Studio models are position independent, so the cache manager can move them.
==============================================================================
*/

#define STUDIO_VERSION 44

#define MAXSTUDIOTRIANGLES  25000 // TODO: tune this
#define MAXSTUDIOVERTS      25000 // TODO: tune this
#define MAXSTUDIOSKINS      32    // total textures
#define MAXSTUDIOBONES      128   // total bones actually used
#define MAXSTUDIOBLENDS     32
#define MAXSTUDIOFLEXDESC   128
#define MAXSTUDIOFLEXCTRL   64
#define MAXSTUDIOPOSEPARAM  24
#define MAXSTUDIOBONECTRLS  4
#define MAXSTUDIOANIMBLOCKS 256

#define MAXSTUDIOBONEBITS 7 // NOTE: MUST MATCH MAXSTUDIOBONES

// NOTE!!! : Changing this number also changes the vtx file format!!!!!
#define MAX_NUM_BONES_PER_VERT 3

// Adrian - Remove this when we completely phase out the old event system.
#define NEW_EVENT_STYLE (1 << 10)

struct mstudiodata_t
{
  int count;
  int offset;
};

#define STUDIO_PROC_AXISINTERP 1
#define STUDIO_PROC_QUATINTERP 2

struct mstudioaxisinterpbone_t
{
  int control;        // local transformation of this bone used to calc 3 point blend
  int axis;           // axis to check
  Vector pos[6];      // X+, X-, Y+, Y-, Z+, Z-
  Quaternion quat[6]; // X+, X-, Y+, Y-, Z+, Z-

private:
  // No copy constructors allowed
  mstudioaxisinterpbone_t(const mstudioaxisinterpbone_t &vOther);
};


struct mstudioquatinterpinfo_t
{
  float inv_tolerance; // 1 / radian angle of trigger influence
  Quaternion trigger;  // angle to match
  Vector pos;          // new position
  Quaternion quat;     // new angle

private:
  // No copy constructors allowed
  mstudioquatinterpinfo_t(const mstudioquatinterpinfo_t &vOther);
};

struct mstudioquatinterpbone_t
{
  int control; // local transformation to check
  int numtriggers;
  int triggerindex;
  inline mstudioquatinterpinfo_t *pTrigger(int i) const { return (mstudioquatinterpinfo_t *)(((byte *)this) + triggerindex) + i; };

private:
  // No copy constructors allowed
  mstudioquatinterpbone_t(const mstudioquatinterpbone_t &vOther);
};

// bones
struct mstudiobone_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int parent;            // parent bone
  int bonecontroller[6]; // bone controller index, -1 == none

  // default values
  Vector pos;
  Quaternion quat;
  RadianEuler rot;
  // compression scale
  Vector posscale;
  Vector rotscale;

  matrix3x4_t poseToBone;
  Quaternion qAlignment;
  int flags;
  int proctype;
  int procindex;           // procedural rule
  mutable int physicsbone; // index into physically simulated bone
  inline void *pProcedure() const
  {
    if (procindex == 0)
      return NULL;
    else
      return (void *)(((byte *)this) + procindex);
  };
  int surfacepropidx; // index into string tablefor property name
  inline char *const pszSurfaceProp(void) const { return ((char *)this) + surfacepropidx; }
  int contents; // See BSPFlags.h for the contents flags

  int unused[8]; // remove as appropriate

private:
  // No copy constructors allowed
  mstudiobone_t(const mstudiobone_t &vOther);
};

#define BONE_CALCULATE_MASK        0x1F
#define BONE_PHYSICALLY_SIMULATED  0x01 // bone is physically simulated when physics are active
#define BONE_PHYSICS_PROCEDURAL    0x02 // procedural when physics is active
#define BONE_ALWAYS_PROCEDURAL     0x04 // bone is always procedurally animated
#define BONE_SCREEN_ALIGN_SPHERE   0x08 // bone aligns to the screen, not constrained in motion.
#define BONE_SCREEN_ALIGN_CYLINDER 0x10 // bone aligns to the screen, constrained by it's own axis.

#define BONE_USED_MASK           0x0007FF00
#define BONE_USED_BY_ANYTHING    0x0007FF00
#define BONE_USED_BY_HITBOX      0x00000100 // bone (or child) is used by a hit box
#define BONE_USED_BY_ATTACHMENT  0x00000200 // bone (or child) is used by an attachment point
#define BONE_USED_BY_VERTEX_MASK 0x0003FC00
#define BONE_USED_BY_VERTEX_LOD0 0x00000400 // bone (or child) is used by the toplevel model via skinned vertex
#define BONE_USED_BY_VERTEX_LOD1 0x00000800
#define BONE_USED_BY_VERTEX_LOD2 0x00001000
#define BONE_USED_BY_VERTEX_LOD3 0x00002000
#define BONE_USED_BY_VERTEX_LOD4 0x00004000
#define BONE_USED_BY_VERTEX_LOD5 0x00008000
#define BONE_USED_BY_VERTEX_LOD6 0x00010000
#define BONE_USED_BY_VERTEX_LOD7 0x00020000
#define BONE_USED_BY_BONE_MERGE  0x00040000 // bone is available for bone merge to occur against it

#define BONE_USED_BY_VERTEX_AT_LOD(lod)   (BONE_USED_BY_VERTEX_LOD0 << (lod))
#define BONE_USED_BY_ANYTHING_AT_LOD(lod) ((BONE_USED_BY_ANYTHING & ~BONE_USED_BY_VERTEX_MASK) | BONE_USED_BY_VERTEX_AT_LOD(lod))

#define MAX_NUM_LODS 8

#define BONE_TYPE_MASK       0x00F00000
#define BONE_FIXED_ALIGNMENT 0x00100000 // bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation

#define BONE_USED_BY_VERTEX_VERSION_32      0x0400 // bone (or child) is used by skinned vertex
#define BONE_USED_BY_VERTEX_LOD2_VERSION_32 0x0800 // ???? There will be N of these, maybe the LOD info returns the mask??
#define BONE_USED_BY_VERTEX_LOD3_VERSION_32 0x1000 // FIXME: these are currently unassigned....
#define BONE_USED_BY_VERTEX_LOD4_VERSION_32 0x2000
#define BONE_FIXED_ALIGNMENT_VERSION_32 \
  0x10000 // bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation


// bone controllers
struct mstudiobonecontroller_t
{
  int bone; // -1 == 0
  int type; // X, Y, Z, XR, YR, ZR, M
  float start;
  float end;
  int rest;       // byte index value at rest
  int inputfield; // 0-3 user set controller, 4 mouth
  int unused[8];
};

// intersection boxes
struct mstudiobbox_t
{
  int bone;
  int group;    // intersection group
  Vector bbmin; // bounding box
  Vector bbmax;
  int szhitboxnameindex; // offset to the name of the hitbox.
  int unused[8];

  char *pszHitboxName(void *pHeader)
  {
    if (szhitboxnameindex == 0)
      return "";

// NJS: Just a cosmetic change, next time the model format is rebuilt, please use the NEXT_MODEL_FORMAT_REVISION.
// also, do a grep to find the corresponding #ifdefs.
#ifdef NEXT_MODEL_FORMAT_REVISION
    return ((char *)this) + szhitboxnameindex;
#else
    return ((char *)pHeader) + szhitboxnameindex;
#endif
  }

  mstudiobbox_t() {}

private:
  // No copy constructors allowed
  mstudiobbox_t(const mstudiobbox_t &vOther);
};

// demand loaded sequence groups
struct mstudiomodelgroup_t
{
  int szlabelindex; // textual name
  inline char *const pszLabel(void) const { return ((char *)this) + szlabelindex; }
  int sznameindex; // file name
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
};

struct mstudiomodelgrouplookup_t
{
  int modelgroup;
  int indexwithingroup;
};

// events
struct mstudioevent_t
{
  float cycle;
  int event;
  int type;
  char options[64];

  int szeventindex;
  inline char *const pszEventName(void) const { return ((char *)this) + szeventindex; }
};

#define ATTACHMENT_FLAG_WORLD_ALIGN 0x10000

// attachment
struct mstudioattachment_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  unsigned int flags;
  int localbone;
  matrix3x4_t local; // attachment point
  int unused[8];
};

#define IK_SELF       1
#define IK_WORLD      2
#define IK_GROUND     3
#define IK_RELEASE    4
#define IK_ATTACHMENT 5
#define IK_UNLATCH    6

struct mstudioikerror_t
{
  Vector pos;
  Quaternion q;

  mstudioikerror_t() {}

private:
  // No copy constructors allowed
  mstudioikerror_t(const mstudioikerror_t &vOther);
};

union mstudioanimvalue_t;

struct mstudiocompressedikerror_t
{
  float scale[6];
  short offset[6];
  inline mstudioanimvalue_t *pAnimvalue(int i) const
  {
    if (offset[i] > 0)
      return (mstudioanimvalue_t *)(((byte *)this) + offset[i]);
    else
      return NULL;
  };

private:
  // No copy constructors allowed
  mstudiocompressedikerror_t(const mstudiocompressedikerror_t &vOther);
};

struct mstudioikrule_t
{
  int index;

  int type;
  int chain;

  int bone;

  int slot; // iktarget slot.  Usually same as chain.
  float height;
  float radius;
  float floor;
  Vector pos;
  Quaternion q;

  int compressedikerrorindex;
  inline mstudiocompressedikerror_t *pCompressedError() const
  {
    return (mstudiocompressedikerror_t *)(((byte *)this) + compressedikerrorindex);
  };
  int unused2;

  int iStart;
  int ikerrorindex;
  inline mstudioikerror_t *pError(int i) const
  {
    return (ikerrorindex) ? (mstudioikerror_t *)(((byte *)this) + ikerrorindex) + (i - iStart) : NULL;
  };

  float start; // beginning of influence
  float peak;  // start of full influence
  float tail;  // end of full influence
  float end;   // end of all influence

  int unused3;
  float contact; // frame footstep makes ground concact
  int unused4;
  int unused5;

  int unused6;
  int unused7;
  int unused8;

  int szattachmentindex; // name of world attachment
  inline char *const pszAttachment(void) const { return ((char *)this) + szattachmentindex; }

  int unused[7];

  mstudioikrule_t() {}

private:
  // No copy constructors allowed
  mstudioikrule_t(const mstudioikrule_t &vOther);
};


struct mstudioiklock_t
{
  int chain;
  float flPosWeight;
  float flLocalQWeight;
  int flags;

  int unused[4];
};

// animation frames
union mstudioanimvalue_t
{
  struct
  {
    byte valid;
    byte total;
  } num;
  short value;
};

struct mstudioanim_valueptr_t
{
  short offset[3];
  inline mstudioanimvalue_t *pAnimvalue(int i) const
  {
    if (offset[i] > 0)
      return (mstudioanimvalue_t *)(((byte *)this) + offset[i]);
    else
      return NULL;
  };
};

#define STUDIO_ANIM_RAWPOS  0x01
#define STUDIO_ANIM_RAWROT  0x02
#define STUDIO_ANIM_ANIMPOS 0x04
#define STUDIO_ANIM_ANIMROT 0x08
#define STUDIO_ANIM_DELTA   0x10

// per bone per animation DOF and weight pointers
struct mstudioanim_t
{
  // float                        weight;         // bone influence
  byte bone;
  byte flags; // weighing options

  // valid for animating data only
  inline byte *pData(void) const { return (((byte *)this) + sizeof(struct mstudioanim_t)); };
  inline mstudioanim_valueptr_t *pRotV(void) const { return (mstudioanim_valueptr_t *)(pData()); };
  inline mstudioanim_valueptr_t *pPosV(void) const
  {
    return (mstudioanim_valueptr_t *)(pData()) + ((flags & STUDIO_ANIM_ANIMROT) != 0);
  };

  // valid if animation unvaring over timeline
  inline Quaternion48 *pQuat(void) const { return (Quaternion48 *)(pData()); };
  inline Vector48 *pPos(void) const { return (Vector48 *)(pData() + ((flags & STUDIO_ANIM_RAWROT) != 0) * sizeof(*pQuat())); };

  short nextoffset;
  inline mstudioanim_t *pNext(void) const
  {
    if (nextoffset != 0)
      return (mstudioanim_t *)(((byte *)this) + nextoffset);
    else
      return NULL;
  };
};

struct mstudiomovement_t
{
  int endframe;
  int motionflags;
  float v0;        // velocity at start of block
  float v1;        // velocity at end of block
  float angle;     // YAW rotation at end of this blocks movement
  Vector vector;   // movement vector relative to this blocks initial angle
  Vector position; // relative to start of animation???

private:
  // No copy constructors allowed
  mstudiomovement_t(const mstudiomovement_t &vOther);
};

struct studiohdr_t;

// used for piecewise loading of animation data
struct mstudioanimblock_t
{
  int datastart;
  int dataend;
};

struct mstudioanimdesc_t
{
  int baseptr;
  inline studiohdr_t *pStudiohdr(void) const { return (studiohdr_t *)(((byte *)this) + baseptr); }

  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }

  float fps; // frames per second
  int flags; // looping/non-looping flags

  int numframes;

  // piecewise movement
  int nummovements;
  int movementindex;
  inline mstudiomovement_t *const pMovement(int i) const { return (mstudiomovement_t *)(((byte *)this) + movementindex) + i; };

  Vector bbmin; // per animation bounding box
  Vector bbmax;

  int animblock;
  int animindex;
  mstudioanim_t *pAnim(void) const;

  int numikrules;
  int ikruleindex;
  inline mstudioikrule_t *pIKRule(int i) const { return (mstudioikrule_t *)(((byte *)this) + ikruleindex) + i; };

  int unused[8]; // remove as appropriate

private:
  // No copy constructors allowed
  mstudioanimdesc_t(const mstudioanimdesc_t &vOther);
};

struct mstudioikrule_t;

struct mstudioautolayer_t
{
  // private:
  int iSequence;
  // public:
  int flags;
  float start; // beginning of influence
  float peak;  // start of full influence
  float tail;  // end of full influence
  float end;   // end of all influence
};

// sequence descriptions
struct mstudioseqdesc_t
{
  int baseptr;
  inline studiohdr_t *pStudiohdr(void) const { return (studiohdr_t *)(((byte *)this) + baseptr); }

  int szlabelindex;
  inline char *const pszLabel(void) const { return ((char *)this) + szlabelindex; }

  int szactivitynameindex;
  inline char *const pszActivityName(void) const { return ((char *)this) + szactivitynameindex; }

  int flags; // looping/non-looping flags

  int activity; // initialized at loadtime to game DLL values
  int actweight;

  int numevents;
  int eventindex;
  inline mstudioevent_t *pEvent(int i) const
  {
    Assert(i >= 0 && i < numevents);
    return (mstudioevent_t *)(((byte *)this) + eventindex) + i;
  };

  Vector bbmin; // per sequence bounding box
  Vector bbmax;

  int numblends;

  // Index into array of shorts which is groupsize[0] x groupsize[1] in length
  int animindexindex;

  inline int anim(int x, int y) const
  {
    if (x >= groupsize[0])
    {
      x = groupsize[0] - 1;
    }

    if (y >= groupsize[1])
    {
      y = groupsize[1] - 1;
    }

    int offset = y * groupsize[0] + x;
    short *blends = (short *)(((byte *)this) + animindexindex);
    int value = (int)blends[offset];
    return value;
  }

  int movementindex; // [blend] float array for blended movement
  int groupsize[2];
  int paramindex[2];   // X, Y, Z, XR, YR, ZR
  float paramstart[2]; // local (0..1) starting value
  float paramend[2];   // local (0..1) ending value
  int paramparent;

  float fadeintime;  // ideal cross fate in time (0.2 default)
  float fadeouttime; // ideal cross fade out time (0.2 default)

  int localentrynode; // transition node at entry
  int localexitnode;  // transition node at exit
  int nodeflags;      // transition rules

  float entryphase; // used to match entry gait
  float exitphase;  // used to match exit gait

  float lastframe; // frame that should generation EndOfSequence

  int nextseq; // auto advancing sequences
  int pose;    // index of delta animation between end and nextseq

  int numikrules;

  int numautolayers; //
  int autolayerindex;
  inline mstudioautolayer_t *pAutolayer(int i) const
  {
    Assert(i >= 0 && i < numautolayers);
    return (mstudioautolayer_t *)(((byte *)this) + autolayerindex) + i;
  };

  int weightlistindex;
  inline float *pBoneweight(int i) const { return ((float *)(((byte *)this) + weightlistindex) + i); };
  inline float weight(int i) const { return *(pBoneweight(i)); };

  // FIXME: make this 2D instead of 2x1D arrays
  int posekeyindex;
  float *pPoseKey(int iParam, int iAnim) const { return (float *)(((byte *)this) + posekeyindex) + iParam * groupsize[0] + iAnim; }
  float poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }

  int numiklocks;
  int iklockindex;
  inline mstudioiklock_t *pIKLock(int i) const
  {
    Assert(i >= 0 && i < numiklocks);
    return (mstudioiklock_t *)(((byte *)this) + iklockindex) + i;
  };

  // Key values
  int keyvalueindex;
  int keyvaluesize;
  inline const char *KeyValueText(void) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

  int unused[8]; // remove/add as appropriate (grow back to 8 ints on version change!)

private:
  // No copy constructors allowed
  mstudioseqdesc_t(const mstudioseqdesc_t &vOther);
};


struct mstudioposeparamdesc_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int flags;   // ????
  float start; // starting value
  float end;   // ending value
  float loop;  // looping range, 0 for no looping, 360 for rotations, etc.
};

struct mstudioflexdesc_t
{
  int szFACSindex;
  inline char *const pszFACS(void) const { return ((char *)this) + szFACSindex; }
};


struct mstudioflexcontroller_t
{
  int sztypeindex;
  inline char *const pszType(void) const { return ((char *)this) + sztypeindex; }
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  mutable int link; // remapped at load time to master list
  float min;
  float max;
};

struct mstudiovertanim_t
{
  short index;
  byte speed; // 255/max_length_in_flex
  byte side;  // 255/left_right
  Vector48 delta;
  Vector48 ndelta;

private:
  // No copy constructors allowed
  mstudiovertanim_t(const mstudiovertanim_t &vOther);
};


struct mstudioflex_t
{
  int flexdesc; // input value

  float target0; // zero
  float target1; // one
  float target2; // one
  float target3; // zero

  int numverts;
  int vertindex;
  inline mstudiovertanim_t *pVertanim(int i) const { return (mstudiovertanim_t *)(((byte *)this) + vertindex) + i; };

  int flexpair; // second flex desc
  int unused[7];
};


struct mstudioflexop_t
{
  int op;
  union
  {
    int index;
    float value;
  } d;
};

struct mstudioflexrule_t
{
  int flex;
  int numops;
  int opindex;
  inline mstudioflexop_t *iFlexOp(int i) const { return (mstudioflexop_t *)(((byte *)this) + opindex) + i; };
};

// 16 bytes
struct mstudioboneweight_t
{
  float weight[MAX_NUM_BONES_PER_VERT];
  char bone[MAX_NUM_BONES_PER_VERT];
  byte numbones;

  //      byte    material;
  //      short   firstref;
  //      short   lastref;
};

// NOTE: This is exactly 48 bytes
struct mstudiovertex_t
{
  mstudioboneweight_t m_BoneWeights;
  Vector m_vecPosition;
  Vector m_vecNormal;
  Vector2D m_vecTexCoord;

  mstudiovertex_t() {}

private:
  // No copy constructors allowed
  mstudiovertex_t(const mstudiovertex_t &vOther);
};

// skin info
struct mstudiotexture_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int flags;
  float width;                  // portion used
  float height;                 // portion used
  mutable IMaterial *material;  // fixme: this needs to go away . .isn't used by the engine, but is used by studiomdl
  mutable void *clientmaterial; // gary, replace with client material pointer if used
  float dPdu;                   // world units per u
  float dPdv;                   // world units per v

  int unused[8];
};

// eyeball
struct mstudioeyeball_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int bone;
  Vector org;
  float zoffset;
  float radius;
  Vector up;
  Vector forward;
  int texture;

  int iris_material;
  float iris_scale;
  int glint_material; // !!!

  int upperflexdesc[3]; // index of raiser, neutral, and lowerer flexdesc that is set by flex controllers
  int lowerflexdesc[3];
  float uppertarget[3]; // angle (radians) of raised, neutral, and lowered lid positions
  float lowertarget[3];
  // int           upperflex;      // index of actual flex
  // int           lowerflex;

  int upperlidflexdesc; // index of flex desc that actual lid flexes look to
  int lowerlidflexdesc;

  float pitch[2]; // min/max pitch
  float yaw[2];   // min/max yaw

  int unused[8];

private:
  // No copy constructors allowed
  mstudioeyeball_t(const mstudioeyeball_t &vOther);
};


// ikinfo
struct mstudioiklink_t
{
  int bone;
  Vector kneeDir; // ideal bending direction (per link, if applicable)
  Vector unused0; // unused

private:
  // No copy constructors allowed
  mstudioiklink_t(const mstudioiklink_t &vOther);
};

struct mstudioikchain_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int linktype;
  int numlinks;
  int linkindex;
  inline mstudioiklink_t *pLink(int i) const { return (mstudioiklink_t *)(((byte *)this) + linkindex) + i; };
  // FIXME: add unused entries
};


struct mstudioiface_t
{
  unsigned short a, b, c; // Indices to vertices
};


struct mstudiomodel_t;

struct mstudio_modelvertexdata_t
{
  Vector *Position(int i) const;
  Vector *Normal(int i) const;
  Vector4D *TangentS(int i) const;
  Vector2D *Texcoord(int i) const;
  mstudioboneweight_t *BoneWeights(int i) const;
  mstudiovertex_t *Vertex(int i) const;
  bool HasTangentData(void) const;

  // base of external vertex data stores
  void *pVertexData;
  void *pTangentData;
};

struct mstudio_meshvertexdata_t
{
  Vector *Position(int i) const;
  Vector *Normal(int i) const;
  Vector4D *TangentS(int i) const;
  Vector2D *Texcoord(int i) const;
  mstudioboneweight_t *BoneWeights(int i) const;
  mstudiovertex_t *Vertex(int i) const;
  bool HasTangentData(void) const;

  // indirection to this mesh's model's vertex data
  const mstudio_modelvertexdata_t *modelvertexdata;

  // used for fixup calcs when culling top level lods
  // expected number of mesh verts at desired lod
  int numLODVertexes[MAX_NUM_LODS];
};

struct mstudiomesh_t
{
  int material;

  int modelindex;
  mstudiomodel_t *pModel() const;

  int numvertices;  // number of unique vertices/normals/texcoords
  int vertexoffset; // vertex mstudiovertex_t

  const mstudio_meshvertexdata_t *GetVertexData();

  int numflexes; // vertex animation
  int flexindex;
  inline mstudioflex_t *pFlex(int i) const { return (mstudioflex_t *)(((byte *)this) + flexindex) + i; };

  // special codes for material operations
  int materialtype;
  int materialparam;

  // a unique ordinal for this mesh
  int meshid;

  Vector center;

  mstudio_meshvertexdata_t vertexdata;

  int unused[8]; // remove as appropriate

private:
  // No copy constructors allowed
  mstudiomesh_t(const mstudiomesh_t &vOther);
};

// studio models
struct mstudiomodel_t
{
  char name[64];

  int type;

  float boundingradius;

  int nummeshes;
  int meshindex;
  inline mstudiomesh_t *pMesh(int i) const { return (mstudiomesh_t *)(((byte *)this) + meshindex) + i; };

  // cache purposes
  int numvertices;   // number of unique vertices/normals/texcoords
  int vertexindex;   // vertex Vector
  int tangentsindex; // tangents Vector

  const mstudio_modelvertexdata_t *GetVertexData();

  int numattachments;
  int attachmentindex;

  int numeyeballs;
  int eyeballindex;
  inline mstudioeyeball_t *pEyeball(int i) { return (mstudioeyeball_t *)(((byte *)this) + eyeballindex) + i; };

  mstudio_modelvertexdata_t vertexdata;

  int unused[8]; // remove as appropriate
};

inline bool mstudio_modelvertexdata_t::HasTangentData(void) const { return (pTangentData != NULL); }

inline mstudiovertex_t *mstudio_modelvertexdata_t::Vertex(int i) const
{
  mstudiomodel_t *modelptr = (mstudiomodel_t *)((byte *)this - offsetof(mstudiomodel_t, vertexdata));
  return (mstudiovertex_t *)((byte *)pVertexData + modelptr->vertexindex) + i;
}

inline Vector *mstudio_modelvertexdata_t::Position(int i) const { return &Vertex(i)->m_vecPosition; }

inline Vector *mstudio_modelvertexdata_t::Normal(int i) const { return &Vertex(i)->m_vecNormal; }

inline Vector4D *mstudio_modelvertexdata_t::TangentS(int i) const
{
  // NOTE: The tangents vector is 16-bytes in a separate array
  // because it only exists on the high end, and if I leave it out
  // of the mstudiovertex_t, the vertex is 64-bytes (good for low end)
  mstudiomodel_t *modelptr = (mstudiomodel_t *)((byte *)this - offsetof(mstudiomodel_t, vertexdata));
  return (Vector4D *)((byte *)pTangentData + modelptr->tangentsindex) + i;
}

inline Vector2D *mstudio_modelvertexdata_t::Texcoord(int i) const { return &Vertex(i)->m_vecTexCoord; }

inline mstudioboneweight_t *mstudio_modelvertexdata_t::BoneWeights(int i) const { return &Vertex(i)->m_BoneWeights; }

inline mstudiomodel_t *mstudiomesh_t::pModel() const { return (mstudiomodel_t *)(((byte *)this) + modelindex); }

inline bool mstudio_meshvertexdata_t::HasTangentData(void) const { return modelvertexdata->HasTangentData(); }

inline const mstudio_meshvertexdata_t *mstudiomesh_t::GetVertexData()
{
  // get this mesh's model's vertex data
  vertexdata.modelvertexdata = this->pModel()->GetVertexData();

  return &vertexdata;
}

inline Vector *mstudio_meshvertexdata_t::Position(int i) const
{
  mstudiomesh_t *meshptr = (mstudiomesh_t *)((byte *)this - offsetof(mstudiomesh_t, vertexdata));
  return modelvertexdata->Position(meshptr->vertexoffset + i);
};

inline Vector *mstudio_meshvertexdata_t::Normal(int i) const
{
  mstudiomesh_t *meshptr = (mstudiomesh_t *)((byte *)this - offsetof(mstudiomesh_t, vertexdata));
  return modelvertexdata->Normal(meshptr->vertexoffset + i);
};

inline Vector4D *mstudio_meshvertexdata_t::TangentS(int i) const
{
  mstudiomesh_t *meshptr = (mstudiomesh_t *)((byte *)this - offsetof(mstudiomesh_t, vertexdata));
  return modelvertexdata->TangentS(meshptr->vertexoffset + i);
}

inline Vector2D *mstudio_meshvertexdata_t::Texcoord(int i) const
{
  mstudiomesh_t *meshptr = (mstudiomesh_t *)((byte *)this - offsetof(mstudiomesh_t, vertexdata));
  return modelvertexdata->Texcoord(meshptr->vertexoffset + i);
};

inline mstudioboneweight_t *mstudio_meshvertexdata_t::BoneWeights(int i) const
{
  mstudiomesh_t *meshptr = (mstudiomesh_t *)((byte *)this - offsetof(mstudiomesh_t, vertexdata));
  return modelvertexdata->BoneWeights(meshptr->vertexoffset + i);
};

inline mstudiovertex_t *mstudio_meshvertexdata_t::Vertex(int i) const
{
  mstudiomesh_t *meshptr = (mstudiomesh_t *)((byte *)this - offsetof(mstudiomesh_t, vertexdata));
  return modelvertexdata->Vertex(meshptr->vertexoffset + i);
}

// a group of studio model data
enum studiomeshgroupflags_t
{
  MESHGROUP_IS_FLEXED = 0x1,
  MESHGROUP_IS_HWSKINNED = 0x2
};


// ----------------------------------------------------------
// runtime stuff
// ----------------------------------------------------------

struct studiomeshgroup_t
{
  IMesh *m_pMesh;
  IMesh *m_pColorMesh;
  int m_NumStrips;
  int m_Flags; // see studiomeshgroupflags_t
  OptimizedModel::StripHeader_t *m_pStripData;
  unsigned short *m_pGroupIndexToMeshIndex;
  int m_NumVertices;
  int *m_pUniqueTris; // for performance measurements
  unsigned short *m_pIndices;
  bool m_MeshNeedsRestore;
  short m_ColorMeshID;

  inline unsigned short MeshIndex(int i) const { return m_pGroupIndexToMeshIndex[m_pIndices[i]]; }
};


// studio model data
struct studiomeshdata_t
{
  int m_NumGroup;
  studiomeshgroup_t *m_pMeshGroup;
};

struct studioloddata_t
{
  // not needed - this is really the same as studiohwdata_t.m_NumStudioMeshes
  // int                                   m_NumMeshes;
  studiomeshdata_t *m_pMeshData; // there are studiohwdata_t.m_NumStudioMeshes of these.
  float m_SwitchPoint;
  // one of these for each lod since we can switch to simpler materials on lower lods.
  int numMaterials;
  IMaterial **ppMaterials; /* will have studiohdr_t.numtextures elements allocated */
  // hack - this needs to go away.
  int *pMaterialFlags; /* will have studiohdr_t.numtextures elements allocated */
};

struct studiohwdata_t
{
  int m_RootLOD; // calced and clamped, nonzero for lod culling
  int m_NumLODs;
  studioloddata_t *m_pLODs;
  int m_NumStudioMeshes;
};

// ----------------------------------------------------------
// ----------------------------------------------------------

// body part index
struct mstudiobodyparts_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int nummodels;
  int base;
  int modelindex; // index into models array
  inline mstudiomodel_t *pModel(int i) const { return (mstudiomodel_t *)(((byte *)this) + modelindex) + i; };
};


struct mstudiomouth_t
{
  int bone;
  Vector forward;
  int flexdesc;

private:
  // No copy constructors allowed
  mstudiomouth_t(const mstudiomouth_t &vOther);
};

struct mstudiohitboxset_t
{
  int sznameindex;
  inline char *const pszName(void) const { return ((char *)this) + sznameindex; }
  int numhitboxes;
  int hitboxindex;
  inline mstudiobbox_t *pHitbox(int i) const { return (mstudiobbox_t *)(((byte *)this) + hitboxindex) + i; };
};


// ----------------------------------------------------------
// Purpose: Load time results on model compositing
// ----------------------------------------------------------

class virtualgroup_t
{
public:
  virtualgroup_t(void) { cache = NULL; };
  // tool dependant.  In engine this is a model_t, in tool it's a direct pointer
  void *cache;
  // converts cache entry into a usable studiohdr_t *
  const studiohdr_t *GetStudioHdr(void) const;

  CUtlVector<int> boneMap;          // maps global bone to local bone
  CUtlVector<int> masterBone;       // maps local bone to global bone
  CUtlVector<int> masterSeq;        // maps local sequence to master sequence
  CUtlVector<int> masterAnim;       // maps local animation to master animation
  CUtlVector<int> masterAttachment; // maps local attachment to global
  CUtlVector<int> masterPose;       // maps local pose parameter to global
  CUtlVector<int> masterNode;       // maps local transition nodes to global
};

struct virtualsequence_t
{
  int flags;
  int activity;
  int group;
  int index;
};

struct virtualgeneric_t
{
  int group;
  int index;
};

struct virtualmodel_t
{
  void AppendSequences(int group, const studiohdr_t *pStudioHdr);
  void AppendAnimations(int group, const studiohdr_t *pStudioHdr);
  void AppendAttachments(int ground, const studiohdr_t *pStudioHdr);
  void AppendPoseParameters(int group, const studiohdr_t *pStudioHdr);
  void AppendBonemap(int group, const studiohdr_t *pStudioHdr);
  void AppendNodes(int group, const studiohdr_t *pStudioHdr);
  void AppendTransitions(int group, const studiohdr_t *pStudioHdr);
  void AppendIKLocks(int group, const studiohdr_t *pStudioHdr);
  void AppendModels(int group, const studiohdr_t *pStudioHdr);

  virtualgroup_t *pAnimGroup(int animation) { return &m_group[m_anim[animation].group]; };
  virtualgroup_t *pSeqGroup(int sequence) { return &m_group[m_seq[sequence].group]; };

  CUtlVector<virtualsequence_t> m_seq;
  CUtlVector<virtualgeneric_t> m_anim;
  CUtlVector<virtualgeneric_t> m_attachment;
  CUtlVector<virtualgeneric_t> m_pose;
  CUtlVector<virtualgroup_t> m_group;
  CUtlVector<virtualgeneric_t> m_node;
  CUtlVector<CUtlVector<byte>> m_transition;
  CUtlVector<virtualgeneric_t> m_iklock;
};

// ----------------------------------------------------------
// Studio Model Vertex Data File
// Position independent flat data for cache manager
// ----------------------------------------------------------

// little-endian "IDSV"
#define MODEL_VERTEX_FILE_ID      (('V' << 24) + ('S' << 16) + ('D' << 8) + 'I')
#define MODEL_VERTEX_FILE_VERSION 4

struct vertexFileHeader_t
{
  int id;                           // MODEL_VERTEX_FILE_ID
  int version;                      // MODEL_VERTEX_FILE_VERSION
  long checksum;                    // same as studiohdr_t, ensures sync
  int numLODs;                      // num of valid lods
  int numLODVertexes[MAX_NUM_LODS]; // num verts for desired root lod
  int numFixups;                    // num of vertexFileFixup_t
  int fixupTableStart;              // offset from base to fixup table
  int vertexDataStart;              // offset from base to vertex block
  int tangentDataStart;             // offset from base to tangent block
};

// apply sequentially to lod sorted vertex and tangent pools to re-establish mesh order
struct vertexFileFixup_t
{
  int lod;            // used to skip culled root lod
  int sourceVertexID; // absolute index from start of vertex/tangent blocks
  int numVertexes;
};

// This flag is set if no hitbox information was specified
#define STUDIOHDR_FLAGS_AUTOGENERATED_HITBOX (1 << 0)

// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
// models when we change materials.
#define STUDIOHDR_FLAGS_USES_ENV_CUBEMAP (1 << 1)

// Use this when there are translucent parts to the model but we're not going to sort it
#define STUDIOHDR_FLAGS_FORCE_OPAQUE (1 << 2)

// Use this when we want to render the opaque parts during the opaque pass
// and the translucent parts during the translucent pass
#define STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS (1 << 3)

// This is set any time the .qc files has $staticprop in it
// Means there's no bones and no transforms
#define STUDIOHDR_FLAGS_STATIC_PROP (1 << 4)

// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
// models when we change materials.
#define STUDIOHDR_FLAGS_USES_FB_TEXTURE (1 << 5)

// This flag is set by studiomdl.exe if a separate "$shadowlod" entry was present
//  for the .mdl (the shadow lod is the last entry in the lod list if present)
#define STUDIOHDR_FLAGS_HASSHADOWLOD (1 << 6)

// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
// models when we change materials.
#define STUDIOHDR_FLAGS_USES_BUMPMAPPING (1 << 7)

// NOTE:  This flag is set when we should use the actual materials on the shadow LOD
// instead of overriding them with the default one (necessary for translucent shadows)
#define STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS (1 << 8)

// NOTE:  This flag is set when we should use the actual materials on the shadow LOD
// instead of overriding them with the default one (necessary for translucent shadows)
#define STUDIOHDR_FLAGS_OBSOLETE (1 << 9)

#define STUDIOHDR_FLAGS_UNUSED (1 << 10)

// NOTE:  This flag is set at mdl build time
#define STUDIOHDR_FLAGS_NO_FORCED_FADE (1 << 11)

// NOTE:  The npc will lengthen the viseme check to always include two phonemes
#define STUDIOHDR_FLAGS_FORCE_PHONEME_CROSSFADE (1 << 12)

struct studiohdr_t
{
  int id;
  int version;

  long checksum; // this has to be the same in the phy and vtx files to load!

  char name[64];
  int length;


  Vector eyeposition; // ideal eye position

  Vector illumposition; // illumination center

  Vector hull_min; // ideal movement hull size
  Vector hull_max;

  Vector view_bbmin; // clipping bounding box
  Vector view_bbmax;

  int flags;

  int numbones; // bones
  int boneindex;
  inline mstudiobone_t *pBone(int i) const
  {
    Assert(i >= 0 && i < numbones);
    return (mstudiobone_t *)(((byte *)this) + boneindex) + i;
  };
  int RemapSeqBone(int iSequence, int iLocalBone) const; // maps local sequence bone to global bone
  int RemapAnimBone(int iAnim, int iLocalBone) const;    // maps local animations bone to global bone

  int numbonecontrollers; // bone controllers
  int bonecontrollerindex;
  inline mstudiobonecontroller_t *pBonecontroller(int i) const
  {
    Assert(i >= 0 && i < numbonecontrollers);
    return (mstudiobonecontroller_t *)(((byte *)this) + bonecontrollerindex) + i;
  };

  int numhitboxsets;
  int hitboxsetindex;

  // Look up hitbox set by index
  mstudiohitboxset_t *pHitboxSet(int i) const
  {
    Assert(i >= 0 && i < numhitboxsets);
    return (mstudiohitboxset_t *)(((byte *)this) + hitboxsetindex) + i;
  };

  // Calls through to hitbox to determine size of specified set
  inline mstudiobbox_t *pHitbox(int i, int set) const
  {
    mstudiohitboxset_t const *s = pHitboxSet(set);
    if (!s)
      return NULL;

    return s->pHitbox(i);
  };

  // Calls through to set to get hitbox count for set
  inline int iHitboxCount(int set) const
  {
    mstudiohitboxset_t const *s = pHitboxSet(set);
    if (!s)
      return 0;

    return s->numhitboxes;
  };

  // file local animations? and sequences
  // private:
  int numlocalanim;   // animations/poses
  int localanimindex; // animation descriptions
  inline mstudioanimdesc_t *pLocalAnimdesc(int i) const
  {
    if (i < 0 || i >= numlocalanim)
      i = 0;
    return (mstudioanimdesc_t *)(((byte *)this) + localanimindex) + i;
  };

  int numlocalseq; // sequences
  int localseqindex;
  inline mstudioseqdesc_t *pLocalSeqdesc(int i) const
  {
    if (i < 0 || i >= numlocalseq)
      i = 0;
    return (mstudioseqdesc_t *)(((byte *)this) + localseqindex) + i;
  };

  // public:
  int GetNumSeq() const;
  mstudioanimdesc_t &pAnimdesc(int i) const;
  mstudioseqdesc_t &pSeqdesc(int i) const;
  int iRelativeAnim(int baseseq, int relanim) const; // maps seq local anim reference to global anim index
  int iRelativeSeq(int baseseq, int relseq) const;   // maps seq local seq reference to global seq index

  // private:
  mutable int activitylistversion; // initialization flag - have the sequences been indexed?
  int eventsindexed;
  // public:
  // int                                   GetSequenceActivity( int iSequence );
  // void                          SetSequenceActivity( int iSequence, int iActivity );
  int GetActivityListVersion(void) const;
  void SetActivityListVersion(int version) const;

  // raw textures
  int numtextures;
  int textureindex;
  inline mstudiotexture_t *pTexture(int i) const { return (mstudiotexture_t *)(((byte *)this) + textureindex) + i; };


  // raw textures search paths
  int numcdtextures;
  int cdtextureindex;
  inline char *pCdtexture(int i) const { return (((char *)this) + *((int *)(((byte *)this) + cdtextureindex) + i)); };

  // replaceable textures tables
  int numskinref;
  int numskinfamilies;
  int skinindex;
  inline short *pSkinref(int i) const { return (short *)(((byte *)this) + skinindex) + i; };

  int numbodyparts;
  int bodypartindex;
  inline mstudiobodyparts_t *pBodypart(int i) const { return (mstudiobodyparts_t *)(((byte *)this) + bodypartindex) + i; };

  // queryable attachable points
  // private:
  int numlocalattachments;
  int localattachmentindex;
  inline mstudioattachment_t *pLocalAttachment(int i) const
  {
    Assert(i >= 0 && i < numlocalattachments);
    return (mstudioattachment_t *)(((byte *)this) + localattachmentindex) + i;
  };
  // public:
  int GetNumAttachments(void) const;
  const mstudioattachment_t &pAttachment(int i) const;
  int GetAttachmentBone(int i) const;
  // used on my tools in hlmv, not persistant
  void SetAttachmentBone(int iAttachment, int iBone);

  // animation node to animation node transition graph
  // private:
  int numlocalnodes;
  int localnodeindex;
  int localnodenameindex;
  inline char *pszLocalNodeName(int iNode) const
  {
    Assert(iNode >= 0 && iNode < numlocalnodes);
    return (((char *)this) + *((int *)(((byte *)this) + localnodenameindex) + iNode));
  }
  inline byte *pLocalTransition(int i) const
  {
    Assert(i >= 0 && i < (numlocalnodes * numlocalnodes));
    return (byte *)(((byte *)this) + localnodeindex) + i;
  };

  // public:
  int EntryNode(int iSequence) const;
  int ExitNode(int iSequence) const;
  char *pszNodeName(int iNode) const;
  int GetTransition(int iFrom, int iTo) const;

  int numflexdesc;
  int flexdescindex;
  inline mstudioflexdesc_t *pFlexdesc(int i) const
  {
    Assert(i >= 0 && i < numflexdesc);
    return (mstudioflexdesc_t *)(((byte *)this) + flexdescindex) + i;
  };

  int numflexcontrollers;
  int flexcontrollerindex;
  inline mstudioflexcontroller_t *pFlexcontroller(int i) const
  {
    Assert(i >= 0 && i < numflexcontrollers);
    return (mstudioflexcontroller_t *)(((byte *)this) + flexcontrollerindex) + i;
  };

  int numflexrules;
  int flexruleindex;
  inline mstudioflexrule_t *pFlexRule(int i) const
  {
    Assert(i >= 0 && i < numflexrules);
    return (mstudioflexrule_t *)(((byte *)this) + flexruleindex) + i;
  };

  int numikchains;
  int ikchainindex;
  inline mstudioikchain_t *pIKChain(int i) const
  {
    Assert(i >= 0 && i < numikchains);
    return (mstudioikchain_t *)(((byte *)this) + ikchainindex) + i;
  };

  int nummouths;
  int mouthindex;
  inline mstudiomouth_t *pMouth(int i) const
  {
    Assert(i >= 0 && i < nummouths);
    return (mstudiomouth_t *)(((byte *)this) + mouthindex) + i;
  };

  // private:
  int numlocalposeparameters;
  int localposeparamindex;
  inline mstudioposeparamdesc_t *pLocalPoseParameter(int i) const
  {
    Assert(i >= 0 && i < numlocalposeparameters);
    return (mstudioposeparamdesc_t *)(((byte *)this) + localposeparamindex) + i;
  };
  // public:
  int GetNumPoseParameters(void) const;
  const mstudioposeparamdesc_t &pPoseParameter(int i) const;
  int GetSharedPoseParameter(int iSequence, int iLocalPose) const;

  int surfacepropindex;
  inline char *const pszSurfaceProp(void) const { return ((char *)this) + surfacepropindex; }

  // Key values
  int keyvalueindex;
  int keyvaluesize;
  inline const char *KeyValueText(void) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

  int numlocalikautoplaylocks;
  int localikautoplaylockindex;
  inline mstudioiklock_t *pLocalIKAutoplayLock(int i) const
  {
    Assert(i >= 0 && i < numlocalikautoplaylocks);
    return (mstudioiklock_t *)(((byte *)this) + localikautoplaylockindex) + i;
  };

  int GetNumIKAutoplayLocks(void) const;
  const mstudioiklock_t &pIKAutoplayLock(int i) const;

  // The collision model mass that jay wanted
  float mass;
  int contents;

  // external animations, models, etc.
  int numincludemodels;
  int includemodelindex;
  inline mstudiomodelgroup_t *pModelGroup(int i) const
  {
    Assert(i >= 0 && i < numincludemodels);
    return (mstudiomodelgroup_t *)(((byte *)this) + includemodelindex) + i;
  };
  // implementation specific call to get a named model
  const studiohdr_t *FindModel(void **cache, char const *modelname) const;

  // implementation specific back pointer to virtual data
  mutable void *virtualModel;
  virtualmodel_t *GetVirtualModel(void) const;

  // for demand loaded animation blocks
  int szanimblocknameindex;
  inline char *const pszAnimBlockName(void) const { return ((char *)this) + szanimblocknameindex; }
  int numanimblocks;
  int animblockindex;
  inline mstudioanimblock_t *pAnimBlock(int i) const
  {
    Assert(i > 0 && i < numanimblocks);
    return (mstudioanimblock_t *)(((byte *)this) + animblockindex) + i;
  };
  mutable void *animblockModel;
  byte *GetAnimBlock(int i) const;

  int bonetablebynameindex;
  inline const byte *GetBoneTableSortedByName() const { return (byte *)this + bonetablebynameindex; }

  // used by tools only that don't cache, but persist mdl's peer data
  // engine uses virtualModel to back link to cache pointers
  void *pVertexBase;
  void *pIndexBase;

  int unused[8]; // remove as appropriate

  studiohdr_t() {}

private:
  // No copy constructors allowed
  studiohdr_t(const studiohdr_t &vOther);

  friend struct virtualmodel_t;
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

struct flexweight_t
{
  int key;
  float weight;
  float influence;
};

//-----------------------------------------------------------------------------
// Purpose: A markov group basically indexes another flex setting and includes a
//  weighting factor which is used to factor how likely the specified member
//  is to be picked
//-----------------------------------------------------------------------------
struct flexmarkovgroup_t
{
  int settingnumber;
  int weight;
};

#define FS_NORMAL 0
#define FS_MARKOV 1

struct flexsetting_t
{
  int nameindex;

  inline char *pszName(void) const { return (char *)(((byte *)this) + nameindex); }

  inline bool IsMarkov(void) const { return type == FS_MARKOV ? true : false; }

  // FS_NORMAL or FS_MARKOV
  int type;

  // Number of flex settings for FS_NORMAL or Number of
  //  Markov group members for FS_MARKOV
  int numsettings;
  int index;

  // For FS_MARKOV only, the client .dll writes the current index into here so that
  //  it can retain which markov group is being followed. This is reset every time the expression
  //  starts to play back during scene playback
  int currentindex;

  // Index of start of contiguous array of flexweight_t or
  //  flexmarkovgroup_t structures
  int settingindex;

  //-----------------------------------------------------------------------------
  // Purpose: Retrieves the specified markov group header if the entry is a markov entry
  // Input  : member -
  // Output : flexmarkovgroup_t *
  //-----------------------------------------------------------------------------
  inline flexmarkovgroup_t *pMarkovGroup(int member) const
  {
    // type must be FS_MARKOV to return this pointer
    if (!IsMarkov())
      return NULL;

    if (member < 0 || member >= numsettings)
      return NULL;

    flexmarkovgroup_t *group = (flexmarkovgroup_t *)(((byte *)this) + settingindex) + member;
    return group;
  };

  //-----------------------------------------------------------------------------
  // Purpose:
  // Input  : member -
  // Output : int
  //-----------------------------------------------------------------------------
  inline int GetMarkovSetting(int member) const
  {
    flexmarkovgroup_t *group = pMarkovGroup(member);
    if (!group)
      return -1;

    return group->settingnumber;
  };

  //-----------------------------------------------------------------------------
  // Purpose: Retrieves a pointer to the flexweight_t, including resolving
  //  any markov chain hierarchy.  Because of this possibility, we return
  //  the number of settings in the weights array returned.  We'll generally
  //  call this function with i == 0
  // Input  : *base -
  //                      i -
  //                      **weights -
  // Output : int
  //-----------------------------------------------------------------------------
  inline int psetting(byte *base, int i, flexweight_t **weights) const;
};


struct flexsettinghdr_t
{
  int id;
  int version;

  char name[64];
  int length;

  int numflexsettings;
  int flexsettingindex;
  inline flexsetting_t *pSetting(int i) const { return (flexsetting_t *)(((byte *)this) + flexsettingindex) + i; };
  int nameindex;

  // look up flex settings by "index"
  int numindexes;
  int indexindex;

  inline flexsetting_t *pIndexedSetting(int index) const
  {
    if (index < 0 || index >= numindexes)
    {
      return NULL;
    }

    int i = *((int *)(((byte *)this) + indexindex) + index);

    if (i == -1)
    {
      return NULL;
    }

    return pSetting(i);
  }

  // index names of "flexcontrollers"
  int numkeys;
  int keynameindex;
  inline char *pLocalName(int i) const { return (char *)(((byte *)this) + *((int *)(((byte *)this) + keynameindex) + i)); };

  int keymappingindex;
  inline int *pLocalToGlobal(int i) const { return (int *)(((byte *)this) + keymappingindex) + i; };
  inline int LocalToGlobal(int i) const { return *pLocalToGlobal(i); };

  //-----------------------------------------------------------------------------
  // Purpose: Same as pSetting( int i ) above, except it translates away any markov groups first
  // Input  : i - index to retrieve
  // Output : flexsetting_t * - a non-markov underlying flexsetting_t
  //-----------------------------------------------------------------------------
  inline flexsetting_t *pTranslatedSetting(int i) const
  {
    flexsetting_t *setting = (flexsetting_t *)(((byte *)this) + flexsettingindex) + i;
    // If this one is not a markov setting, return it
    if (!setting->IsMarkov())
    {
      return setting;
    }

    int newindex = setting->GetMarkovSetting(setting->currentindex);
    // Ack, this should never happen (the markov references something that is gone)
    //  Since there was a problem,
    //  just return this setting anyway, sigh.
    if (newindex == -1)
    {
      return setting;
    }

    // Otherwise, recurse on the translated index
    // NOTE:  It's theoretically possible to have an infinite recursion if two markov
    //  groups reference each other.  The faceposer shouldn't create such groups,
    //  so I don't think this will ever actually occur -- ywb
    return pTranslatedSetting(newindex);
  }
};

//-----------------------------------------------------------------------------
// Purpose: Retrieves a pointer to the flexweight_t, including resolving
//  any markov chain hierarchy.  Because of this possibility, we return
//  the number of settings in the weights array returned.  We'll generally
//  call this function with i == 0
// Input  : *base - flexsettinghdr_t * pointer
//                      i - index of flex setting to retrieve
//                      **weights - destination for weights array starting at index i.
// Output : int
//-----------------------------------------------------------------------------
inline int flexsetting_t::psetting(byte *base, int i, flexweight_t **weights) const
{
  // Assume failure to find index
  *weights = NULL;

  // Recurse if this is a markov setting
  if (IsMarkov())
  {
    // Find the current redirected index
    int settingnum = GetMarkovSetting(currentindex);
    if (settingnum == -1)
    {
      // Couldn't find currentindex in the markov list for this flex setting
      return -1;
    }

    // Follow the markov link instead
    flexsetting_t *setting = ((flexsettinghdr_t *)base)->pSetting(settingnum);
    if (!setting)
    {
      return -1;
    }

    // Recurse ( could support more than one level of markov chains this way )
    return setting->psetting(base, i, weights);
  }

  // Grab array pointer
  *weights = (flexweight_t *)(((byte *)this) + settingindex) + i;
  // Return true number of settings
  return numsettings;
};

#define STUDIO_CONST  1 // get float
#define STUDIO_FETCH1 2 // get Flexcontroller value
#define STUDIO_FETCH2 3 // get flex weight
#define STUDIO_ADD    4
#define STUDIO_SUB    5
#define STUDIO_MUL    6
#define STUDIO_DIV    7
#define STUDIO_NEG    8  // not implemented
#define STUDIO_EXP    9  // not implemented
#define STUDIO_OPEN   10 // only used in token parsing
#define STUDIO_CLOSE  11

// motion flags
#define STUDIO_X  0x00000001
#define STUDIO_Y  0x00000002
#define STUDIO_Z  0x00000004
#define STUDIO_XR 0x00000008
#define STUDIO_YR 0x00000010
#define STUDIO_ZR 0x00000020

#define STUDIO_LX  0x00000040
#define STUDIO_LY  0x00000080
#define STUDIO_LZ  0x00000100
#define STUDIO_LXR 0x00000200
#define STUDIO_LYR 0x00000400
#define STUDIO_LZR 0x00000800

#define STUDIO_LINEAR 0x00001000

#define STUDIO_TYPES 0x0003FFFF
#define STUDIO_RLOOP 0x00040000 // controller that wraps shortest distance

// sequence flags
#define STUDIO_LOOPING  0x00000001 // ending frame should be the same as the starting frame
#define STUDIO_SNAP     0x00000002 // do not interpolate between previous animation and this one
#define STUDIO_DELTA    0x00000004 // this sequence "adds" to the base sequences, not slerp blends
#define STUDIO_AUTOPLAY 0x00000008 // temporary flag that forces the sequence to always play
#define STUDIO_POST     0x00000010 //
#define STUDIO_ALLZEROS 0x00000020 // this animation/sequence has no real animation data
#define STUDIO_SPLINE   0x00000040 // convert layer ramp in/out curve is a spline instead of linear
#define STUDIO_XFADE \
  0x00000080 // pre-bias the ramp curve to compense for a non-1 weight, assuming a second layer is also going to accumulate
#define STUDIO_REALTIME 0x00000100 // cycle index is taken from a real-time clock, not the animations cycle index
#define STUDIO_NOBLEND  0x00000200 // animation always blends at 1.0 (ignores weight)
#define STUDIO_HIDDEN   0x00000400 // don't show in default selection views
#define STUDIO_OVERRIDE 0x00000800 // a forward declared sequence (empty)
#define STUDIO_ACTIVITY 0x00001000 // Has been updated at runtime to activity index
#define STUDIO_EVENT    0x00002000 // Has been updated at runtime to event index


// Insert this code anywhere that you need to allow for conversion from an old STUDIO_VERSION
// to a new one.
// If we only support the current version, this function should be empty.
inline void Studio_ConvertStudioHdrToNewVersion(studiohdr_t *pStudioHdr)
{
  COMPILE_TIME_ASSERT(STUDIO_VERSION == 44); //  put this to make sure this code is updated upon changing version.

  int version = pStudioHdr->version;
  if (version == STUDIO_VERSION)
  {
    return;
  }
}

// must be run to fixup with specified rootLOD
inline void Studio_SetRootLOD(studiohdr_t *pStudioHdr, int rootLOD)
{
  // run the lod fixups that culls higher detail lods
  // vertexes are external, fixups ensure relative offsets and counts are cognizant of shrinking data
  // indexes are built in lodN..lod0 order so higher detail lod data can be truncated at load
  int vertexindex = 0;
  int tangentsindex = 0;
  int bodyPartID;
  for (bodyPartID = 0; bodyPartID < pStudioHdr->numbodyparts; bodyPartID++)
  {
    mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart(bodyPartID);
    int modelID;
    for (modelID = 0; modelID < pBodyPart->nummodels; modelID++)
    {
      mstudiomodel_t *pModel = pBodyPart->pModel(modelID);
      int totalMeshVertexes = 0;
      int meshID;
      for (meshID = 0; meshID < pModel->nummeshes; meshID++)
      {
        mstudiomesh_t *pMesh = pModel->pMesh(meshID);

        // get the fixup, vertexes are reduced
        pMesh->numvertices = pMesh->vertexdata.numLODVertexes[rootLOD];
        pMesh->vertexoffset = totalMeshVertexes;
        totalMeshVertexes += pMesh->numvertices;
      }

      // stay in sync
      pModel->numvertices = totalMeshVertexes;
      pModel->vertexindex = vertexindex;
      pModel->tangentsindex = tangentsindex;

      vertexindex += totalMeshVertexes * sizeof(mstudiovertex_t);
      tangentsindex += totalMeshVertexes * sizeof(Vector4D);
    }
  }
}

// Determines reduced allocation requirements for vertexes
inline int Studio_VertexDataSize(const vertexFileHeader_t *pVvdHdr, int rootLOD, bool bNeedsTangentS)
{
  // the quantity of vertexes necessary for root lod and all lower detail lods
  // add one extra vertex to each section
  // the extra vertex allows prefetch hints to read ahead 1 vertex without faulting
  int numVertexes = pVvdHdr->numLODVertexes[rootLOD] + 1;
  int dataLength = pVvdHdr->vertexDataStart + numVertexes * sizeof(mstudiovertex_t);
  if (bNeedsTangentS)
  {
    dataLength += numVertexes * sizeof(Vector4D);
  }

  // allocate this much
  return dataLength;
}

// Load the minimum qunatity of verts and run fixups
inline int Studio_LoadVertexes(const vertexFileHeader_t *pTempVvdHdr, vertexFileHeader_t *pNewVvdHdr, int rootLOD, bool bNeedsTangentS)
{
  int i;
  int target;
  int numVertexes;
  vertexFileFixup_t *pFixupTable;

  numVertexes = pTempVvdHdr->numLODVertexes[rootLOD];

  // copy all data up to start of vertexes
  memcpy((void *)pNewVvdHdr, (void *)pTempVvdHdr, pTempVvdHdr->vertexDataStart);

  // fixup data starts
  if (bNeedsTangentS)
  {
    // tangent data follows possibly reduced vertex data
    pNewVvdHdr->tangentDataStart = pTempVvdHdr->vertexDataStart + numVertexes * sizeof(mstudiovertex_t);
  }
  else
  {
    // no tangent data will be available, mark for identification
    pNewVvdHdr->tangentDataStart = 0;
  }

  if (!pNewVvdHdr->numFixups)
  {
    // fixups not required
    // transfer vertex data
    memcpy((byte *)pNewVvdHdr + pNewVvdHdr->vertexDataStart, (byte *)pTempVvdHdr + pTempVvdHdr->vertexDataStart,
      numVertexes * sizeof(mstudiovertex_t));

    if (bNeedsTangentS)
    {
      // transfer tangent data to cache memory
      memcpy((byte *)pNewVvdHdr + pNewVvdHdr->tangentDataStart, (byte *)pTempVvdHdr + pTempVvdHdr->tangentDataStart,
        numVertexes * sizeof(Vector4D));
    }

    return numVertexes;
  }

  // fixups required
  // re-establish mesh ordered vertexes into cache memory, according to table
  target = 0;
  pFixupTable = (vertexFileFixup_t *)((byte *)pTempVvdHdr + pTempVvdHdr->fixupTableStart);
  for (i = 0; i < pTempVvdHdr->numFixups; i++)
  {
    if (pFixupTable[i].lod < rootLOD)
    {
      // working bottom up, skip over copying higher detail lods
      continue;
    }

    // copy vertexes
    memcpy((mstudiovertex_t *)((byte *)pNewVvdHdr + pNewVvdHdr->vertexDataStart) + target,
      (mstudiovertex_t *)((byte *)pTempVvdHdr + pTempVvdHdr->vertexDataStart) + pFixupTable[i].sourceVertexID,
      pFixupTable[i].numVertexes * sizeof(mstudiovertex_t));

    if (bNeedsTangentS)
    {
      // copy tangents
      memcpy((Vector4D *)((byte *)pNewVvdHdr + pNewVvdHdr->tangentDataStart) + target,
        (Vector4D *)((byte *)pTempVvdHdr + pTempVvdHdr->tangentDataStart) + pFixupTable[i].sourceVertexID,
        pFixupTable[i].numVertexes * sizeof(Vector4D));
    }

    // data is placed consecutively
    target += pFixupTable[i].numVertexes;
  }

  return target;
}

#endif // STUDIO_H

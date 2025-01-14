// Copyright (C) Gaijin Games KFT.  All rights reserved.

/*******************************************************
 *                                                      *
 *  volInt.c                                            *
 *                                                      *
 *  This code computes volume integrals needed for      *
 *  determining mass properties of polyhedral bodies.   *
 *                                                      *
 *  For more information, see the accompanying README   *
 *  file, and the paper                                 *
 *                                                      *
 *  Brian Mirtich, "Fast and Accurate Computation of    *
 *  Polyhedral Mass Properties," journal of graphics    *
 *  tools, volume 1, number 1, 1996.                    *
 *                                                      *
 *  This source code is public domain, and may be used  *
 *  in any way, shape or form, free of charge.          *
 *                                                      *
 *  Copyright 1995 by Brian Mirtich                     *
 *                                                      *
 *  mirtich@cs.berkeley.edu                             *
 *  http://www.cs.berkeley.edu/~mirtich                 *
 *                                                      *
 *******************************************************/

/*
        Revision history

        26 Jan 1996     Program creation.

         3 Aug 1996     Corrected bug arising when polyhedron density
                        is not 1.0.  Changes confined to function main().
                        Thanks to Zoran Popovic for catching this one.

        27 May 1997     Corrected sign error in translation of inertia
                        product terms to center of mass frame.  Changes
                        confined to function main().  Thanks to
                        Chris Hecker.

        28 Jul 2003     Incorporated into Dagor engine.
*/


#include <libTools/dagFileRW/geomMeshHelper.h>


/*
   ============================================================================
   constants
   ============================================================================
*/

#define X 0
#define Y 1
#define Z 2

/*
   ============================================================================
   macros
   ============================================================================
*/

#define CUBE(x) ((x) * (x) * (x))

/*
   ============================================================================
   data structures
   ============================================================================
*/

/*
typedef struct {
  int numVerts;
  double norm[3];
  double w;
  int verts[MAX_POLYGON_SZ];
  struct polyhedron *poly;
} FACE;

typedef struct polyhedron {
  int numVerts, numFaces;
  double verts[MAX_VERTS][3];
  FACE faces[MAX_FACES];
} POLYHEDRON;
*/


/*
   ============================================================================
   globals
   ============================================================================
*/

static int A; /* alpha */ //-V707
static int B; /* beta */  //-V707
static int C; /* gamma */ //-V707

/* projection integrals */
static double P1, Pa, Pb, Paa, Pab, Pbb, Paaa, Paab, Pabb, Pbbb; //-V707

/* face integrals */
static double Fa, Fb, Fc, Faa, Fbb, Fcc, Faaa, Fbbb, Fccc, Faab, Fbbc, Fcca; //-V707

/* volume integrals */
static double T0, T1[3], T2[3], TP[3]; //-V707

struct VolIntFaceData
{
  DPoint3 norm;
  double w;
};

static Tab<VolIntFaceData> facedata(tmpmem_ptr());
static Tab<Point3> verts(tmpmem_ptr());


/*
   ============================================================================
   start calculation
   ============================================================================
*/

static void start_calc(GeomMeshHelper &m, const TMatrix &tm)
{
  verts.resize(m.verts.size());
  for (int i = 0; i < verts.size(); ++i)
    verts[i] = tm * m.verts[i];

  facedata.resize(m.faces.size());

  for (int i = 0; i < facedata.size(); ++i)
  {
    GeomMeshHelper::Face &f = m.faces[i];
    VolIntFaceData &fd = facedata[i];
    /* compute face normal and offset w from first 3 vertices */
    double dx1 = verts[f.v[1]][X] - verts[f.v[0]][X];
    double dy1 = verts[f.v[1]][Y] - verts[f.v[0]][Y];
    double dz1 = verts[f.v[1]][Z] - verts[f.v[0]][Z];
    double dx2 = verts[f.v[2]][X] - verts[f.v[1]][X];
    double dy2 = verts[f.v[2]][Y] - verts[f.v[1]][Y];
    double dz2 = verts[f.v[2]][Z] - verts[f.v[1]][Z];
    double nx = dy1 * dz2 - dy2 * dz1;
    double ny = dz1 * dx2 - dz2 * dx1;
    double nz = dx1 * dy2 - dx2 * dy1;
    double len = sqrt(nx * nx + ny * ny + nz * nz);
    fd.norm[X] = nx / len;
    fd.norm[Y] = ny / len;
    fd.norm[Z] = nz / len;
    fd.w = -fd.norm[X] * verts[f.v[0]][X] - fd.norm[Y] * verts[f.v[0]][Y] - fd.norm[Z] * verts[f.v[0]][Z];
  }
}


static void end_calc()
{
  clear_and_shrink(facedata);
  clear_and_shrink(verts);
}


/*
   ============================================================================
   compute mass properties
   ============================================================================
*/

/* compute various integrations over projection of face */
static void compProjectionIntegrals(GeomMeshHelper &m, int fi)
{
  double a0, a1, da;
  double b0, b1, db;
  double a0_2, a0_3, a0_4, b0_2, b0_3, b0_4;
  double a1_2, a1_3, b1_2, b1_3;
  double C1, Ca, Caa, Caaa, Cb, Cbb, Cbbb;
  double Cab, Kab, Caab, Kaab, Cabb, Kabb;
  int i;

  P1 = Pa = Pb = Paa = Pab = Pbb = Paaa = Paab = Pabb = Pbbb = 0.0;

  GeomMeshHelper::Face &f = m.faces[fi];

  for (i = 0; i < 3; i++)
  {
    a0 = verts[f.v[i]][A];
    b0 = verts[f.v[i]][B];
    a1 = verts[f.v[(i + 1) % 3]][A];
    b1 = verts[f.v[(i + 1) % 3]][B];
    da = a1 - a0;
    db = b1 - b0;
    a0_2 = a0 * a0;
    a0_3 = a0_2 * a0;
    a0_4 = a0_3 * a0;
    b0_2 = b0 * b0;
    b0_3 = b0_2 * b0;
    b0_4 = b0_3 * b0;
    a1_2 = a1 * a1;
    a1_3 = a1_2 * a1;
    b1_2 = b1 * b1;
    b1_3 = b1_2 * b1;

    C1 = a1 + a0;
    Ca = a1 * C1 + a0_2;
    Caa = a1 * Ca + a0_3;
    Caaa = a1 * Caa + a0_4;
    Cb = b1 * (b1 + b0) + b0_2;
    Cbb = b1 * Cb + b0_3;
    Cbbb = b1 * Cbb + b0_4;
    Cab = 3 * a1_2 + 2 * a1 * a0 + a0_2;
    Kab = a1_2 + 2 * a1 * a0 + 3 * a0_2;
    Caab = a0 * Cab + 4 * a1_3;
    Kaab = a1 * Kab + 4 * a0_3;
    Cabb = 4 * b1_3 + 3 * b1_2 * b0 + 2 * b1 * b0_2 + b0_3;
    Kabb = b1_3 + 2 * b1_2 * b0 + 3 * b1 * b0_2 + 4 * b0_3;

    P1 += db * C1;
    Pa += db * Ca;
    Paa += db * Caa;
    Paaa += db * Caaa;
    Pb += da * Cb;
    Pbb += da * Cbb;
    Pbbb += da * Cbbb;
    Pab += db * (b1 * Cab + b0 * Kab);
    Paab += db * (b1 * Caab + b0 * Kaab);
    Pabb += da * (a1 * Cabb + a0 * Kabb);
  }

  P1 /= 2.0;
  Pa /= 6.0;
  Paa /= 12.0;
  Paaa /= 20.0;
  Pb /= -6.0;
  Pbb /= -12.0;
  Pbbb /= -20.0;
  Pab /= 24.0;
  Paab /= 60.0;
  Pabb /= -60.0;
}

static void compFaceIntegrals(GeomMeshHelper &m, int fi)
{
  double *n, w;
  double k1, k2, k3, k4;

  GeomMeshHelper::Face &f = m.faces[fi];
  VolIntFaceData &fd = facedata[fi];

  compProjectionIntegrals(m, fi);

  w = fd.w;
  n = &fd.norm.x;
  k1 = 1 / n[C];
  k2 = k1 * k1;
  k3 = k2 * k1;
  k4 = k3 * k1;

  Fa = k1 * Pa;
  Fb = k1 * Pb;
  Fc = -k2 * (n[A] * Pa + n[B] * Pb + w * P1);

  Faa = k1 * Paa;
  Fbb = k1 * Pbb;
  Fcc = k3 * (sqr(n[A]) * Paa + 2 * n[A] * n[B] * Pab + sqr(n[B]) * Pbb + w * (2 * (n[A] * Pa + n[B] * Pb) + w * P1));

  Faaa = k1 * Paaa;
  Fbbb = k1 * Pbbb;
  Fccc = -k4 * (CUBE(n[A]) * Paaa + 3 * sqr(n[A]) * n[B] * Paab + 3 * n[A] * sqr(n[B]) * Pabb + CUBE(n[B]) * Pbbb +
                 3 * w * (sqr(n[A]) * Paa + 2 * n[A] * n[B] * Pab + sqr(n[B]) * Pbb) + w * w * (3 * (n[A] * Pa + n[B] * Pb) + w * P1));

  Faab = k1 * Paab;
  Fbbc = -k2 * (n[A] * Pabb + n[B] * Pbbb + w * Pbb);
  Fcca = k3 * (sqr(n[A]) * Paaa + 2 * n[A] * n[B] * Paab + sqr(n[B]) * Pabb + w * (2 * (n[A] * Paa + n[B] * Pab) + w * Pa));
}

static void compVolumeIntegrals(GeomMeshHelper &m)
{
  double nx, ny, nz;
  int i;

  T0 = T1[X] = T1[Y] = T1[Z] = T2[X] = T2[Y] = T2[Z] = TP[X] = TP[Y] = TP[Z] = 0;

  for (i = 0; i < m.faces.size(); i++)
  {

    GeomMeshHelper::Face &f = m.faces[i];
    VolIntFaceData &fd = facedata[i];

    nx = fabs(fd.norm[X]);
    ny = fabs(fd.norm[Y]);
    nz = fabs(fd.norm[Z]);
    if (nx > ny && nx > nz)
      C = X;
    else
      C = (ny > nz) ? Y : Z;
    A = (C + 1) % 3;
    B = (A + 1) % 3;

    compFaceIntegrals(m, i);

    T0 += fd.norm[X] * ((A == X) ? Fa : ((B == X) ? Fb : Fc));

    T1[A] += fd.norm[A] * Faa;
    T1[B] += fd.norm[B] * Fbb;
    T1[C] += fd.norm[C] * Fcc;
    T2[A] += fd.norm[A] * Faaa;
    T2[B] += fd.norm[B] * Fbbb;
    T2[C] += fd.norm[C] * Fccc;
    TP[A] += fd.norm[A] * Faab;
    TP[B] += fd.norm[B] * Fbbc;
    TP[C] += fd.norm[C] * Fcca;
  }

  T1[X] /= 2;
  T1[Y] /= 2;
  T1[Z] /= 2;
  T2[X] /= 3;
  T2[Y] /= 3;
  T2[Z] /= 3;
  TP[X] /= 2;
  TP[Y] /= 2;
  TP[Z] /= 2;
}


/*
   ============================================================================
   main
   ============================================================================
*/


void GeomMeshHelper::calcMomj(Matrix3 &momj, Point3 &cmpos, real &volume, const TMatrix &tm)
{
  double r[3];    /* center of mass */
  double J[3][3]; /* inertia tensor */

  start_calc(*this, tm);

  compVolumeIntegrals(*this);

  double M, d = 1;

  M = T0;
  volume = T0;

  /* compute center of mass */
  r[X] = T1[X] / T0;
  r[Y] = T1[Y] / T0;
  r[Z] = T1[Z] / T0;

  /* compute inertia tensor */
  J[X][X] = d * (T2[Y] + T2[Z]);
  J[Y][Y] = d * (T2[Z] + T2[X]);
  J[Z][Z] = d * (T2[X] + T2[Y]);
  J[X][Y] = J[Y][X] = -d * TP[X];
  J[Y][Z] = J[Z][Y] = -d * TP[Y];
  J[Z][X] = J[X][Z] = -d * TP[Z];

  /* translate inertia tensor to center of mass */
  J[X][X] -= M * (r[Y] * r[Y] + r[Z] * r[Z]);
  J[Y][Y] -= M * (r[Z] * r[Z] + r[X] * r[X]);
  J[Z][Z] -= M * (r[X] * r[X] + r[Y] * r[Y]);
  J[X][Y] = J[Y][X] += M * r[X] * r[Y];
  J[Y][Z] = J[Z][Y] += M * r[Y] * r[Z];
  J[Z][X] = J[X][Z] += M * r[Z] * r[X];

  momj[0][0] = J[0][0];
  momj[1][1] = J[1][1];
  momj[2][2] = J[2][2];
  momj[0][1] = momj[1][0] = J[0][1];
  momj[0][2] = momj[2][0] = J[0][2];
  momj[1][2] = momj[2][1] = J[1][2];

  cmpos.x = r[X];
  cmpos.y = r[Y];
  cmpos.z = r[Z];

  end_calc();
}

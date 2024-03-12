/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#include "matrix.h"

Matrix::Matrix(const Quaternion &q)
{
  m[0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
  m[1] = 2 * q.x * q.y - 2 * q.s * q.z;
  m[2] = 2 * q.x * q.z + 2 * q.s * q.y;
  m[3] = 0;
  m[4] = 2 * q.x * q.y + 2 * q.s * q.z;
  m[5] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
  m[6] = 2 * q.y * q.z - 2 * q.s * q.x;
  m[7] = 0;
  m[8] = 2 * q.x * q.z - 2 * q.s * q.y;
  m[9] = 2 * q.y * q.z + 2 * q.s * q.x;
  m[10] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
  m[11] = 0;
  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}


void Matrix::loadIdentity()
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] = ((i & 3) == (i >> 2));
  }
}

void Matrix::loadRotateX(const float angle)
{
  float cosA = cosf(angle), sinA = sinf(angle);

  m[0] = 1;
  m[1] = 0;
  m[2] = 0;
  m[3] = 0;

  m[4] = 0;
  m[5] = cosA;
  m[6] = sinA;
  m[7] = 0;

  m[8] = 0;
  m[9] = -sinA;
  m[10] = cosA;
  m[11] = 0;

  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

void Matrix::loadRotateY(const float angle)
{
  float cosA = cosf(angle), sinA = sinf(angle);

  m[0] = cosA;
  m[1] = 0;
  m[2] = sinA;
  m[3] = 0;

  m[4] = 0;
  m[5] = 1;
  m[6] = 0;
  m[7] = 0;

  m[8] = -sinA;
  m[9] = 0;
  m[10] = cosA;
  m[11] = 0;

  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

void Matrix::loadRotateZ(const float angle)
{
  float cosA = cosf(angle), sinA = sinf(angle);

  m[0] = cosA;
  m[1] = sinA;
  m[2] = 0;
  m[3] = 0;

  m[4] = -sinA;
  m[5] = cosA;
  m[6] = 0;
  m[7] = 0;

  m[8] = 0;
  m[9] = 0;
  m[10] = 1;
  m[11] = 0;

  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

void Matrix::loadRotateXY(const float angleX, const float angleY)
{
  float cosX = cosf(angleX), sinX = sinf(angleX), cosY = cosf(angleY), sinY = sinf(angleY);

  m[0] = cosY;
  m[1] = -sinX * sinY;
  m[2] = cosX * sinY;
  m[3] = 0;

  m[4] = 0;
  m[5] = cosX;
  m[6] = sinX;
  m[7] = 0;

  m[8] = -sinY;
  m[9] = -sinX * cosY;
  m[10] = cosX * cosY;
  m[11] = 0;

  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

void Matrix::loadRotateZXY(const float angleX, const float angleY, const float angleZ)
{
  float cosX = cosf(angleX), sinX = sinf(angleX), cosY = cosf(angleY), sinY = sinf(angleY), cosZ = cosf(angleZ), sinZ = sinf(angleZ);

  m[0] = cosY * cosZ + sinX * sinY * sinZ;
  m[1] = cosY * sinZ - sinX * sinY * cosZ;
  m[2] = cosX * sinY;
  m[3] = 0;

  m[4] = -cosX * sinZ;
  m[5] = cosX * cosZ;
  m[6] = sinX;
  m[7] = 0;

  m[8] = sinX * cosY * sinZ - sinY * cosZ;
  m[9] = -sinY * sinZ - sinX * cosY * cosZ;
  m[10] = cosX * cosY;
  m[11] = 0;

  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

void Matrix::loadRotate(const Vertex &v, const float angle)
{
  float cosA = cosf(angle), sinA = sinf(angle);

  m[0] = v.x * v.x * (1 - cosA) + cosA;
  m[1] = v.x * v.y * (cosA - 1) + v.z * sinA;
  m[2] = v.x * v.z * (1 - cosA) + v.y * sinA;
  m[3] = 0;
  m[4] = v.x * v.y * (cosA - 1) - v.z * sinA;
  m[5] = v.y * v.y * (1 - cosA) + cosA;
  m[6] = v.y * v.z * (cosA - 1) + v.x * sinA;
  m[7] = 0;
  m[8] = v.x * v.z * (1 - cosA) - v.y * sinA;
  m[9] = v.y * v.z * (cosA - 1) - v.x * sinA;
  m[10] = v.z * v.z * (1 - cosA) + cosA;
  m[11] = 0;
  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

void Matrix::loadProjectionMatrix(float fov, float aspect, float zNear, float zFar)
{
  float w, h;


  w = tanf(0.5f * fov);
  h = w * aspect;

  m[0] = 1.0f / w;
  m[1] = 0;
  m[2] = 0;
  m[3] = 0;

  m[4] = 0;
  m[5] = 1.0f / h;
  m[6] = 0;
  m[7] = 0;

  m[8] = 0;
  m[9] = 0;
  m[10] = (zFar + zNear) / (zFar - zNear);
  m[11] = 1;

  m[12] = 0;
  m[13] = 0;
  m[14] = -(2 * zFar * zNear) / (zFar - zNear);
  m[15] = 0;
}

void Matrix::loadShadowMatrix(const Plane &plane, const Vertex &lightPos)
{
  float dist = plane.distance(lightPos);

  m[0] = dist - lightPos.x * plane.normal.x;
  m[4] = -lightPos.x * plane.normal.y;
  m[8] = -lightPos.x * plane.normal.z;
  m[12] = -lightPos.x * plane.offset;

  m[1] = -lightPos.y * plane.normal.x;
  m[5] = dist - lightPos.y * plane.normal.y;
  m[9] = -lightPos.y * plane.normal.z;
  m[13] = -lightPos.y * plane.offset;

  m[2] = -lightPos.z * plane.normal.x;
  m[6] = -lightPos.z * plane.normal.y;
  m[10] = dist - lightPos.z * plane.normal.z;
  m[14] = -lightPos.z * plane.offset;

  m[3] = -plane.normal.x;
  m[7] = -plane.normal.y;
  m[11] = -plane.normal.z;
  m[15] = dist - plane.offset;
}

void Matrix::loadCubemapMatrix(const unsigned int side)
{
  switch (side)
  {
    case POSITIVE_X:
      m[0] = 0;
      m[4] = 0;
      m[8] = -1;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = 1;
      m[6] = 0;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case NEGATIVE_X:
      m[0] = 0;
      m[4] = 0;
      m[8] = 1;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = -1;
      m[6] = 0;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case POSITIVE_Y:
      m[0] = 1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 0;
      m[9] = -1;
      m[13] = 0;
      m[2] = 0;
      m[6] = 1;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case NEGATIVE_Y:
      m[0] = 1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 0;
      m[9] = 1;
      m[13] = 0;
      m[2] = 0;
      m[6] = -1;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case POSITIVE_Z:
      m[0] = 1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = 0;
      m[6] = 0;
      m[10] = 1;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case NEGATIVE_Z:
      m[0] = -1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = 0;
      m[6] = 0;
      m[10] = -1;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
  };
}

void Matrix::loadCubemapMatrixOGL(const unsigned int side)
{
  switch (side)
  {
    case POSITIVE_X:
      m[0] = 0;
      m[4] = 0;
      m[8] = 1;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = -1;
      m[6] = 0;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case NEGATIVE_X:
      m[0] = 0;
      m[4] = 0;
      m[8] = -1;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = 1;
      m[6] = 0;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case POSITIVE_Y:
      m[0] = -1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 0;
      m[9] = -1;
      m[13] = 0;
      m[2] = 0;
      m[6] = -1;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case NEGATIVE_Y:
      m[0] = -1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 0;
      m[9] = 1;
      m[13] = 0;
      m[2] = 0;
      m[6] = 1;
      m[10] = 0;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case POSITIVE_Z:
      m[0] = -1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = 0;
      m[6] = 0;
      m[10] = -1;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
    case NEGATIVE_Z:
      m[0] = 1;
      m[4] = 0;
      m[8] = 0;
      m[12] = 0;
      m[1] = 0;
      m[5] = 1;
      m[9] = 0;
      m[13] = 0;
      m[2] = 0;
      m[6] = 0;
      m[10] = 1;
      m[14] = 0;
      m[3] = 0;
      m[7] = 0;
      m[11] = 0;
      m[15] = 1;
      break;
  }
}

void Matrix::translate(const Vertex &v)
{
  m[12] += (v.x * m[0] + v.y * m[4] + v.z * m[8]);
  m[13] += (v.x * m[1] + v.y * m[5] + v.z * m[9]);
  m[14] += (v.x * m[2] + v.y * m[6] + v.z * m[10]);
  m[15] += (v.x * m[3] + v.y * m[7] + v.z * m[11]);
}


void Matrix::scale(const float sx, const float sy, const float sz)
{
  m[0] *= sx;
  m[1] *= sx;
  m[2] *= sx;
  m[3] *= sx;
  m[4] *= sy;
  m[5] *= sy;
  m[6] *= sy;
  m[7] *= sy;
  m[8] *= sz;
  m[9] *= sz;
  m[10] *= sz;
  m[11] *= sz;
}

void Matrix::rotateX(const float angle)
{
  Matrix temp;

  temp.loadRotateX(angle);
  (*this) *= temp;
}

void Matrix::rotateY(const float angle)
{
  Matrix temp;

  temp.loadRotateY(angle);
  (*this) *= temp;
}

void Matrix::rotateZ(const float angle)
{
  Matrix temp;

  temp.loadRotateZ(angle);
  (*this) *= temp;
}

void Matrix::rotateXY(const float angleX, const float angleY)
{
  Matrix temp;

  temp.loadRotateXY(angleX, angleY);
  (*this) *= temp;
}

void Matrix::rotateZXY(const float angleX, const float angleY, const float angleZ)
{
  Matrix temp;

  temp.loadRotateZXY(angleX, angleY, angleZ);
  (*this) *= temp;
}

void Matrix::rotate(const Vertex &v, const float angle)
{
  Matrix temp;

  temp.loadRotate(v, angle);
  (*this) *= temp;
}

void Matrix::transpose()
{
  float temp;

  temp = m[4];
  m[4] = m[1];
  m[1] = temp;

  temp = m[8];
  m[8] = m[2];
  m[2] = temp;

  temp = m[12];
  m[12] = m[3];
  m[3] = temp;

  temp = m[9];
  m[9] = m[6];
  m[6] = temp;

  temp = m[13];
  m[13] = m[7];
  m[7] = temp;

  temp = m[14];
  m[14] = m[11];
  m[11] = temp;
}


Matrix operator+(const Matrix &u, const Matrix &v)
{
  Matrix mat;
  for (unsigned int i = 0; i < 16; i++)
  {
    mat.m[i] = u.m[i] + v.m[i];
  }
  return mat;
}

Matrix operator-(const Matrix &u, const Matrix &v)
{
  Matrix mat;
  for (unsigned int i = 0; i < 16; i++)
  {
    mat.m[i] = u.m[i] - v.m[i];
  }
  return mat;
}

Matrix operator-(const Matrix &v)
{
  Matrix mat;
  for (unsigned int i = 0; i < 16; i++)
  {
    mat.m[i] = -v.m[i];
  }
  return mat;
}

void Matrix::operator+=(const Matrix &v)
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] += v.m[i];
  }
}

void Matrix::operator-=(const Matrix &v)
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] -= v.m[i];
  }
}


void Matrix::operator+=(const float term)
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] += term;
  }
}

void Matrix::operator-=(const float term)
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] -= term;
  }
}

void Matrix::operator*=(const float scalar)
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] *= scalar;
  }
}

void Matrix::operator/=(const float dividend)
{
  for (unsigned int i = 0; i < 16; i++)
  {
    m[i] /= dividend;
  }
}

Matrix operator*(const Matrix &u, const Matrix &v)
{
  Matrix mat;
  unsigned int i, us, vs;

  for (i = 0; i < 16; i++)
  {
    us = (i & 0x3);
    vs = (i & 0xC);

    mat.m[i] = u.m[us] * v.m[vs] + u.m[us + 4] * v.m[vs + 1] + u.m[us + 8] * v.m[vs + 2] + u.m[us + 12] * v.m[vs + 3];
  }
  return mat;
}

void Matrix::operator*=(const Matrix &v)
{
  unsigned int i, us, vs;
  float nm[16];

  for (i = 0; i < 16; i++)
  {
    us = (i & 0x3);
    vs = (i & 0xC);

    nm[i] = m[us] * v.m[vs] + m[us + 4] * v.m[vs + 1] + m[us + 8] * v.m[vs + 2] + m[us + 12] * v.m[vs + 3];
  }
  for (i = 0; i < 16; i++)
  {
    m[i] = nm[i];
  }
}

Vertex operator*(const Matrix &mat, const Vertex &v)
{
  return Vertex(mat.m[0] * v.x + mat.m[4] * v.y + mat.m[8] * v.z, mat.m[1] * v.x + mat.m[5] * v.y + mat.m[9] * v.z,
    mat.m[2] * v.x + mat.m[6] * v.y + mat.m[10] * v.z);
}


Matrix operator+(const Matrix &v, const float term)
{
  Matrix mat;
  unsigned int i;

  for (i = 0; i < 16; i++)
  {
    mat.m[i] = v.m[i] + term;
  }
  return mat;
}

Matrix operator-(const Matrix &v, const float term)
{
  Matrix mat;
  unsigned int i;

  for (i = 0; i < 16; i++)
  {
    mat.m[i] = v.m[i] - term;
  }
  return mat;
}

Matrix operator*(const float scalar, const Matrix &v)
{
  Matrix mat;
  unsigned int i;

  for (i = 0; i < 16; i++)
  {
    mat.m[i] = v.m[i] * scalar;
  }
  return mat;
}

Matrix operator*(const Matrix &v, const float scalar)
{
  Matrix mat;
  unsigned int i;

  for (i = 0; i < 16; i++)
  {
    mat.m[i] = v.m[i] * scalar;
  }
  return mat;
}

Matrix operator/(const Matrix &v, const float dividend)
{
  Matrix mat;
  unsigned int i;

  for (i = 0; i < 16; i++)
  {
    mat.m[i] = v.m[i] / dividend;
  }
  return mat;
}

Matrix operator!(const Matrix &v)
{
  Matrix mat;
  unsigned int i, j, k;
  float temp, newMat[4][8];

  for (k = i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      newMat[j][i] = v.m[k];
      newMat[i][j + 4] = (i == j);
      k++;
    }
  }


  for (i = 0; i < 4; i++)
  {
    // Flip rows if neccesary. It doesn't take care of uninvertible matrixes.
    if (newMat[i][i] == 0)
    {
      j = 0;
      do
      {
        j++;
      } while (newMat[j][i] == 0);

      for (k = i; k < 8; k++)
      {
        temp = newMat[j][k];
        newMat[j][k] = newMat[i][k];
        newMat[i][k] = temp;
      }
    }

    // Shorten the numbers the current row to get a 1 at (i,i) ... the actual 1 is uninteresting and omitted
    for (j = i + 1; j < 8; j++)
    {
      newMat[i][j] /= newMat[i][i];
    }

    // Get zeros in the beginning ... the actual zeros are uninteresting and omitted
    for (j = 0; j < 4; j++)
    {
      if (j != i)
      {
        for (k = i + 1; k < 8; k++)
        {
          newMat[j][k] -= newMat[j][i] * newMat[i][k];
        }
      }
    }
  }

  // Pack it into the return Matrix object.
  for (k = 0, i = 4; i < 8; i++)
  {
    for (j = 0; j < 4; j++)
    {
      mat.m[k] = newMat[j][i];
      k++;
    }
  }

  return mat;
}

Matrix transpose(const Matrix &u)
{
  Matrix m;
  m.m[0] = u.m[0];
  m.m[1] = u.m[4];
  m.m[2] = u.m[8];
  m.m[3] = u.m[12];
  m.m[4] = u.m[1];
  m.m[5] = u.m[5];
  m.m[6] = u.m[9];
  m.m[7] = u.m[13];
  m.m[8] = u.m[2];
  m.m[9] = u.m[6];
  m.m[10] = u.m[10];
  m.m[11] = u.m[14];
  m.m[12] = u.m[3];
  m.m[13] = u.m[7];
  m.m[14] = u.m[11];
  m.m[15] = u.m[15];

  return m;
}

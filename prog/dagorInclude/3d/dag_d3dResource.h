//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
 * @file
 * @brief This file provides an abstract class for DirectX 3D resource.
 */
#pragma once

#include <util/dag_string.h>

/**
 * @brief Abstract base class for DirectX 3D resources.
 */
class D3dResource
{
public:

  /**
   * @brief Destroys the resource and frees the memory allocated to it. 
   */
  virtual void destroy() = 0;

  /**
   * @brief Returns the resource type.
   * 
   * @return One of the following: \c RES3D_TEX, \c RES3D_CUBETEX, \c RES3D_VOLTEX, \c RES3D_ARRTEX, \c RES3D_CUBEARRTEX, \c RES3D_SBUF.
  */
  virtual int restype() const = 0;

  /**
  * @brief Returns the resource size.
  * 
  * @return Size of the resource.
  */
  virtual int ressize() const = 0;

  /**
   * @brief Returns the resource name.
   *
   * @return Name of the resource.
   */
  const char *getResName() const { return statName.c_str(); }

  /**
   * @brief Sets the name of the resource.
   *
   * @param [in] name Name to set for the resource.
   */
  void setResName(const char *name) { statName = name; }

  /**
   * @brief Sets the API-specific name of the resource.
   *
   * @warning This method might allocate memory. Avoid calling it frequently.
   *
   * @param [in] name API-specific name to set for the resource.
   */
  virtual void setResApiName(const char * /*name*/) const {}

protected:

  /**
   * @brief Destructor
   */
  virtual ~D3dResource(){};

private:
  /**
  * @brief Name of the resource.
  */
  String statName;
};

enum
{
  RES3D_TEX,        /**< 3D texture resource type. */
  RES3D_CUBETEX,    /**< Cube texture resource type. */
  RES3D_VOLTEX,     /**< Volume texture resource type. */
  RES3D_ARRTEX,     /**< Texture array resource type. */
  RES3D_CUBEARRTEX, /**< Cube texture array resource type. */
  RES3D_SBUF        /**< Shader buffer resource type. */
};

/**
 * @brief Destroys a D3dResource object and frees the memory allocated to it.
 *
 * @param [in] res A pointer to the resource to destroy.
 * 
 * If \b res is NULL, the function does nothing.
 */
inline void destroy_d3dres(D3dResource *res) { return res ? res->destroy() : (void)0; }

/**
 * @brief Deletes a D3dResource object and frees the memory allocated to it.
 *
 * @tparam T The type of the resource.
 * 
 * @param [in, out] p A pointer to the D3dResource object to delete.
 * 
 * Sets \b res to NULL after the deletion. If \b res is NULL, the function does nothing.
 */
template <class T>
inline void del_d3dres(T *&p)
{
  destroy_d3dres(p);
  p = nullptr;
}

//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
* @file
* @brief This file contains an interface class for creating BaseTexture objects.
*/
#pragma once

// forward declarations for external classes
class BaseTexture;
struct TextureMetaData;

/**
 * @brief Factory interface for creating BaseTexture objects.
 */
class ICreateTexFactory
{
public:
  /**
   * @brief Virtual factory method for creating BaseTexture objects.
   *
   * @param [in] fn		The name of the texture file.
   * @param [in] flg	Flags for texture creation. See \ref dag_texFlags.h
   * @param [in] levels The number of mipmap levels.
   * @param [in] fn_ext Extension of the texture file.
   * @param [in] tmd	Metadata of the texture.
   *
   * @return			A pointer to the created BaseTexture object.
   */
  virtual BaseTexture *createTex(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd) = 0;
};

/**
 * @brief Adds an ICreateTexFactory object to the factory list.
 *
 * @param [in] ctf A pointer to the ICreateTexFactory object to add.
 */
void add_create_tex_factory(ICreateTexFactory *ctf);

/**
 * @brief Removes an ICreateTexFactory object from the factory list.
 *
 * @param ctf A pointer to the ICreateTexFactory object to remove.
 */
void del_create_tex_factory(ICreateTexFactory *ctf);

/**
 * @brief Creates a BaseTexture object from a file using registered factories.
 *
 * @param [in] fn			The name of the texture file.
 * @param [in] flg			Flags for texture creation.
 * @param [in] levels		The number of mipmap levels.
 * @param [in] fatal_on_err Flag indicating whether to throw fatal error on creation failure.
 * @param [in] fnext		Extension of the texture file.
 *
 * @return					A pointer to the created BaseTexture object.
 */
BaseTexture *create_texture(const char *fn, int flg, int levels, bool fatal_on_err, const char *fnext = NULL);

/**
 * @brief Creates a BaseTexture object from a file using registered factories, excluding a specific factory.
 *
 * @param [in] fn			The name of the texture file.
 * @param [in] flg			Flags for texture creation.
 * @param [in] levels		The number of mipmap levels.
 * @param [in] fn_ext		Extension of the texture file.
 * @param [in] tmd			Metadata of the texture.
 * @param [in] excl_factory The factory to exclude from the creation process.
 * @return					A pointer to the created BaseTexture object.
 * 
 * \b excl_factory is typically set to NULL to avoid exclusion or to \c this, 
 * when called from an ICreatetextFactory method to delegate creation to any other factory.
 */
BaseTexture *create_texture_via_factories(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd,
  ICreateTexFactory *excl_factory);

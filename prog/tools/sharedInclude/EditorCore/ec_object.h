// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_decl.h>

#include <math/dag_math3d.h>
#include <util/dag_string.h>
#include <generic/dag_DObject.h>


/// Helper macro for RTTI implementation.
/// static DClassID @b getStaticClassId() - Returns class ID\n
/// virtual bool @b isSubOf(DClassID cid) - Returns @b true if tested object
///     is of specified class or is derived from specified class,\n
///     @b false in other case\n
/// virtual DClassID @b getClassId () - Returns class ID
/// @ingroup EditorCore
#define EO_IMPLEMENT_RTTI_EX(id, BASECLS)        \
  static DClassID getStaticClassId()             \
  {                                              \
    return id;                                   \
  }                                              \
  virtual bool isSubOf(DClassID cid)             \
  {                                              \
    return cid == (id) || BASECLS::isSubOf(cid); \
  }                                              \
  virtual DClassID getClassId()                  \
  {                                              \
    return id;                                   \
  }


/// Base class for Editor objects.
/// Most of objects used in EditorCore based editors are derived
/// from EditableObject.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup EditableObject
class EditableObject : public DObject
{
public:
  /// Constructor 1.
  inline EditableObject() : matrix(TMatrix::IDENT) {}
  /// Constructor 2.
  inline EditableObject(const EditableObject &from) : name(from.name), matrix(from.matrix) {}
  /// Destructor.
  virtual ~EditableObject() {}

  /// @name RTTI implementation.
  //@{
  static DClassID getStaticClassId() { return 0; }
  virtual bool isSubOf(DClassID cid) { return cid == getStaticClassId(); }
  virtual DClassID getClassId() { return getStaticClassId(); }
  //@}

  /// Get object name.
  /// @return pointer to object name
  virtual const char *getName() const { return (const char *)name; }

  /// Get object position.
  /// @return object position
  virtual Point3 getPos() const { return matrix.getcol(3); }

  /// Get object matrix.
  /// @return object tmatrix
  virtual TMatrix getTm() const { return matrix; }

  /// Set object name.
  /// @param[in] nm - pointer to string with object name
  /// @return @b true if operation successful, @b false in other case
  virtual bool setName(const char *nm) = 0;

  /// Set object position.
  /// @param[in] p - object position
  /// @return @b true if operation successful, @b false in other case
  virtual bool setPos(const Point3 &p) = 0;

  /// Test whether mouse cursor is on the object.
  /// @param[in] vp - pointer to viewport where test is performed
  /// @param[in] x, y - <b>x,y</b> mouse cursor coordinates
  /// @return @b true if mouse cursor is on the object, @b false in other case
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const = 0;

protected:
  String name;
  TMatrix matrix;
};


// Helper template to cast EditableObject to derived class.
template <class T>
T *RTTI_cast(EditableObject *source)
{
  if (!source || !source->isSubOf(T::getStaticClassId()))
    return NULL;
  return (T *)source;
}

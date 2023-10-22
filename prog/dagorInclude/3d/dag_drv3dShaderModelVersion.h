//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace d3d
{
//! This namespace hold all shader model version related types and utilities.
namespace shadermodel
{
//! This version type compares always true to any other shader model version.
struct AnyVersion
{};
//! This namespace holds all match / mapping utility functionality.
namespace matcher
{
//! Simple equal comparison compare type.
//! \tparam T Type of the compared type.
template <typename T>
struct EqualCompare
{
  //! Compares \p l and \p r and returns true when they are equal.
  //! \param l First value to compare.
  //! \param r Second value to compare.
  //! \returns Result of \p l == \p r.
  template <typename OT>
  static bool compare(const T &l, const OT &r)
  {
    return l == r;
  }
};
//! Basic map type, that allows mapping of matchers to values.
//! \tparam V Type of the value that is matched with the input matchers on each matching attempt.
//! \tparam T Type of the stored value.
//! \tparam finalMapType Type of the terminating mapping. This acts as a default and returns the value instead of a map.
//! \tparam Cmp Comparison type that should be used to determine if anything was matched.
template <typename V, typename T, typename FinalMapType, typename Cmp = EqualCompare<V>>
class BasicMap : protected Cmp
{
  //! Value to compare to determine if a match was found or not
  V value{};
  //! Keeps track if anything was matched.
  bool wasMatched = false;
  //! Raw bytes to store T
  alignas(alignof(T)) unsigned char memory[sizeof(T)]; // -V730
  /**
   * Helper to retrieve the stored instance of \p T.
   * \returns Reference to the store instance of \p T.
   */
  T &object() { return *(T *)&memory[0]; }
  /**
   * \brief Conditional move constructor helper.
   * \details A new instance of \p T is going to be instantiated when \p should_construct is true and the internal state indicates that
   * there is no instance stored yet. In this case \p v will be moved into the parameter of the move constructor of \p T for the
   * new instance.
   * \param should_construct Indicates if a instance of \p T should be instantiated or not.
   * \param v May be used as a input for the move constructor of \p T.
   */
  void cMove(bool should_construct, T &&v)
  {
    if (!should_construct)
      return;
    if (wasMatched)
      return;
    ::new (&object()) T{(T &&) v};
    wasMatched = true;
  }
  /**
   * \brief Conditional constructor helper.
   * \details A new instance of \p T is going to be instantiated when \p should_construct is true and the internal state indicates that
   * there is no instance stored yet. In this case \p generator will be invoked to generate the input of the constructor of \p T for
   * the new instance.
   * \param should_construct Indicates if a instance of \p T should be instantiated or not.
   * \param generator When invoked it should generate valid input for the constructor of \p T.
   */
  template <typename C>
  void cContruct(bool should_construct, C &&generator)
  {
    if (!should_construct)
      return;
    if (wasMatched)
      return;
    ::new (&object()) T{generator()};
    wasMatched = true;
  }
  /**
   * Conditional emplace constructor helper.
   * \details A new instance of \p T is going to be instantiated when \p should_construct is true and the internal state indicates that
   * there is no instance stored yet. In this case \p vs will be moved into the parameter of the constructor of \p T for the new
   * instance.
   * \param should_construct Indicates if a instance of \p T should be instantiated or not.
   * \param vs May be used as a input for the constructor of \p T.
   */
  template <typename... Vs>
  void cEmplace(bool should_construct, Vs &&...vs)
  {
    if (!should_construct)
      return;
    if (wasMatched)
      return;
    ::new (&object()) T{(Vs &&) vs...};
    wasMatched = true;
  }

public:
  //! Basic default constructor
  BasicMap() = default;
  //! Copy is not allowed as we don't want users to copy maps around.
  BasicMap(const BasicMap &) = delete;
  //! Basic move constructor that moves internal state from \p other to this.
  //! \param other Other instance that is used to initialize this, should a value be stored, it is moved to this.
  BasicMap(BasicMap &&other) : value{(V &&) other.value}, wasMatched{other.wasMatched} // -V730
  {
    if (wasMatched)
    {
      ::new (&object()) T{(T &&) other.object()};
    }
  }
  //! Copy is not allowed as we don't want users to copy maps around.
  BasicMap &operator=(const BasicMap &) = delete;
  //! Move is not allowed as we don't want users to move maps around.
  BasicMap &operator=(BasicMap &&) = delete;
  //! Constructor that initializes the compare value state.
  //! \param v Value that should be used with matchers.
  BasicMap(const V &v) : value{v} {} // -V730
  //! Constructor that initializes the compare value state and does one map operation.
  //! \param v Value that should be used with matchers.
  //! \param cmp Matcher that should be compared to the compare value.
  //! \param t Value that is stored should the compare of the matcher to the value yield true.
  template <typename OV>
  BasicMap(const V &v, const OV &cmp, T &&t) : value{v} // -V730
  {
    cMove(this->compare(value, cmp), (T &&) t);
  }
  //! Constructor that initializes the compare value state and does one map operation.
  //! \param v Value that should be used with matchers.
  //! \param cmp Matcher that should be compared to the compare value.
  //! \param c A generator that is used to generate input to the constructor of \p T should the compare of the matcher to the value
  //! yield true.
  template <typename OV, typename C>
  BasicMap(const V &v, const OV &cmp, C &&c) : value{v} // -V730
  {
    cContruct(this->compare(value, cmp), (C &&) c);
  }
  //! Constructor that initializes the compare value state and does one map operation.
  //! \param v Value that should be used with matchers.
  //! \param cmp Matcher that should be compared to the compare value.
  //! \param vs Values that should be passed to the constructor of \p T should the compare of the matcher to the value yield true.
  template <typename OV, typename... Vs>
  BasicMap(const V &v, const OV &cmp, Vs &&...vs) : value{v} // -V730
  {
    cEmplace(this->compare(value, cmp), (Vs &&) vs...);
  }
  //! Maps a matcher to a value.
  //! \param cmp Value matcher.
  //! \param t Value that is stored in this if no value is stored in stored.
  //! \returns Reference to this.
  template <typename OV>
  BasicMap &map(const OV &cmp, T &&t)
  {
    cMove(this->compare(value, cmp), (T &&) t);
    return *this;
  }
  //! Maps a matcher to a value.
  //! \param cmp Value matcher.
  //! \param c Generator that generates input passed to the constructor of \p T should no value be stored in this.
  //! \returns Reference to this.
  template <typename OV, typename C>
  BasicMap &map(const OV &cmp, C &&c)
  {
    cContruct(this->compare(value, cmp), (C &&) c);
    return *this;
  }
  //! Maps a matcher to a value.
  //! \param cmp Value matcher.
  //! \param vs Parameters passed to the constructor of \p T should no value be stored in this.
  //! \returns Reference to this.
  template <typename OV, typename... Vs>
  BasicMap &map(const OV &cmp, Vs &&...vs)
  {
    cContruct(this->compare(value, cmp), (Vs &&) vs...);
    return *this;
  }
  //! Maps final type to a value.
  //! \param cmp Final value matcher.
  //! \param t Value that is stored in this if no value is stored in stored.
  //! \returns Stored valued of \p T.
  T &&map(const FinalMapType &cmp, T &&t)
  {
    (void)cmp;
    if (wasMatched)
    {
      return (T &&) object();
    }
    return (T &&) t;
  }
  //! Maps final type to a value.
  //! \param cmp Final value matcher.
  //! \param c Generator that generates input passed to the constructor of \p T should no value be stored in this.
  //! \returns Stored valued of \p T.
  template <typename C>
  T &&map(const FinalMapType &cmp, C &&c)
  {
    (void)cmp;
    cContruct(true, (C &&) c);
    return (T &&) object();
  }
  //! Maps final type to a value.
  //! \param cmp Final value matcher.
  //! \param vs Parameters passed to the constructor of \p T should no value be stored in this.
  //! \returns Stored valued of \p T.
  template <typename... Vs>
  T &&map(const FinalMapType &cmp, Vs &&...vs)
  {
    (void)cmp;
    cEmplace(this->compare(value, cmp), (Vs &&) vs...);
    return (T &&) object();
  }
  //! Gets the stored value.
  //! \param c Receiver of the value, should any value be stored in this.
  //! \returns Returns true if a value was stored in this and this value was moved into \p o.
  bool get(T &o)
  {
    if (wasMatched)
    {
      o = (T &&) object();
    }
    return wasMatched;
  }
  //! Gets the stored value.
  //! \param c Receiver of the value, should any value be stored in this.
  //! \returns Returns true if a value was stored in this and \p c was called.
  template <typename C>
  bool get(C &&c)
  {
    if (wasMatched)
    {
      c((T &&) object());
    }
    return wasMatched;
  }
};
//! Map type that exactly matches values versions.
//! \tparam T Type of the stored value.
//! \tparam V Type of the value that is matched with the input matchers on each matching attempt.
template <typename T, typename V>
class Map : protected BasicMap<V, T, AnyVersion>
{
  //! Type shorthand to reduce typing.
  using BaseType = BasicMap<V, T, AnyVersion>;

public:
  using BaseType::BaseType;
  using BaseType::map;
};
} // namespace matcher
//! Represents a shader model version.
//! \note To add new supported shader models, you need to define a constant of the shader model and
//! add this constant to the shader model list. See definition / uses of sm50 as example. It is also
//! important to keep the order of versions in the list of AllVersionsList, otherwise other algorithms
//! may fail to work properly.
struct Version
{
  //! Major version of the shader model version.
  unsigned int major : 16;
  //! Minor version of the shader model version.
  unsigned int minor : 16;
  //! Default constructor, sets version values to 0.
  constexpr Version() : major{0}, minor{0} {}
  //! Initializes the version with the given values.
  //! \ma Major version that should be stored.
  //! \mi Minor version that should be stored.
  constexpr Version(unsigned int ma, unsigned int mi) : major{ma}, minor{mi} {}
  //! Constructor that stores a representation of AnyVersion, which sets major and minor version the max possible value.
  constexpr Version(const AnyVersion &v) : major{0xFFff}, minor{0xFFff} { (void)v; }
  //! Begins a map from versions over this version to a value.
  //! \param version The matcher of the first value matcher pair.
  //! \param value The value that should be stored when \p version should match.
  //! \returns Map to map versions to values.
  template <typename T>
  matcher::Map<T, Version> map(const Version &version, T &&value) const
  {
    return {*this, version, (T &&) value};
  }
  //! Begins a map from versions over this version to a value.
  //! \param version The matcher of the first value matcher pair.
  //! \param value The value that should be stored when \p version should match.
  //! \returns Map to map versions to values.
  template <unsigned int N>
  matcher::Map<const char *, Version> map(const Version &version, const char (&str)[N]) const
  {
    return {*this, version, str};
  }
  //! Checks if the given version is supported by this version.
  //! \param other Version to check this version against.
  //! \returns Is true when this version is equal or later than \p other.
  constexpr bool supports(Version other) const { return (major > other.major) || ((major == other.major) && (minor >= other.minor)); }
};
//! Represents a shader model version with paired version name string and ps name string.
struct VersionWithName : Version
{
  //! Version name string in the <major>.<minor> format.
  const char *versionName;
  //! Version ps name string in the ps<major><minor> format.
  const char *psName;
};
//! A shader model version type.
//! \tparam MajorVersion Major version of this constant.
//! \tparam MinorVersion Minor version of this constant.
template <unsigned int MajorVersion, unsigned int MinorVersion>
struct VersionConstant
{
  //! Major version member / constant to make access uniform to #Version.
  static constexpr unsigned int major = MajorVersion;
  //! Minor version member / constant to make access uniform to #Version.
  static constexpr unsigned int minor = MinorVersion;
  //! String representation of this version in <major>.<minor> format.
  static constexpr char as_string[4] = {'0' + MajorVersion, '.', '0' + MinorVersion, '\0'};
  //! String representation of this version in ps<major><minor> format.
  static constexpr char as_ps_string[5] = {'p', 's', '0' + MajorVersion, '0' + MinorVersion, '\0'};
  //! Conversion operator to convert to #Version.
  constexpr operator Version() const { return {major, minor}; }
  //! Conversion operator to convert to #VersionWithName.
  operator VersionWithName() const { return {{major, minor}, as_string, as_ps_string}; }
  //! Begins a map from versions over this version to a value.
  //! \param version The matcher of the first value matcher pair.
  //! \param value The value that should be stored when \p version should match.
  //! \returns Map to map versions to values.
  template <typename T>
  matcher::Map<T, Version> map(const Version &version, T &&value) const
  {
    return {*this, version, (T &&) value};
  }
  //! Begins a map from versions over this version to a value.
  //! \param version The matcher of the first value matcher pair.
  //! \param value The value that should be stored when \p version should match.
  //! \returns Map to map versions to values.
  template <unsigned int N>
  matcher::Map<const char *, Version> map(const Version &version, const char (&str)[N]) const
  {
    return {*this, version, str};
  }
  //! Checks if the given version is supported by this version.
  //! \param other Version to check this version against.
  //! \returns Is true when this version is equal or later than \p other.
  constexpr bool supports(Version other) const
  {
    return (major > other.major) || ((major == other.major) && (minor >= other.minor)); // -V560
  }
};
//! Compares two version numbers and returns true when the first one equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l equal to \p r.
constexpr bool operator==(const Version &l, const Version &r) { return l.major == r.major && l.minor == r.minor; }
//! Compares two version numbers and returns true when the first one not equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l not equal to \p r.
constexpr bool operator!=(const Version &l, const Version &r) { return !(l == r); }
//! Compares two version numbers and returns true when the first less than the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l less than \p r.
constexpr bool operator<(const Version &l, const Version &r)
{
  return l.major < r.major || ((l.major == r.major) && l.minor < r.minor);
}
//! Compares two version numbers and returns true when the first one greater the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l greater than \p r.
constexpr bool operator>(const Version &l, const Version &r) { return r < l; }
//! Compares two version numbers and returns true when the first one less or equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l less or equal to \p r.
constexpr bool operator<=(const Version &l, const Version &r) { return l == r || l < r; }
//! Compares two version numbers and returns true when the first greater or equal the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l greater or equal to \p r.
constexpr bool operator>=(const Version &l, const Version &r) { return l == r || l > r; }
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True.
constexpr bool operator==(const AnyVersion &l, const Version &r)
{
  (void)l;
  (void)r;
  return true;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns true.
constexpr bool operator==(const Version &l, const AnyVersion &r)
{
  (void)l;
  (void)r;
  return true;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns False.
constexpr bool operator!=(const AnyVersion &l, const Version &r)
{
  (void)l;
  (void)r;
  return false;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns False.
constexpr bool operator!=(const Version &l, const AnyVersion &r)
{
  (void)l;
  (void)r;
  return false;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True.
constexpr bool operator>(const AnyVersion &l, const Version &r)
{
  (void)l;
  (void)r;
  return true;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns False.
constexpr bool operator>(const Version &l, const AnyVersion &r)
{
  (void)l;
  (void)r;
  return false;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns False.
constexpr bool operator<(const AnyVersion &l, const Version &r)
{
  (void)l;
  (void)r;
  return false;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True.
constexpr bool operator<(const Version &l, const AnyVersion &r)
{
  (void)l;
  (void)r;
  return true;
}
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is larger or equal to \p r.
constexpr bool operator>=(const AnyVersion &l, const Version &r) { return l == r || l > r; }
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is larger or equal to \p r.
constexpr bool operator>=(const Version &l, const AnyVersion &r) { return l == r || l > r; }
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is less or equal to \p r.
constexpr bool operator<=(const AnyVersion &l, const Version &r) { return l == r || l < r; }
//! Compares a version to any version.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is less or equal to \p r.
constexpr bool operator<=(const Version &l, const AnyVersion &r) { return l == r || l < r; }
//! Compares two constant version numbers and returns true when the first one equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l equal to \p r.
template <unsigned int mj1, unsigned int mi1, unsigned int mj2, unsigned int mi2>
constexpr bool operator==(const VersionConstant<mj1, mi1> &l, const VersionConstant<mj2, mi2> &r)
{
  (void)l;
  (void)r;
  return mj1 == mj2 && mi1 == mi2;
}
//! Compares two constant version numbers and returns true when the first one not equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l not equal to \p r.
template <unsigned int mj1, unsigned int mi1, unsigned int mj2, unsigned int mi2>
constexpr bool operator!=(const VersionConstant<mj1, mi1> &l, const VersionConstant<mj2, mi2> &r)
{
  return !(l == r);
}
//! Compares two constant version numbers and returns true when the first one is smaller than the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is smaller than \p r.
template <unsigned int mj1, unsigned int mi1, unsigned int mj2, unsigned int mi2>
constexpr bool operator<(const VersionConstant<mj1, mi1> &l, const VersionConstant<mj2, mi2> &r)
{
  (void)l;
  (void)r;
  return mj1 < mj2 || ((mj1 == mj2) && mi1 < mi2);
}
//! Compares two constant version numbers and returns true when the first one is larger than the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is larger than \p r.
template <unsigned int mj1, unsigned int mi1, unsigned int mj2, unsigned int mi2>
constexpr bool operator>(const VersionConstant<mj1, mi1> &l, const VersionConstant<mj2, mi2> &r)
{
  return r < l;
}
//! Compares two constant version numbers and returns true when the first one is less or equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is less or equal to \p r.
template <unsigned int mj1, unsigned int mi1, unsigned int mj2, unsigned int mi2>
constexpr bool operator<=(const VersionConstant<mj1, mi1> &l, const VersionConstant<mj2, mi2> &r)
{
  return l == r || l < r;
}
//! Compares two constant version numbers and returns true when the first one is larger or equal to the second one.
//! \param l First value to compare.
//! \param r Second value to compare.
//! \returns True when \l is larger or equal to \p r.
template <unsigned int mj1, unsigned int mi1, unsigned int mj2, unsigned int mi2>
constexpr bool operator>=(const VersionConstant<mj1, mi1> &l, const VersionConstant<mj2, mi2> &r)
{
  return l == r || l > r;
}
//! A list type of a set of versions.
//! \tparam List of versions.
template <typename... Versions>
struct VersionList
{
  //! Number of elements in the \p Versions list.
  static constexpr auto size = sizeof...(Versions);
};
//! Finds the latest version in the given version list.
//! \param l Version list to extract the latest version from.
//! \returns The earlier version of \p A or \p B.
template <typename A, typename B>
inline constexpr auto max(VersionList<A, B>)
{
  constexpr A a;
  constexpr B b;
  if constexpr (a > b)
  {
    return a;
  }
  else
  {
    return b;
  }
}
//! Finds the latest version in the given version list.
//! \param l Version list to extract the latest version from.
//! \returns The latest version (and its exact type) in the given list.
template <typename A, typename B, typename C, typename... Versions>
inline constexpr auto max(VersionList<A, B, C, Versions...>)
{
  constexpr A a;
  constexpr auto b = max(VersionList<B, C, Versions...>{});
  if constexpr (a > b)
  {
    return a;
  }
  else
  {
    return b;
  }
}
//! Finds the earliest version in the given version list.
//! \param l Version list to extract the earliest version from.
//! \returns The earlier version of \p A or \p B.
template <typename A, typename B>
inline constexpr auto min(VersionList<A, B> l)
{
  (void)l;
  constexpr A a;
  constexpr B b;
  if constexpr (a < b)
  {
    return a;
  }
  else
  {
    return b;
  }
}
//! Finds the earliest version in the given version list.
//! \param l Version list to extract the earliest version from.
//! \returns The earliest version (and its exact type) in the given list.
template <typename A, typename B, typename C, typename... Versions>
inline constexpr auto min(VersionList<A, B, C, Versions...> l)
{
  (void)l;
  constexpr A a;
  constexpr auto b = min(VersionList<B, C, Versions...>{});
  if constexpr (a < b)
  {
    return a;
  }
  else
  {
    return b;
  }
}
//! Finds the index of the version of \p current in the version list \p l.
//! \param l Version list to search the index for.
//! \param current Version to search of in \p l.
//! \returns Index into \p l representing \p current, if none can found it will return the size of the version list.
template <typename VC, typename... Vs>
inline unsigned int index_of(VersionList<Vs...> l, VC current)
{
  (void)l;
  static const VC valueTable[] = {Vs{}...};
  for (unsigned int i = 0; i < sizeof...(Vs); ++i)
  {
    if (valueTable[i] == current)
    {
      return i;
    }
  }
  return sizeof...(Vs);
}
//! Returns the \p R representation of the type in \p l at the index \p index.
//! \param l Version list type to index into.
//! \param index Index into \p l.
//! \returns When index is in range of the version list, then it will return the \p R representation of that type at that index,
//! otherwise t will return the default value of \p R.
template <typename R, typename... Vs>
inline R at(VersionList<Vs...> l, int index)
{
  (void)l;
  static const R valueTable[] = {Vs{}...};
  return index >= 0 && index < sizeof...(Vs) ? valueTable[index] : R{};
}
//! Version range iterator.
//! \tparam V Value type that the iterator represents.
//! \pparam VersionList List of versions that this iterator iterates over.
template <typename V, typename VersionList>
class VersionRangeIterator
{
  //! Index into the VersionList list of versions.
  int index;

public:
  //! Default constructor, leaves type in uninitialized state.
  constexpr VersionRangeIterator() = default;
  //! Basic copy constructor.
  constexpr VersionRangeIterator(const VersionRangeIterator &) = default;
  //! Constructor which initializes the state with an given index into the list of \p VersionList.
  //! \param idx Index that should be stored in this iterator.
  constexpr VersionRangeIterator(int idx) : index{idx} {}
  //! Dereference operator that will return a value of \p V.
  //! \returns A representation of the value at the \p VersionList at the currently stored index.
  V operator*() const { return at<V>(VersionList{}, index); }
  //! Increment operator, moves the current index to the previous entry of the \p VersionList type.
  //! \returns Reference to an altered this.
  VersionRangeIterator &operator++()
  {
    --index;
    return *this;
  }
  //! Increment operator, moves the current index to the previous entry of the \p VersionList type.
  //! \returns Copy of this advanced by one.
  constexpr VersionRangeIterator operator++(int) const { return {index - 1}; }
  //! Increment operator, moves the current index to the next entry of the \p VersionList type.
  //! \returns Reference to an altered this.
  VersionRangeIterator &operator--()
  {
    ++index;
    return *this;
  }
  //! Increment operator, moves the current index to the previous entry of the \p VersionList type.
  //! \returns Copy of this reversed one.
  constexpr VersionRangeIterator operator--(int) const { return {index + 1}; }
  //! Equal compare operator for to VersionRangeIterator.
  //! \p l First value to compare.
  //! \p r Second value to compare.
  //! \returns Returns true when \p l and \p r do represent the same version value.
  friend constexpr bool operator==(const VersionRangeIterator &l, const VersionRangeIterator &r) { return l.index == r.index; }
  //! Not equal compare operator for to VersionRangeIterator.
  //! \p l First value to compare.
  //! \p r Second value to compare.
  //! \returns Returns true when \p l and \p r do not represent the same version value.
  friend constexpr bool operator!=(const VersionRangeIterator &l, const VersionRangeIterator &r) { return !(l == r); }
};
//! Represents a range of shader model versions that can be iterated over.
template <typename List>
struct VersionRange;
//! Represents a range of shader model versions that can be iterated over.
//! \tparam Versions The list of versions to iterate over.
template <typename... Versions>
struct VersionRange<VersionList<Versions...>>
{
  //! Helper type to cut down repeat code.
  using ListOfVersions = VersionList<Versions...>;
  //! Iterator type of this range.
  using Iterator = VersionRangeIterator<VersionWithName, VersionList<Versions...>>;
  //! Returns the beginning of the version range, as in the most recent version.
  //! \returns Iterator representing the most recent version of this range.
  Iterator begin() const { return {ListOfVersions::size - 1}; }
  //! Returns the beginning of the version range, as in the most recent version.
  //! \returns Iterator representing the most recent version of this range.
  Iterator cbegin() const { return begin(); }
  //! Returns a iterator past the last (oldest) version of this range.
  //! \returns Iterator past the last (oldest) version of this range.
  Iterator end() const { return {-1}; }
  //! Returns a iterator past the last (oldest) version of this range.
  //! \returns Iterator past the last (oldest) version of this range.
  Iterator cend() const { return end(); }
  //! Returns the number of versions in this range.
  //! \returns Number of versions of this range.
  auto size() const { return ListOfVersions::size; }
};
} // namespace shadermodel
//! Shader model constant that represents no shader model.
inline constexpr shadermodel::VersionConstant<0, 0> smNone;
//! Version constant of the 4.0 shader model.
inline constexpr shadermodel::VersionConstant<4, 0> sm40;
//! Version constant of the 4.1 shader model.
inline constexpr shadermodel::VersionConstant<4, 1> sm41;
//! Version constant of the 5.0 shader model.
inline constexpr shadermodel::VersionConstant<5, 0> sm50;
//! Version constant of the 6.0 shader model.
inline constexpr shadermodel::VersionConstant<6, 0> sm60;
//! Version constant of the 6.6 shader model.
inline constexpr shadermodel::VersionConstant<6, 6> sm66;
//! Version constant that will match true to any shader model version.
inline constexpr shadermodel::AnyVersion smAny;
namespace shadermodel
{
//! This type list consists of all supported shader model versions.
//! \warning Listing has to be from earliest to latest, otherwise dependent algorithms may not work properly.
using AllVersionsList = VersionList<decltype(sm40), decltype(sm41), decltype(sm50), decltype(sm60), decltype(sm66)>;
} // namespace shadermodel
//! Latest supported shader model version of the list defined by #shadermodel::AllVersionsList.
inline constexpr auto smMax = max(shadermodel::AllVersionsList{});
//! Earliest supported shader model version of the list defined by #shadermodel::AllVersionsList.
inline constexpr auto smMin = min(shadermodel::AllVersionsList{});
//! This value is a version range over all listed versions, to extend the list, alter the type #d3d::shadermodel::AllVersionsList.
//! This value supports begin / end and can be used as a range for range based for loops.
inline constexpr shadermodel::VersionRange<shadermodel::AllVersionsList> smAll;
//! Returns the "<major>.<minor>" string representation of a Version value that is part of d3d::smAny.
//! \p v Version for which string it should be returned.
//! \returns A valid constant string for versions included in d3d::smAny.
inline const char *as_string(d3d::shadermodel::Version v)
{
  for (auto version : d3d::smAll)
  {
    if (version != v)
      continue;
    return version.versionName;
  }
  return "";
}
//! Returns the "<major>.<minor>" string representation of a VersionWithName value.
//! \p v Version for which string it should be returned.
//! \returns \p versionName member of \p v.
inline const char *as_string(d3d::shadermodel::VersionWithName v) { return v.versionName; }
//! Returns the "ps<major><minor>" string representation of a Version value that is part of d3d::smAny.
//! \p v Version for which string it should be returned.
//! \returns A valid constant string for versions included in d3d::smAny.
inline const char *as_ps_string(d3d::shadermodel::Version v)
{
  for (auto version : d3d::smAll)
  {
    if (version != v)
      continue;
    return version.psName;
  }
  return "";
}
//! Returns the "ps<major><minor>" string representation of a VersionWithName value.
//! \p v Version for which string it should be returned.
//! \returns \p psName member of \p v.
inline const char *as_ps_string(d3d::shadermodel::VersionWithName v) { return v.psName; }
} // namespace d3d
//! User literal operator for \p _sm to generate shader model versions.
//! This is a raw operator, valid input is <digit>.<digit>, any other literals will result in undefined behavior.
//! \p text Literal text to process. Format has to be <digit>.<digit>
//! \returns A valid version value should the input match the format. Otherwise undefined behavior.
constexpr d3d::shadermodel::Version operator""_sm(const char *text)
{
#if 1
  // only works if <digit>.<digit>
  unsigned major = text[0] - '0';
  unsigned minor = text[2] - '0';
#else
  unsigned major = 0;
  unsigned minor = 0;
  unsigned i = 0;
  for (; '\0' != text[i] && '.' != text[i]; ++i)
  {
    major = major * 10 + text[i] - '0';
  }
  if (text[i] != '\0')
  {
    for (++i; '\0' != text[i]; ++i)
    {
      minor = minor * 10 + text[i] - '0';
    }
  }
#endif
  return {major, minor};
}

template <typename T>
struct DebugConverter;
template <>
struct DebugConverter<d3d::shadermodel::VersionWithName>
{
  static const char *getDebugType(const d3d::shadermodel::VersionWithName &version) { return version.versionName; }
};
template <>
struct DebugConverter<d3d::shadermodel::Version>
{
  static unsigned int getDebugType(const d3d::shadermodel::Version &version) { return version.major << 16 | version.minor; }
};
template <unsigned Major, unsigned Minor>
struct DebugConverter<d3d::shadermodel::VersionConstant<Major, Minor>>
{
  static unsigned int getDebugType(const d3d::shadermodel::VersionConstant<Major, Minor> &version)
  {
    return version.major << 16 | version.minor;
  }
};

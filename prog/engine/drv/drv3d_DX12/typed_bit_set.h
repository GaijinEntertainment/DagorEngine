#pragma once

/**
 * @file
 * @brief Defines a typed bitset class template.
 */

#include <EASTL/bitset.h>

/**
 * @brief A typed bit set class template.
 *
 * This class template represents a bitset where each bit corresponds to a value of the specified enumeration type.
 *
 * @tparam T The enumeration type whose values will be used as indices in the bit set. 
 * The enumeration last enumerator must be COUNT showing the number of actual enumerators.
 * @tparam BIT_COUNT The number of bits in the bit set. Default is determined by the number of enumerators in the enumeration type.
 */
template <typename T, size_t BIT_COUNT = static_cast<size_t>(T::COUNT)>
class TypedBitSet : private eastl::bitset<BIT_COUNT>
{
  using BaseType = eastl::bitset<BIT_COUNT>;

public:
  using BaseType::all;
  using BaseType::any;
  using BaseType::count;
  using BaseType::flip;
  using BaseType::none;
  // using BaseType::to_string;
  using BaseType::to_ulong;
  // using BaseType::to_ullong;
  using BaseType::size;
  using typename BaseType::reference;

  /**
   * @brief Default constructor.
   */
  TypedBitSet() = default;

  /**
   * @brief Copy constructor.
   * 
   * @param [in] other A bit set instance to copy from.
   */
  TypedBitSet(const TypedBitSet &) = default;

  /**
   * @brief Destructor.
   */
  ~TypedBitSet() = default;

  /**
   * @brief Copy assignment operator.
   * 
   * @param [in] other  A bit set instance to assign.
   * @return            A reference to the updated bit set.
   */
  TypedBitSet &operator=(const TypedBitSet &) = default;

  /**
   * @brief Accesses the bit at the specified index.
   * 
   * @param [in] index  An enumerator passed as the index of the bit to access.
   * @return            \c true if the bit is set, \c false otherwise.
   */
  bool operator[](T index) const { return (*this)[static_cast<size_t>(index)]; }

  /**
   * @brief Accesses the bit at the specified index.
   * 
   * @param [in] index  An enumerator passed as the index of the bit to access.
   * @return            A reference to the bit.
   */
  typename BaseType::reference operator[](T index) { return (*this)[static_cast<size_t>(index)]; }

  /**
   * @brief Checks if the bit at the specified index is set.
   * 
   * @param [in] index  An enumerator passed as the index of the bit to check.
   * @return            \c true if the bit is set, \c false otherwise.
   */
  bool test(T index) const { return BaseType::test(static_cast<size_t>(index)); }

  /**
   * @brief Sets all bits in the bitset to \c true.
   * 
   * @return A reference to the modified bit set.
   */
  TypedBitSet &set()
  {
    BaseType::set();
    return *this;
  }

  /**
   * @brief Sets the bit at the specified index.
   * 
   * @param [in] index  The index of the bit to set.
   * @param [in] value  The value to set the bit to (default is \c true).
   * @return            A reference to the updated bit set.
   */
  TypedBitSet &set(T index, bool value = true)
  {
    BaseType::set(static_cast<size_t>(index), value);
    return *this;
  }

  /**
   * @brief Sets all bits in the bitset to \c false.
   *
   * @return A reference to the modified bit set.
   */
  TypedBitSet &reset()
  {
    BaseType::reset();
    return *this;
  }

  /**
   * @brief Sets the bit at the specified index to \c false.
   *
   * @param [in] index  The index of the bit to set.
   * @return            A reference to the updated bit set.
   */
  TypedBitSet &reset(T index)
  {
    BaseType::reset(static_cast<size_t>(index));
    return *this;
  }

  /**
   * @brief Performs bitwise NOT operation on the bitset.
   * 
   * @return The copy of the bitset from before the operation.
   */
  TypedBitSet operator~() const
  {
    auto cpy = *this;
    cpy.flip();
    return cpy;
  }

  /**
   * @brief Checks if two bit sets are equal.
   * 
   * @param [in] other  The other bit set instance to compare with.
   * @return            \c true if the two TypedBitSet objects are equal, \c false otherwise.
   */
  bool operator==(const TypedBitSet &other) const { return BaseType::operator==(other); }

    /**
   * @brief Checks if two bit sets are not equal.
   * 
   * @param [in] other  The other bit set instance to compare with.
   * @return            \c true if the two TypedBitSet objects are not equal, \c false otherwise.
   */
  bool operator!=(const TypedBitSet &other) const { return BaseType::operator!=(other); }

  /**
   * @brief Sets multiple bits in the bitset to \c true.
   * 
   * @tparam T0         The enumeration type of the first index to set.
   * @tparam T1         The enumeration type of the second index to set.
   * @tparam ...Ts      Enumerations types of remaining indices.
   * 
   * @param [in] v0     The enumerator passed as the first index to set.
   * @param [in] v1     The enumerator passed as the second index to set.
   * @param [in] ...vs  Enumerators passed as remaining indices.
   * @return            A reference to the updated bit set.
   */
  template <typename T0, typename T1, typename... Ts>
  TypedBitSet &set(T0 v0, T1 v1, Ts... vs)
  {
    set(v0);
    set(v1, vs...);
    return *this;
  }

  /**
   * @brief Sets multiple bits in the bitset to \c false.
   *
   * @tparam T0     The enumeration type of the first index to reset.
   * @tparam T1     The enumeration type of the second index to reset.
   * @tparam ...Ts  Enumerations types of remaining indices.
   *
   * @param [in] v0     The enumerator passed as the first index to reset.
   * @param [in] v1     The enumerator passed as the second index to reset.
   * @param [in] ...vs  Enumerators passed as remaining indices.
   * @return            A reference to the updated bit set.
   */
  template <typename T0, typename T1, typename... Ts>
  TypedBitSet &reset(T0 v0, T1 v1, Ts... vs)
  {
    reset(v0);
    reset(v1, vs...);
    return *this;
  }

  /**
   * @brief Initialization constructor.
   *
   * @tparam T0     The enumeration type of the first index to set.
   * @tparam T1     The enumeration type of the second index to set.
   * @tparam ...Ts  Enumerations types of remaining indices.
   *
   * @param [in] v0     The enumerator passed as the first index to set.
   * @param [in] v1     The enumerator passed as the second index to set.
   * @param [in] ...vs  Enumerators passed as remaining indices.
   * 
   * Creates a TypedBitSet instance and sets given bits to \c true.
   */
  template <typename T0, typename... Ts>
  TypedBitSet(T0 v0, Ts... vs)
  {
    set(v0, vs...);
  }
};

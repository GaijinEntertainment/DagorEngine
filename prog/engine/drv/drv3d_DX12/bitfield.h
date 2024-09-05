#pragma once

#include <EASTL/type_traits.h>

/**
 * @file
 * @brief Defines macros for managing a bit field type and its members.
 * 
 * @note Duplicated in vulkan backend. Taken from https://github.com/preshing/cpp11-on-multicore/blob/master/common/
 */

/**
 * @brief Represents a member of a bit field type.
 * 
 * @tparam T        The underlying type that stores bit fields. Must be integral.
 * @tparam Offset   The bit offset at which this bit field member starts within the bit field type.
 * @tparam Bits     The bit size of the bit field member.
 * 
 * @note \b Offset + \b Bits must not be greater than the size of type \b T.
 * @note \b Bits must be less than than the size of type \b T.
 * 
 * The structs represents a bit field occupying a part of a memory block allocated to the bit field type.
 * The block is shared and/or divided between other bit field within the type.
 * The class is used internally by ADD_BITFIELD_MEMBER macro.
*/
template <typename T, int Offset, int Bits>
struct BitFieldMember
{
  /**
   * @brief The underlying value that stores bit fields.
   */
  T value;

  /**
   * @brief Stores bit field member template parameters values.
   */
  enum
  {
    BitOffset = Offset, /**<The bit offset at which this bit field member starts within the bit field type.*/
    BitCount = Bits     /**<The bit size of the bit field member.*/
  };

  /**
   * @brief The maximum value that can be represented by the bit field member.
   */
  static const T Maximum = (T(1) << T(Bits)) - ((typename eastl::make_signed<T>::type)(1));

  /**
   * @brief The mask for isolating the bits of the bit field member.
   */
  static const T Mask = Maximum << T(Offset);

  /**
  * @brief Returns the maximum value that can be represented by the bit field member.
  * 
  * @returns The maximum value of the bitfield.
  */
  inline T maximum() const
  {
    G_STATIC_ASSERT(Offset + Bits <= (int)sizeof(T) * 8);
    G_STATIC_ASSERT(Bits < (int)sizeof(T) * 8);
    return Maximum;
  }

  /**
   * @brief Constructs a bit field member value in which the only set bit is the first.
   * 
   * @returns The constructed value.
   */
  inline T one() const { return T(1) << Offset; }

  /**
   * @brief Retrieves the bit field member value and converts it to the underlying type value.
   * 
   * @return The converted bit field value.
   */
  inline operator T() const { return (value >> Offset) & Maximum; }

  /**
   * @brief Assigns a new value to the bit field member.
   * 
   * @param [in] v  The value to assign.
   * @return        A reference to the modified BitFieldMember.
   * 
   * @note \b v must fit inside the bit field member. 
   */
  inline BitFieldMember &operator=(T v)
  {
    G_ASSERTF(v <= Maximum, "%d <= %d", v, maximum()); // v must fit inside the bitfield member
    value = (value & ~Mask) | (v << T(Offset));
    return *this;
  }

  /**
   * @brief Adds a value to the bit field member and assigns the result to it.
   * 
   * @param [in] v  The value to add.
   * @return        A reference to the modified bit field member.
   *
   * @note The resulted value must fit inside the bit field member.
   */
  inline BitFieldMember &operator+=(T v)
  {
    G_ASSERT(T(*this) + v <= Maximum); // result must fit inside the bitfield member
    value += v << Offset;
    return *this;
  }

  /**
   * @brief Subtracts a value from the bit field member and assigns the result to it.
   * 
   * @param [in]    v The value to subtract.
   * @return        A reference to the modified bit field member.
   *
   * @note The resulted value must not underflow.
   */
  inline BitFieldMember &operator-=(T v)
  {
    G_ASSERT(T(*this) >= v); // result must not underflow
    value -= v << Offset;
    return *this;
  }

  /**
   * @brief Increments the bit field member.
   * 
   * @return A reference to the incremented bit field member.
   *
   * @note The resulted value must fit inside the bit field member.
   */
  inline BitFieldMember &operator++() { return *this += 1; }

  /**
   * @brief Increments the bit field member.
   * 
   * @return A copy of the bit field member value from before incrementation.
   *
   * @note The resulted value must fit inside the bit field member.
   */
  inline T operator++(int)
  {
    T c = *this;
    ++(*this);
    return c;
  } // postfix form

  /**
   * @brief Decrements the bit field member.
   * 
   * @return A reference to the decremented bit field member.
   *
   * @note The resulted value must not underflow.
   */
  inline BitFieldMember &operator--() { return *this -= 1; }

  /**
   * @brief Increments the bit field member.
   * 
   * @return A copy of the bit field member value from before the decrementation.
   *
   * @note The resulted value must not underflow.
   */
  inline T operator--(int)
  {
    T c = *this;
    --(*this);
    return c;
  } // postfix form
};


/**
 * @brief Represents an array of bit fields as a member of a bit field type.
 *
 * @tparam T            The underlying type that stores bit fields. Must be integral.
 * @tparam BaseOffset   The bit offset at which the first bit field in the array starts within the bit field type.
 * @tparam BitsPerItem  The bit size of each bit field in the array.
 * @tparam NumItems     The number of elements in the array.
 *
 * @note \b BaseOffset + \b BitsPerItem * \b NumItems must not be greater than the size of type \b T.
 * @note \b BitsPerItem must be less than the size of type \b T.
 *
 * The structs represents an array of bit fields occupying a part of a memory block allocated to the bit field type.
 * The block is shared and/or divided between other bitfield members.
 * The class is used internally by ADD_BITFIELD_ARRAY macro.
 */
template <typename T, int BaseOffset, int BitsPerItem, int NumItems>
struct BitFieldArray
{

  /**
   * @brief The underlying value that stores bitfields.
   */
  T value;

  /**
   * @brief The maximum value that can be represented by an element of the bit field array.
   */
  static const T Maximum = (T(1) << BitsPerItem) - 1;

  /**
   * @brief Returns the maximum value that can be represented by an element of the bit field array.
   * 
   * @returns The maximum value of the bit field array element.
   */
  inline T maximum() const
  {
    G_STATIC_ASSERT(BaseOffset + BitsPerItem * NumItems <= (int)sizeof(T) * 8);
    G_STATIC_ASSERT(BitsPerItem < (int)sizeof(T) * 8);
    return Maximum;
  }

  /**
   * @brief Returns a number of bit fields stored in the array.
   * 
   * @returns The number of bitfields in the array.
   */
  inline int numItems() const { return NumItems; }

  /**
   * @brief Represents an individual element within the bitfield array.
   */
  class Element
  {
  private:

    /**
     * @brief Reference to the underlying value that stores bit fields.
     */
    T &value;

    /**
     * @brief The bit offset at which this element starts within the bit field type.
     */
    int offset;

  public:

    /**
     * @brief Constructor.
     * 
     * @param [in] value    A reference to the underlying value that stores bit fields.
     * @param [in] offset   The bit offset at which this element starts within the bit field type.
     */
    inline Element(T &value, int offset) : value(value), offset(offset) {}

    /**
     * @brief Returns a bit mask for isolating the element from other bit field members.
     * 
     * @return A bit mask for isolating the element.
     */
    inline T mask() const { return Maximum << offset; }

    /**
     * @brief Retrieves the bit field array element value and converts it to the underlying type.
     * 
     * @return The element value.
     */
    inline operator T() const { return (value >> offset) & Maximum; }

    /**
     * @brief Assigns a new value to the bit field array element.
     * 
     * @param [in]  v   The value to assign.
     * @return          A reference to the modified element.
     *
     * @note \b v must fit inside the bit field array element.
     */
    inline Element &operator=(T v)
    {
      G_ASSERT(v <= Maximum); // v must fit inside the bitfield member
      value = (value & ~mask()) | (v << offset);
      return *this;
    }

    /**
     * @brief Adds a value to the bit field array element and assigns the result to it.
     * 
     * @param [in] v   The value to add.
     * @return         A reference to the modified element.
     *
     * @note The resulted value must fit inside the bit field array member.
     */
    inline Element &operator+=(T v)
    {
      G_ASSERT(T(*this) + v <= Maximum); // result must fit inside the bitfield member
      value += v << offset;
      return *this;
    }

    /**
     * @brief Subtracts a value from the bit field array element and assigns the result to it.
     * 
     * @param [in] v   The value to subtract.
     * @return         A reference to the modified element.
     *
     * @note The resulted value must not underflow.
     */
    inline Element &operator-=(T v)
    {
      G_ASSERT(T(*this) >= v); // result must not underflow
      value -= v << offset;
      return *this;
    }

    /**
     * @brief Increments the bit field array element.
     * 
     * @return A reference to the incremented element.
     * @note The resulted value must fit inside the bit field member.
     */
    inline Element &operator++() { return *this += 1; }

    /**
     * @brief Increments the bit field array element.
     * 
     * @return A copy of the element value from before incrementation.
     * @note The resulted value must fit inside the bit field member.
     */
    inline T operator++(int)
    {
      T cpy = *this;
      ++(*this);
      return cpy;
    } // postfix form

    /**
     * @brief Decrements the bit field array element.
     * 
     * @return A reference to the decremented element.
     * @note The resulted value must not underflow.
     */
    inline Element &operator--() { return *this -= 1; }

    /**
     * @brief Decrements the bit field array element.
     * 
     * @return A copy of the element value from before decrementation.
     * @note The resulted value must not underflow.
     */
    inline T operator--(int)
    {
      T cpy = *this;
      --(*this);
      return cpy;
    } // postfix form
  };

  /**
   * @brief Accesses an element of the bit field array.
   * 
   * @param [in] i   The index of the accessed element. it must be less than \c NumItems.
   * @return         A copy of the element at the specified index.
   */
  inline Element operator[](int i)
  {
    G_ASSERT(i >= 0 && i < NumItems); // array index must be in range
    return Element(value, BaseOffset + BitsPerItem * i);
  }

  /**
   * @brief Accesses an element of the bit field array (const version).
   * 
   * @param [in] i   The index of the accessed element. it must be less than \c NumItems.
   * @return         A copy of the element at the specified index.
   */
  inline const Element operator[](int i) const
  {
    G_ASSERT(i >= 0 && i < NumItems); // array index must be in range
    return Element(const_cast<T &>(value), BaseOffset + BitsPerItem * i);
  }
};

/**
 * @brief Begins a bit field type declaration.
 * 
 * @param typeName  The name of the new type.
 * @param T         The underlying type storing the bit field members. Must be integral.    
 * 
 * Begins a union struct declaration which implements the following methods:\n
 * \li The default constructor
 * \li The constructor which takes a value of the underlying type as an argument.
 * \li The cast operator which converts the bit field type to reference to the underlying type.
 * \li The cast operator which converts the bit field type to the underlying type.
 * 
 * After the bit field type has been introduced, \ref ADD_BITFIELD_MEMBER and \ref ADD_BITFIELD_ARRAY macros
 * can be used to declare bit fields as members of the type. The members can be used to access 
 * particualar bit segments of the bit field. They provide different layout or view on the memory allocated to the bit field.
 * The members may overlap. 
 * 
 * In the end of the bit field type declaration, \ref ADD_BITFIELD_ARRAY must be called.
 */
#define BEGIN_BITFIELD_TYPE(typeName, T) \
  union typeName                         \
  {                                      \
    struct Wrapper                       \
    {                                    \
      T value;                           \
    };                                   \
    Wrapper wrapper;                     \
    inline typeName()                    \
    {                                    \
      wrapper.value = 0;                 \
    }                                    \
    inline explicit typeName(T v)        \
    {                                    \
      wrapper.value = v;                 \
    }                                    \
    inline operator T &()                \
    {                                    \
      return wrapper.value;              \
    }                                    \
    inline operator T() const            \
    {                                    \
      return wrapper.value;              \
    }                                    \
    typedef T StorageType;

/**
 * @brief Adds a bit field member to the bit field type.
 * 
 * @param memberName    The name of the new member.
 * @param offset        The bit offset at which this bit field member starts within the bit field type. 
 * @param bits          The bit size of the bit field member.
 * 
 * @note \b offset + \b bits must not be greater than the size of the bit field underlying type.
 * @note \b bits must be less than than the size of the bit field underlying type.
 * 
 * Can only be used within bit field type declaration. See \ref BEGIN_BITFIELD_TYPE for details.
 */
#define ADD_BITFIELD_MEMBER(memberName, offset, bits) BitFieldMember<StorageType, offset, bits> memberName;

/**
 * @brief Adds a bit field array member to the bit field type.
 *
 * @param memberName    The name of the new member.
 * @param offset        The bit offset at which the first bit field in the array starts within the bit field type.
 * @param bits          The bit size of each bit field in the array.
 * @param               The number of elements in the array.
 * 
 * @note \b offset + \b bits * \b numItems must not be greater than the size of the bit field underlying type.
 * @note \b bits must be less than the size of the bit field underlying type.
 * 
 * Can only be used within bit field type declaration. See \ref BEGIN_BITFIELD_TYPE for details.
 */
#define ADD_BITFIELD_ARRAY(memberName, offset, bits, numItems) BitFieldArray<StorageType, offset, bits, numItems> memberName;

/**
 * @brief Concludes the bit field type declaration.
 * 
 * See \ref BEGIN_BITFIELD_TYPE for details.
*/
#define END_BITFIELD_TYPE() \
  }                         \
  ;

/**
 * @brief Determines the minimal number of bits needed to represent a given unsigned integer.
 * @tparam I The number to represent.
 * 
 * @todo This should be replaced with a constexpr function. It will compile a lot faster.
 */
template <uint32_t I>
struct BitsNeeded
{
    /**
     * @brief The minimal number of bits needed to represent \c I.
     */
    static constexpr int VALUE = BitsNeeded<I / 2>::VALUE + 1;
};

//@cond
template <>
struct BitsNeeded<0>
{
  static constexpr int VALUE = 1;
};
template <>
struct BitsNeeded<1>
{
  static constexpr int VALUE = 1;
};
//@endcond

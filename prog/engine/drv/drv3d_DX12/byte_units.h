#pragma once

#include <util/dag_inttypes.h>

/**
 * @file
 * @brief Defines utility functions and a class for handling byte units.
 */

/**
 * @brief Determines the appropriate unit index for the given size.
 * 
 * @param [in] sz   The size in bytes.
 * @return          The index of the unit in the unit table.
 * 
 * See \ref get_unit_name function reference for more details.
 */
inline uint32_t size_to_unit_table(uint64_t sz)
{
  uint32_t unitIndex = 0;
  unitIndex += sz >= (1024 * 1024 * 1024);
  unitIndex += sz >= (1024 * 1024);
  unitIndex += sz >= (1024);
  return unitIndex;
}

/**
 * @brief Retrieves the name of the unit based on the unit index.
 * 
 * @param [in] index    The index of the unit.
 * @return              <c>"Bytes", "KiBytes", "MiBytes" or "GiBytes"</c> depending on the unit index.
 * 
 * 
 * See \ref size_to_unit_table function reference for more details.
 */
inline const char *get_unit_name(uint32_t index)
{
  static const char *unitTable[] = {"Bytes", "KiBytes", "MiBytes", "GiBytes"};
  return unitTable[index];
}

/**
 * @brief Convertes a size value in bytes to the equivalent value in units given the unit index.
 * 
 * @param [in] sz           The size in bytes.
 * @param [in] unit_index   The index of the unit.
 * @return                  The converted size.
 * 
 * See \ref get_unit_name function reference for more details.
 */
inline float compute_unit_type_size(uint64_t sz, uint32_t unit_index) { return static_cast<float>(sz) / (powf(1024, unit_index)); }

/**
 * @brief A class representing a byte size as a number of appropriate units.
 * 
 * The appropriate unit is the largest unit capable of representing
 * the given size as a number that is not less than 1. 
 */
class ByteUnits
{
  /*
   * @brief The size in bytes.
   */
  uint64_t size = 0;

public:
  /**
   * @brief Default constructor.
   */
  ByteUnits() = default;

  /**
   * @brief Copy constructor.
   * @param [in] other An instance to copy.
   */
  ByteUnits(const ByteUnits &) = default;

  /**
   * @brief Assignment operator.
   * @param [in] other An instance to assign.
   */
  ByteUnits &operator=(const ByteUnits &) = default;

  /**
   * @brief Initialization constructor.
   * @param [in] v The size in bytes.
   * 
   * Constructs \c ByteUnits object and initializes its \c size with the given value.
   */
  ByteUnits(uint64_t v) : size{v} {}

  /**
   * @brief Size assignment operator.
   * 
   * @param [in] v  The new size in bytes.
   * @return        Reference to the updated ByteUnits object.
   * 
   * Assigns a value to \c size.
   */
  ByteUnits &operator=(uint64_t v)
  {
    size = v;
    return *this;
  }

  /**
   * @brief Size addition assignment operator.
   * @param [in] o  The instance whose \c size is to be added.
   * @return        Reference to the updated \c ByteUnits instance.
   * 
   * Modifies \c size value by adding \c size of a \c ByteUnits object.
   */
  ByteUnits &operator+=(ByteUnits o)
  {
    size += o.size;
    return *this;
  }

  /**
   * @brief Size subtraction assignment operator.
   * @param [in] o  The instance whose \c size is to be subtracted.
   * @return        Reference to the updated \c ByteUnits instance.
   *
   * Modifies \c size value by subtracting \c size of a \c ByteUnits object.
   */
  ByteUnits &operator-=(ByteUnits o)
  {
    size -= o.size;
    return *this;
  }

  /**
   * @brief Size addition operator.
   * @param [in] l,r  The instances whose \c size values are to be summed.
   * @return          An instance with its \c size being the sum of \c l.size and \c r.size.
   *
   * Sums the \c size values of 2 \c ByteUnits objects. 
   */
  friend ByteUnits operator+(ByteUnits l, ByteUnits r) { return {l.size + r.size}; }

  /**
   * @brief Size subtraction operator.
   * @param [in] l  The instance whose \c size is to be subtracted from.
   * @param [in] r  The instance whose \c size is to be subtracted.
   * @return        An instance whose size equals \c l.size - \c r.size.
   *
   * Subtracts \c r.size from \c l.size.
   */
  friend ByteUnits operator-(ByteUnits l, ByteUnits r) { return {l.size - r.size}; }

  /**
   * @brief Returns the \c size value in bytes.
   * 
   * @return The \c size value in bytes.
   */
  uint64_t value() const { return size; }

  /**
   * @brief Returns the size representation in appropriate units.
   * 
   * @return The size in the appropriate unit.
   */
  float units() const { return compute_unit_type_size(size, size_to_unit_table(size)); }

  /**
   * @brief Returns the name of the appropriate unit.
   *
   * @return <c>"Bytes", "KiBytes", "MiBytes" or "GiBytes"</c> depending on the \c size value.
   */
  const char *name() const { return get_unit_name(size_to_unit_table(size)); }
};

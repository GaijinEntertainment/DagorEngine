//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_generationRefId.h>

namespace shaders
{
/**
 * \brief A helper class used to declare \p OverrideStateId type.
 */
class OverrideStateIdDummy
{};
/**
 * \brief A type used to identify override state.
 */
typedef GenerationRefId<8, OverrideStateIdDummy> OverrideStateId; // weak reference

struct OverrideState;

namespace overrides
{
/**
 * \brief Create a new override state.
 *
 * \param s The override state description.
 * \return A handle to the created override state.
 */
OverrideStateId create(const OverrideState &);
/**
 * \brief Destroy an override state.
 *
 * \param override_id The handle to the override state to destroy.
 * \return \p true if the override state was destroyed, \p false otherwise (state id wasn't found).
 */
bool destroy(OverrideStateId &override_id);
/**
 * \brief Check if an override state exists.
 *
 * \param override_id The handle to the override state to check.
 * \return \p true if the override state exists, \p false otherwise.
 */
bool exists(OverrideStateId override_id);

//! set current override. It will fail, if there is already one set, and override_id is not invalid.
/**
 * \brief Set the current override state.
 *
 * If there is already an override state set, this function will fail. The user will be notified with a logerr message.
 * If the override_id is invalid, the current override state will be reset. The user will be notified with a logerr message.
 * The method invalidates the cached state block.
 *
 * \param override_id The handle to the override state to set.
 * \return \p true if the override state was set, \p false otherwise.
 */
bool set(OverrideStateId override_id);
/**
 * \brief Reset the current override state. It sets the override state that doesn't have any effect.
 *
 * The method invalidates the cached state block.
 *
 * \return \p true if the override state was reset, \p false otherwise.
 */
inline bool reset() { return set(OverrideStateId()); }

/**
 * \brief Get the override state description.
 *
 * \param override_id The handle to the override state to get.
 * \return The override state description.
 */
OverrideState get(OverrideStateId override_id);
/**
 * \brief Get the current override state.
 *
 * \return The handle to the current override state.
 */
OverrideStateId get_current();
/**
 * \brief Get the current override state with the master override state applied.
 *
 * The master override is applied to all the render states.
 *
 * \return The handle to the current override state with the master override state applied.
 */
OverrideStateId get_current_with_master();

/**
 * \brief Set the master override state.
 *
 * The master override state is applied to all the render states.
 *
 * \param s The master override state description.
 */
void set_master_state(const OverrideState &s);
/**
 * \brief Reset the master override state.
 */
void reset_master_state();

/**
 * \brief Destroy all managed master override states.
 */
void destroy_all_managed_master_states();
} // namespace overrides

/**
 * \brief A unique handle to the override state.
 *
 * It will destroy the referenced override state id in the destructor.
 */
struct UniqueOverrideStateId
{
  /**
   * \brief Destructor.
   *
   * It will destroy the referenced override state id.
   */
  ~UniqueOverrideStateId() { shaders::overrides::destroy(ref); };
  /**
   * \brief Default constructor.
   *
   * It creates an empty handle.
   */
  UniqueOverrideStateId() = default;
  /**
   * \brief Constructor with the override state id.
   *
   * It creates a handle with the given override state id.
   *
   * \param id The override state id.
   */
  UniqueOverrideStateId(const OverrideStateId &id) { reset(id); }
  /**
   * \brief Assignment operator.
   *
   * It destroys the current override state id and sets the new one.
   *
   * \param id The override state id.
   * \return Reference to this object.
   */
  UniqueOverrideStateId &operator=(OverrideStateId id)
  {
    reset(id);
    return *this;
  }
  /**
   * \brief Reset the handle with the new override state id.
   *
   * It destroys the current override state id and sets the new one.
   *
   * \param id The override state id.
   * \return \p true if the override state was reset, \p false otherwise.
   */
  bool reset(OverrideStateId id = OverrideStateId())
  {
    bool ret = overrides::destroy(ref);
    ref = id;
    return ret;
  }
  /**
   * \brief Get the override state id.
   *
   * \return The override state id.
   */
  const OverrideStateId &get() const { return ref; }
  /**
   * \brief Get the override state id.
   *
   * \return The override state id.
   */
  OverrideStateId &get() { return ref; }
  /**
   * \brief Move constructor.
   *
   * It moves the override state id from the given handle.
   * The given handle will be empty after the move.
   *
   * \param id The handle to move.
   */
  UniqueOverrideStateId(UniqueOverrideStateId &&id)
  {
    ref = id.ref;
    id.ref = OverrideStateId();
  }
  /**
   * \brief Move assignment operator.
   *
   * It moves the override state id from the given handle.
   * The given handle will be empty after the move.
   *
   * \param id The handle to move.
   * \return Reference to this object.
   */
  UniqueOverrideStateId &operator=(UniqueOverrideStateId &&id)
  {
    reset(id.ref);
    id.ref = OverrideStateId();
    return *this;
  }
  /**
   * \brief Copy constructor.
   *
   * It is deleted because \p UniqueOverrideStateId owns the state id exclusivelly.
   *
   * \param id The handle to copy.
   */
  UniqueOverrideStateId(const UniqueOverrideStateId &id) = delete;
  /**
   * \brief Copy assignment operator.
   *
   * It is deleted because \p UniqueOverrideStateId owns the state id exclusivelly.
   *
   * \param id The handle to copy.
   * \return Reference to this object.
   */
  UniqueOverrideStateId &operator=(const UniqueOverrideStateId &id) = delete;
  /**
   * \brief Check if the handle is valid.
   *
   * \return \p true if the handle is valid, \p false otherwise.
   */
  explicit operator bool() const { return (bool)ref; }

protected:
  OverrideStateId ref; ///< The override state id owned by the class.
};

namespace overrides
{
/**
 * \brief Destroy the unique override state id.
 *
 * It makes the handle empty, so the override state doesn't have any effect.
 *
 * \param id The unique override state id to destroy.
 */
inline bool destroy(UniqueOverrideStateId &id) { return id.reset(); }
/**
 * \brief Set the override state.
 *
 * It sets the override state to the current state.
 *
 * \param id The unique override state id to set.
 */
inline void set(UniqueOverrideStateId &id) { shaders::overrides::set(id.get()); }
/**
 * \brief Set the override state.
 *
 * It sets the override state to the current state.
 *
 * \param id The unique override state id to set.
 */
inline void set(const UniqueOverrideStateId &id) { shaders::overrides::set(id.get()); }
/**
 * \brief Check if the override state exists.
 *
 * \param id The unique override state id to check.
 * \return \p true if the override state exists, \p false otherwise.
 */
inline bool exists(UniqueOverrideStateId id) { return shaders::overrides::exists(id.get()); }
/**
 * \brief Get the override state description.
 *
 * \param id The unique override state id to get.
 * \return The override state description.
 */
OverrideState get(const UniqueOverrideStateId &id);
} // namespace overrides

}; // namespace shaders

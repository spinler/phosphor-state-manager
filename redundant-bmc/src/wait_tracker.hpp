// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string>
#include <vector>

namespace rbmc
{

/**
 * These APIs are used for saving what the code is currently waiting for
 * so that things like rbmctool can display it.
 */

enum class Wait
{
    startUnit,
    systemInventoryPath,
    systemInventoryStatus,
    peerConnection,
    siblingAlive,
    siblingRole,
    siblingBMCSteadyState,
    siblingHealth,
    fullSync,
    failoverImminent
};

/**
 * @brief Convert a Wait enum value to its string name.
 *
 * @param[in] wait - The Wait enum value
 * @return The string name with the first letter uppercased, e.g. "StartUnit"
 */
std::string waitToString(Wait wait);

/**
 * @brief Returns the currently tracked waits as a vector of strings.
 *
 * e.g. "StartUnit", "SiblingAlive".
 *
 * @return vector of tracked wait names, empty if none are tracked.
 */
std::vector<std::string> getTrackedWaits();

/**
 * @brief Add a wait to the tracked waits list.
 *
 * If the wait is already in the list, it will not be added again.
 *
 * @param[in] wait - The wait to add
 */
void addTrackedWait(Wait wait);

/**
 * @brief Remove a wait from the tracked waits list.
 *
 * If the wait is not in the list, nothing happens.
 *
 * @param[in] wait - The wait to remove
 */
void removeTrackedWait(Wait wait);

/**
 * @brief Remove all tracked waits.
 *
 * Clears the entire tracked waits list from persistent storage.
 * If there are no tracked waits, nothing happens.
 */
void removeAllTrackedWaits();

/**
 * @class ScopeWaitTracker
 *
 * RAII class that adds a wait on construction and removes it on destruction.
 */
class ScopeWaitTracker
{
  public:
    ScopeWaitTracker(const ScopeWaitTracker&) = delete;
    ScopeWaitTracker& operator=(const ScopeWaitTracker&) = delete;
    ScopeWaitTracker(ScopeWaitTracker&&) = delete;
    ScopeWaitTracker& operator=(ScopeWaitTracker&&) = delete;

    ScopeWaitTracker(Wait w) : wait(w)
    {
        addTrackedWait(wait);
    }

    ~ScopeWaitTracker()
    {
        removeTrackedWait(wait);
    }

  private:
    Wait wait;
};

} // namespace rbmc

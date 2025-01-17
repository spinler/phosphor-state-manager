// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <xyz/openbmc_project/State/BMC/Redundancy/common.hpp>
#include <xyz/openbmc_project/State/BMC/common.hpp>

namespace rbmc
{

using Role =
    sdbusplus::common::xyz::openbmc_project::state::bmc::Redundancy::Role;

using BMCState = sdbusplus::common::xyz::openbmc_project::state::BMC::BMCState;

namespace redundancy
{

/**
 * @brief Inputs to the getNoRedundancyReasons function
 */
struct Input
{
    Role role;
    bool siblingPresent;
    bool siblingHeartbeat;
    bool siblingProvisioned;
    bool siblingHasSiblingComm;
    Role siblingRole;
    BMCState siblingState;
    bool codeVersionsMatch;
    bool manualDisable;
};

// TODO: Move this to PDI Enums
/**
 * @brief Reasons why redundancy can't be enabled
 */
enum class NoRedundancyReason
{
    none,
    bmcNotActive,
    manuallyDisabled,
    siblingMissing,
    noSiblingHeartbeat,
    siblingNotProvisioned,
    siblingNotPassive,
    siblingNoCommunication,
    codeMismatch,
    siblingNotAtReady,
    systemHardwareConfigIssue,
    other
};

using NoRedundancyReasons = std::set<NoRedundancyReason>;

/**
 * @brief Returns the reasons that redundancy can't be enabled.
 *
 * @return The reasons.  Empty if it can be enabled.
 */
NoRedundancyReasons getNoRedundancyReasons(const Input& input);

/**
 * @brief Return the string description of the reason
 *
 * @return The human readable description.
 */
std::string getNoRedundancyDescription(NoRedundancyReason reason);

namespace fp
{

/**
 * @brief Inputs to the getFailoversPausedReasons function
 */
struct Input
{
    bool siblingHeartbeat;
};

/**
 * @brief Reasons why failovers have to be paused
 */
enum class FailoversPausedReason
{
    noSiblingHeartbeat
};

using FailoversPausedReasons = std::set<FailoversPausedReason>;

/**
 * @brief Returns the reasons that failovers must be paused
 *
 * @return The reasons.  Empty there are none.
 */
FailoversPausedReasons getFailoversPausedReasons(const fp::Input& input);

/**
 * @brief Return the string description of the reason
 *
 * @return The human readable description.
 */
std::string getFailoversPausedDescription(FailoversPausedReason reason);

} // namespace fp

} // namespace redundancy
} // namespace rbmc

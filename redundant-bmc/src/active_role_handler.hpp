// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "health_monitor.hpp"
#include "redundancy_mgr.hpp"
#include "role_handler.hpp"

namespace rbmc
{

/**
 * @class ActiveRoleHandler
 *
 * This class handles operation specific to the active role.
 */
class ActiveRoleHandler : public RoleHandler
{
  public:
    ActiveRoleHandler(const ActiveRoleHandler&) = delete;
    ActiveRoleHandler& operator=(const ActiveRoleHandler&) = delete;
    ActiveRoleHandler(ActiveRoleHandler&&) = delete;
    ActiveRoleHandler& operator=(ActiveRoleHandler&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] ctx - The async context object
     * @param[in] services - The services object
     * @param[in] sibling - The sibling object
     * @param[in] iface - The redundancy D-Bus interface object
     */
    ActiveRoleHandler(sdbusplus::async::context& ctx, Services& services,
                      Sibling& sibling, RedundancyInterface& iface);

    /**
     * @brief Destructor
     */
    ~ActiveRoleHandler() override
    {
        sibling.clearCallbacks("active");
    }

    /**
     * @brief Starts the handler.
     */
    sdbusplus::async::task<> start() override;

    /**
     * @brief Called when the DisableRedundancyOverride D-Bus property
     *        is updated.
     *
     * May disable or enable redundancy at this time if possible.
     *
     * @param[in] disable - If redundancy should be disabled
     *                      or enabled.
     */
    void disableRedPropChanged(bool disable) override
    {
        redMgr.disableRedPropChanged(disable);
    }

  private:
    /**
     * @brief Starts the Sibling property watches/callbacks
     */
    inline void startSiblingWatches()
    {
        sibling.addBMCStateCallback(
            "active",
            std::bind_front(&ActiveRoleHandler::siblingStateChange, this));

        siblingHBMon.startMonitor(sibling.hasHeartbeat());
    }

    /**
     * @brief Stops the sibling property callbacks/watches
     */
    inline void stopSiblingWatches()
    {
        sibling.clearBMCStateCallback("active");
        siblingHBMon.stopMonitor();
    }

    using BMCState =
        sdbusplus::common::xyz::openbmc_project::state::BMC::BMCState;

    /**
     * @brief Called when the sibling's BMC state changes
     *        assuming the callback has been enabled.
     *
     * @param[in] state - The new state value
     */
    void siblingStateChange(BMCState state);

    /**
     * @brief Called by the HealthMonitor on sibling heartbeat
     *        events (good, warning, critical).
     */
    sdbusplus::async::task<> siblingHBEvent(HealthMonitor::State state);

    /**
     * @brief Called by the HealthMonitor (via siblingHBEvent: good)
     *        when the sibling BMC's heartbeat starts back up.
     *
     * This will attempt to re-enable redundancy, though it might
     * not be possible for other reasons.
     */
    sdbusplus::async::task<> siblingHBStarted();

    /**
     * @brief Called by the HealthMonitor (via siblingHBEvent: warning)
     *        when it detects the sibling BMC's heartbeat first stopped.
     *
     * This will cause failoversPaused to be asserted.
     */
    sdbusplus::async::task<> siblingHBWarning();

    /**
     * @brief Called by the HealthMonitor (via siblingHBEvent: critical)
     *        when it detects the sibling heartbeat has been stopped
     *        long enough that redundancy will need to be disabled.
     */
    sdbusplus::async::task<> siblingHBCritical();

    /**
     * @brief Redundancy manager object
     */
    RedundancyMgr redMgr;

    /**
     * @brief Sibling heartbeat monitor
     */
    HealthMonitor siblingHBMon;
};

} // namespace rbmc

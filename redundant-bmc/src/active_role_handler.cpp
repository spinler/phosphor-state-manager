// SPDX-License-Identifier: Apache-2.0
#include "active_role_handler.hpp"

#include <phosphor-logging/lg2.hpp>

namespace rbmc
{

constexpr auto bmcActiveTarget = "obmc-bmc-active.target";
const std::chrono::seconds siblingBufferDuration{5};
const std::chrono::minutes siblingWarningDuration{5};

ActiveRoleHandler::ActiveRoleHandler(sdbusplus::async::context& ctx,
                                     Services& services, Sibling& sibling,
                                     RedundancyInterface& iface) :
    RoleHandler(ctx, services, sibling, iface),
    redMgr(ctx, services, sibling, iface),
    siblingHBMon(ctx, std::bind_front(&ActiveRoleHandler::siblingHBEvent, this),
                 siblingBufferDuration, siblingWarningDuration)
{
    // Let the sibling HB monitor know when the HB changes
    sibling.addHeartbeatCallback("active", [this](bool hb) {
        siblingHBMon.setHealthStatus(hb);
    });
}

// NOLINTNEXTLINE
sdbusplus::async::task<> ActiveRoleHandler::start()
{
    try
    {
        // NOLINTNEXTLINE
        co_await services.startUnit(bmcActiveTarget);
    }
    catch (const sdbusplus::exception_t& e)
    {
        // TODO: error log
        lg2::error("Failed while starting BMC active target: {ERROR}", "ERROR",
                   e);
    }

    if (sibling.hasHeartbeat())
    {
        // Make sure the sibling had time to get its role assigned, and
        // also wait for it to hit steady state as redundancy can only
        // be enabled if the sibling BMC is at the Ready state.
        co_await sdbusplus::async::execution::when_all(
            sibling.waitForSiblingRole(), sibling.waitForBMCSteadyState());
    }

    redMgr.determineAndSetRedundancy();

    // TODO: Create an error if no redundancy

    startSiblingWatches();

    co_return;
}

void ActiveRoleHandler::siblingStateChange(BMCState state)
{
    if (state == BMCState::Quiesced)
    {
        lg2::error("Sibling BMC went to Quiesce, disabling redundancy");
        redMgr.determineAndSetRedundancy();
    }
}

sdbusplus::async::task<>
    // NOLINTNEXTLINE
    ActiveRoleHandler::siblingHBEvent(HealthMonitor::State state)
{
    if (state == HealthMonitor::State::good)
    {
        co_return co_await siblingHBStarted();
    }
    else if (state == HealthMonitor::State::warning)
    {
        co_return co_await siblingHBWarning();
    }
    else if (state == HealthMonitor::State::critical)
    {
        co_return co_await siblingHBCritical();
    }
    else
    {
        lg2::error("Invalid state {STATE} received in siblingHBEvent", "STATE",
                   state);
    }

    co_return;
}

// NOLINTNEXTLINE
sdbusplus::async::task<> ActiveRoleHandler::siblingHBWarning()
{
    lg2::warning("Passive heartbeat warning.");

    if (redundancyInterface.redundancy_enabled())
    {
        redMgr.determineAndSetFailoversPaused();
    }

    co_return;
}

// NOLINTNEXTLINE
sdbusplus::async::task<> ActiveRoleHandler::siblingHBCritical()
{
    lg2::error("Passive heartbeat critical");

    // Disable redundancy
    redMgr.determineAndSetRedundancy();
    co_return;
}

// NOLINTNEXTLINE
sdbusplus::async::task<> ActiveRoleHandler::siblingHBStarted()
{
    lg2::info("Passive heartbeat started");

    // Stop callbacks so they don't fire during these waits
    stopSiblingWatches();

    co_await sdbusplus::async::execution::when_all(
        sibling.waitForSiblingRole(), sibling.waitForBMCSteadyState());

    if (!redundancyInterface.redundancy_enabled())
    {
        redMgr.determineAndSetRedundancy();
    }
    else
    {
        // Still attempt to unpause failovers.
        redMgr.determineAndSetFailoversPaused();
    }

    // TODO: full sync, etc

    startSiblingWatches();

    co_return;
}

} // namespace rbmc

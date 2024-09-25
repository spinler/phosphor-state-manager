#include "active_role_handler.hpp"

#include <phosphor-logging/lg2.hpp>

namespace rbmc
{

constexpr auto bmcActiveTarget = "obmc-bmc-active.target";

// NOLINTNEXTLINE
sdbusplus::async::task<> ActiveRoleHandler::start()
{
    try
    {
        co_await services->startUnit(bmcActiveTarget);
    }
    catch (const sdbusplus::exception_t& e)
    {
        // TODO: error log
        lg2::error("Failed while starting BMC active target: {ERROR}", "ERROR",
                   e);
    }

    co_return;
}

} // namespace rbmc

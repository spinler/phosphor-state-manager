// SPDX-License-Identifier: Apache-2.0

#include "wait_tracker.hpp"

#include "persistent_data.hpp"

#include <phosphor-logging/lg2.hpp>

#include <algorithm>

namespace rbmc
{

std::string waitToString(Wait wait)
{
    switch (wait)
    {
        case Wait::startUnit:
            return "StartUnit";
        case Wait::systemInventoryPath:
            return "SystemInventoryPath";
        case Wait::systemInventoryStatus:
            return "SystemInventoryStatus";
        case Wait::peerConnection:
            return "PeerConnection";
        case Wait::siblingAlive:
            return "SiblingAlive";
        case Wait::siblingRole:
            return "SiblingRole";
        case Wait::siblingBMCSteadyState:
            return "SiblingBMCSteadyState";
        case Wait::siblingHealth:
            return "SiblingHealth";
        case Wait::fullSync:
            return "FullSync";
        case Wait::failoverImminent:
            return "FailoverImminent";
    }
    return "Unknown";
}

std::vector<std::string> getTrackedWaits()
{
    try
    {
        using U = std::underlying_type_t<Wait>;
        auto waits = data::read<std::vector<U>>(data::key::trackedWaits)
                         .value_or(std::vector<U>{});

        std::vector<std::string> result;
        result.reserve(waits.size());
        for (auto w : waits)
        {
            result.push_back(waitToString(static_cast<Wait>(w)));
        }
        return result;
    }
    catch (const std::exception& e)
    {
        lg2::error("Error reading tracked waits: {ERROR}", "ERROR", e);
    }
    return {};
}

void addTrackedWait(Wait wait)
{
    try
    {
        using U = std::underlying_type_t<Wait>;
        auto waits = data::read<std::vector<U>>(data::key::trackedWaits)
                         .value_or(std::vector<U>{});

        if (!std::ranges::contains(waits, std::to_underlying(wait)))
        {
            waits.push_back(std::to_underlying(wait));
            data::write(data::key::trackedWaits, waits);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Error adding tracked wait: {ERROR}", "ERROR", e);
    }
}

void removeTrackedWait(Wait wait)
{
    try
    {
        using U = std::underlying_type_t<Wait>;
        auto waits = data::read<std::vector<U>>(data::key::trackedWaits)
                         .value_or(std::vector<U>{});

        if (std::erase(waits, std::to_underlying(wait)) == 0)
        {
            return;
        }

        if (waits.empty())
        {
            data::remove(data::key::trackedWaits);
        }
        else
        {
            data::write(data::key::trackedWaits, waits);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Error removing tracked wait: {ERROR}", "ERROR", e);
    }
}

void removeAllTrackedWaits()
{
    try
    {
        data::remove(data::key::trackedWaits);
    }
    catch (const std::exception& e)
    {
        lg2::error("Error removing all tracked waits: {ERROR}", "ERROR", e);
    }
}

} // namespace rbmc

/* SPDX-License-Identifier: Apache-2.0 */

#include "health_monitor.hpp"

namespace rbmc
{

void HealthMonitor::setHealthStatus(bool good)
{
    if (shutdown)
    {
        return;
    }

    if (good)
    {
        // Moving from bad to good
        if (state != State::good)
        {
            auto previous = state;
            state = State::good;

            // Going from bad ot good, invoke the 'good'
            // callback. No callback if moving from buffer -> good.
            if ((previous == State::warning) || (previous == State::critical))
            {
                ctx.spawn(callback(state));
            }
        }
        return;
    }

    // At this point, the health is bad.

    if (state != State::good)
    {
        // Already in a bad health state so stateMachine()
        // is already running.
        return;
    }

    // First bad health state is 'buffer'.
    state = State::buffer;
    failTime = std::chrono::steady_clock::now();
    ctx.spawn(stateMachine());
}

// NOLINTNEXTLINE
sdbusplus::async::task<> HealthMonitor::stateMachine()
{
    using namespace std::chrono_literals;
    // Allow less than a second so testcases can be faster.
    std::chrono::milliseconds sleep =
        (bufferDuration > 1s) ? 1s : bufferDuration;

    while (!shutdown && !ctx.stop_requested())
    {
        if (state == State::good)
        {
            // Callback already made in setHealthStatus(), so now
            // just shut down the loop.
            co_return;
        }
        else if (state == State::buffer)
        {
            if ((std::chrono::steady_clock::now() - failTime) >= bufferDuration)
            {
                state = State::warning;
                ctx.spawn(callback(state));
            }
        }
        else if (state == State::warning)
        {
            if ((std::chrono::steady_clock::now() - failTime) >=
                warningDuration)
            {
                state = State::critical;
                ctx.spawn(callback(state));
                co_return;
            }
        }

        co_await sdbusplus::async::sleep_for(ctx, sleep);
    }

    co_return;
}

} // namespace rbmc

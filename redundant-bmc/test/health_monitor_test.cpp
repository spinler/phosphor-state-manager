#include "health_monitor.hpp"

#include <print>

#include <gtest/gtest.h>

using namespace rbmc;

using State = HealthMonitor::State;

class Handler
{
  public:
    // The state callback function for testing.  Could use a mock,
    // but that is a lot more verbose.
    // NOLINTNEXTLINE
    sdbusplus::async::task<> healthStateChange(State state)
    {
        if (state == State::good)
        {
            good++;
        }
        else if (state == State::critical)
        {
            critical++;
        }
        else if (state == State::warning)
        {
            warning++;
        }
        else
        {
            ADD_FAILURE() << "Invalid state in callback "
                          << std::to_underlying(state);
        }
        co_return;
    }
    size_t good = 0;
    size_t warning = 0;
    size_t critical = 0;
};

using namespace std::chrono_literals;

// No events emitted after startMonitor() but before setHealthStatus().
TEST(HealthMonitorTest, InitialStates)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      50ms, 100ms};
    {
        mon.startMonitor(true);
        EXPECT_EQ(handler.good, 0);
        EXPECT_EQ(handler.warning, 0);
        EXPECT_EQ(handler.critical, 0);
    }

    {
        mon.startMonitor(false);
        EXPECT_EQ(handler.good, 0);
        EXPECT_EQ(handler.warning, 0);
        EXPECT_EQ(handler.critical, 0);
    }
}

// Health glitches for less than buffer time. No callbacks.
TEST(HealthMonitorTest, NoCallbackWithinBuffer)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      100ms, 200ms};
    mon.startMonitor(true);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 20ms);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    // no callbacks
    EXPECT_EQ(handler.good, 0);
    EXPECT_EQ(handler.warning, 0);
    EXPECT_EQ(handler.critical, 0);
}

// Health fail warning
TEST(HealthMonitorTest, WarningOnly)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      100ms, 200ms};
    mon.startMonitor(true);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 150ms);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 0);
    EXPECT_EQ(handler.warning, 1);
    EXPECT_EQ(handler.critical, 0);
}

// Health permanently fails. Warning and critical events emitted.
TEST(HealthMonitorTest, WarningThenCritical)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      100ms, 200ms};
    mon.startMonitor(true);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 250ms);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 0);
    EXPECT_EQ(handler.warning, 1);
    EXPECT_EQ(handler.critical, 1);
}

// Health fails until warning event, then recovers.
TEST(HealthMonitorTest, WarningThenGood)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      100ms, 200ms};
    mon.startMonitor(true);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 150ms);
        mon.setHealthStatus(true);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 1);
    EXPECT_EQ(handler.warning, 1);
    EXPECT_EQ(handler.critical, 0);
}

// Health fails until critical event, then recovers.
TEST(HealthMonitorTest, WarningCriticalThenGood)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      100ms, 200ms};
    mon.startMonitor(true);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 225ms);
        mon.setHealthStatus(true);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 1);
    EXPECT_EQ(handler.warning, 1);
    EXPECT_EQ(handler.critical, 1);
}

// Starts out failed, then recovers.
TEST(HealthMonitorTest, FailedThenGood)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      50ms, 100ms};
    mon.startMonitor(false);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(true);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 1);
    EXPECT_EQ(handler.warning, 0);
    EXPECT_EQ(handler.critical, 0);
}

// good -> warning -> critical -> good all emitted
TEST(HealthMonitorTest, GoodWarningCriticalGood)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      100ms, 200ms};
    mon.startMonitor(false);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(true);
        co_await sdbusplus::async::sleep_for(ctx, 5ms);
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 225ms);
        mon.setHealthStatus(true);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 2);
    EXPECT_EQ(handler.warning, 1);
    EXPECT_EQ(handler.critical, 1);
}

// Health fails immediately with no buffer time configured.
TEST(HealthMonitorTest, NoBuffer)
{
    sdbusplus::async::context ctx;
    Handler handler;

    HealthMonitor mon{ctx,
                      std::bind_front(&Handler::healthStateChange, &handler),
                      0ms, 100ms};
    mon.startMonitor(true);

    // NOLINTNEXTLINE
    ctx.spawn([&ctx, &mon]() -> sdbusplus::async::task<> {
        mon.setHealthStatus(false);
        co_await sdbusplus::async::sleep_for(ctx, 1ms);
        ctx.request_stop();
        co_return;
    }());

    ctx.run();

    EXPECT_EQ(handler.good, 0);
    EXPECT_EQ(handler.warning, 1);
    EXPECT_EQ(handler.critical, 0);
}

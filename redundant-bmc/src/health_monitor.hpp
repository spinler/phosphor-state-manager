/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <sdbusplus/async.hpp>

#include <chrono>

namespace rbmc
{

/**
 * @class HealthMonitor
 *
 * This class can monitor a boolean health property and
 * execute callbacks when the health is bad after certain
 * amounts of time.
 *
 * It needs someone else to call the setHealthStatus() method to
 * tell it when the health changes.
 *
 * It will call the callback on these state changes:
 *
 * Warning:
 *   Health is bad longer than bufferDuration but
 *   less than warningDuration.
 *
 * Critical:
 *   Health is bad longer than warningDuration.
 *
 * Good:
 *   Health changes to good.
 *
 * The buffer is an amount of time the health can be bad
 * without affecting anything, to allow for things like
 * service restarts on the sibling.
 *
 * startMonitor() needs to be called with an initial value first.
 * This doesn't trigger any state changes itself.
 *
 * stopMonitor() can be called to stop any further callbacks.
 */
class HealthMonitor
{
  public:
    enum class State
    {
        good,
        buffer,
        warning,
        critical
    };

    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;
    HealthMonitor(HealthMonitor&&) = delete;
    HealthMonitor& operator=(HealthMonitor&&) = delete;

    using StateChangeCallback = std::function<sdbusplus::async::task<>(State)>;

    /**
     * @brief Constructor
     *
     * @param[in] ctx - The async context object
     * @param[in] callback - The function to call when the state changes.
     * @param[in] bufferDuration - The amount of time the health can be bad
     *            before the warning state.
     * @param[in] warningDuration - The amount of time the health can be bad
     *            and still be considered a warning, after which is the
     *            critical state.
     */
    HealthMonitor(sdbusplus::async::context& ctx, StateChangeCallback callback,
                  std::chrono::milliseconds bufferDuration,
                  std::chrono::milliseconds warningDuration) :

        ctx(ctx), callback(std::move(callback)), bufferDuration(bufferDuration),
        warningDuration(warningDuration)
    {}

    /**
     * @brief Destructor.
     */
    ~HealthMonitor()
    {
        shutdown = true;
    }

    /**
     * @brief External callers use to set the new health value
     *
     * @param[in] good - If the health is good or not
     */
    void setHealthStatus(bool good);

    /**
     * @brief Starts monitoring.
     *
     * Does not start any state changes itself.
     *
     * Any calls of setHealthStatus() before this will not
     * do anything.
     *
     * @param[in] good - The initial health value
     */
    void startMonitor(bool good)
    {
        shutdown = false;
        state = (good) ? State::good : State::critical;
    }

    /**
     * @brief Stops any further state change callbacks.
     */
    void stopMonitor()
    {
        shutdown = true;
        state = State::good;
    }

  private:
    /**
     * @brief Implements the state machine that handles the
     *        timers to move between states.
     *
     * @return The coroutine's task object.
     */
    sdbusplus::async::task<> stateMachine();

    /**
     * @brief The async context object
     */
    sdbusplus::async::context& ctx;

    /**
     * @brief The function called when the state changes.
     *
     * The new state is a parameter of the function.
     */
    StateChangeCallback callback;

    /**
     * @brief The amount of time to ride through
     *        bad health.
     */
    std::chrono::milliseconds bufferDuration;

    /**
     * @brief The maximum amount of time the health can be bad
     *        and still be considered a warning.  Any longer
     *        and it's considered critical.
     */
    std::chrono::milliseconds warningDuration;

    /**
     * @brief The current state
     */
    State state = State::critical;

    /**
     * @brief The time point at which the health failed.
     */
    std::chrono::time_point<std::chrono::steady_clock> failTime;

    /**
     * If the state machine is shut down or not, meaning no
     * state change callbacks will be made.
     */
    bool shutdown = true;
};

}; // namespace rbmc

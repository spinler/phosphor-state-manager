// SPDX-License-Identifier: Apache-2.0

#include "persistent_data_test_fixture.hpp"
#include "wait_tracker.hpp"

#include <gtest/gtest.h>

using namespace rbmc;
using namespace rbmc::test;

class WaitTrackerTest : public PersistentDataTestFixture
{};

// Helper to read the raw wait list from persistent data
static std::vector<std::underlying_type_t<Wait>> readWaits()
{
    using U = std::underlying_type_t<Wait>;
    return data::read<std::vector<U>>(data::key::trackedWaits)
        .value_or(std::vector<U>{});
}

TEST_F(WaitTrackerTest, AddSingleWait)
{
    addTrackedWait(Wait::startUnit);

    auto waits = readWaits();
    ASSERT_EQ(waits.size(), 1);
    EXPECT_EQ(waits[0], std::to_underlying(Wait::startUnit));
}

TEST_F(WaitTrackerTest, AddMultipleWaits)
{
    addTrackedWait(Wait::startUnit);
    addTrackedWait(Wait::siblingAlive);
    addTrackedWait(Wait::fullSync);

    auto waits = readWaits();
    ASSERT_EQ(waits.size(), 3);
    EXPECT_TRUE(
        std::ranges::contains(waits, std::to_underlying(Wait::startUnit)));
    EXPECT_TRUE(
        std::ranges::contains(waits, std::to_underlying(Wait::siblingAlive)));
    EXPECT_TRUE(
        std::ranges::contains(waits, std::to_underlying(Wait::fullSync)));
}

TEST_F(WaitTrackerTest, AddDuplicateWait)
{
    addTrackedWait(Wait::peerConnection);
    addTrackedWait(Wait::peerConnection);

    auto waits = readWaits();
    EXPECT_EQ(waits.size(), 1);
}

TEST_F(WaitTrackerTest, RemoveWait)
{
    addTrackedWait(Wait::siblingRole);
    addTrackedWait(Wait::siblingBMCSteadyState);

    removeTrackedWait(Wait::siblingRole);

    auto waits = readWaits();
    ASSERT_EQ(waits.size(), 1);
    EXPECT_EQ(waits[0], std::to_underlying(Wait::siblingBMCSteadyState));
}

TEST_F(WaitTrackerTest, RemoveLastWait)
{
    addTrackedWait(Wait::systemInventoryPath);
    removeTrackedWait(Wait::systemInventoryPath);

    // Key should be gone entirely, not an empty array
    EXPECT_EQ(data::read<std::vector<std::underlying_type_t<Wait>>>(
                  data::key::trackedWaits),
              std::nullopt);
}

TEST_F(WaitTrackerTest, RemoveNonExistentWait)
{
    EXPECT_NO_THROW(removeTrackedWait(Wait::fullSync));

    addTrackedWait(Wait::startUnit);
    EXPECT_NO_THROW(removeTrackedWait(Wait::fullSync));

    auto waits = readWaits();
    EXPECT_EQ(waits.size(), 1);
}

TEST_F(WaitTrackerTest, ScopeWaitTrackerAddsOnConstruct)
{
    {
        ScopeWaitTracker swt{Wait::siblingAlive};

        auto waits = readWaits();
        ASSERT_EQ(waits.size(), 1);
        EXPECT_EQ(waits[0], std::to_underlying(Wait::siblingAlive));
    }
}

TEST_F(WaitTrackerTest, ScopeWaitTrackerRemovesOnDestruct)
{
    {
        ScopeWaitTracker swt{Wait::siblingAlive};
    }

    // After going out of scope, the key should be gone
    EXPECT_EQ(data::read<std::vector<std::underlying_type_t<Wait>>>(
                  data::key::trackedWaits),
              std::nullopt);
}

TEST_F(WaitTrackerTest, ScopeWaitTrackerMultipleScopes)
{
    {
        ScopeWaitTracker outer{Wait::peerConnection};

        auto waits = readWaits();
        ASSERT_EQ(waits.size(), 1);

        {
            ScopeWaitTracker inner{Wait::fullSync};

            waits = readWaits();
            ASSERT_EQ(waits.size(), 2);
            EXPECT_TRUE(std::ranges::contains(
                waits, std::to_underlying(Wait::peerConnection)));
            EXPECT_TRUE(std::ranges::contains(
                waits, std::to_underlying(Wait::fullSync)));
        }

        // inner gone, outer still present
        waits = readWaits();
        ASSERT_EQ(waits.size(), 1);
        EXPECT_EQ(waits[0], std::to_underlying(Wait::peerConnection));
    }

    // Both gone
    EXPECT_EQ(data::read<std::vector<std::underlying_type_t<Wait>>>(
                  data::key::trackedWaits),
              std::nullopt);
}

TEST_F(WaitTrackerTest, GetTrackedWaitsAsStrings)
{
    // No waits - should return empty vector
    EXPECT_TRUE(getTrackedWaits().empty());

    addTrackedWait(Wait::startUnit);
    addTrackedWait(Wait::siblingAlive);
    addTrackedWait(Wait::fullSync);

    auto waits = getTrackedWaits();
    ASSERT_EQ(waits.size(), 3);
    EXPECT_TRUE(std::ranges::contains(waits, "StartUnit"));
    EXPECT_TRUE(std::ranges::contains(waits, "SiblingAlive"));
    EXPECT_TRUE(std::ranges::contains(waits, "FullSync"));

    // Remove one and verify it's gone from the string list
    removeTrackedWait(Wait::siblingAlive);
    waits = getTrackedWaits();
    ASSERT_EQ(waits.size(), 2);
    EXPECT_FALSE(std::ranges::contains(waits, "SiblingAlive"));
    EXPECT_TRUE(std::ranges::contains(waits, "StartUnit"));
    EXPECT_TRUE(std::ranges::contains(waits, "FullSync"));
}

TEST_F(WaitTrackerTest, RemoveAllTrackedWaits)
{
    // Add several waits
    addTrackedWait(Wait::startUnit);
    addTrackedWait(Wait::siblingAlive);
    addTrackedWait(Wait::fullSync);

    ASSERT_EQ(readWaits().size(), 3);

    // Remove all - key should be gone entirely
    removeAllTrackedWaits();

    EXPECT_EQ(data::read<std::vector<std::underlying_type_t<Wait>>>(
                  data::key::trackedWaits),
              std::nullopt);
}

TEST_F(WaitTrackerTest, RemoveAllTrackedWaitsWhenEmpty)
{
    // Should not crash when there are no tracked waits
    EXPECT_NO_THROW(removeAllTrackedWaits());

    EXPECT_EQ(data::read<std::vector<std::underlying_type_t<Wait>>>(
                  data::key::trackedWaits),
              std::nullopt);
}

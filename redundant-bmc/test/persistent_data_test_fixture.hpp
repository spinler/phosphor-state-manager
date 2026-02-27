// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "persistent_data.hpp"

#include <filesystem>

#include <gtest/gtest.h>

namespace rbmc::test
{

/**
 * @class PersistentDataTestFixture
 *
 * Test fixture to manage the directory used to save
 * persistent data in testcases.
 *
 * Tests that need persistent data functionality should
 * inherit from this.
 */
class PersistentDataTestFixture : public ::testing::Test
{
  protected:
    PersistentDataTestFixture()
    {
        char tempDir[] = "/tmp/rbmc_data_test_XXXXXX";
        dataDir = mkdtemp(tempDir);

        // Set the data directory for the data namespace
        data::setDataDirectory(dataDir);
    }

    ~PersistentDataTestFixture() override
    {
        if (!dataDir.empty())
        {
            std::filesystem::remove_all(dataDir);
        }
    }

    std::filesystem::path dataDir;
};

} // namespace rbmc::test

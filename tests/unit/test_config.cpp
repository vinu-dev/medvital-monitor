/**
 * @file test_config.cpp
 * @brief Unit tests for application configuration persistence.
 *
 * @req SWR-GUI-010  Configuration shall persist across application restarts.
 * @req SWR-GUI-014  Dashboard readability mode shall persist across restarts.
 *
 * All tests redirect I/O to a temporary file via app_config_set_path() so
 * they never touch the production monitor.cfg.  The fixture removes the
 * temporary file in TearDown() to keep the test directory clean.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

extern "C" {
#include "app_config.h"
}

// ---------------------------------------------------------------------------
// Helper: build a temp file path unique to this test run
// ---------------------------------------------------------------------------
static std::string make_temp_path(const std::string &suffix = "")
{
    // Use the system temp directory when available, fall back to "."
    const char *tmp_dir = nullptr;

#ifdef _WIN32
    char tmp_buf[512] = {0};
    DWORD len = GetTempPathA(static_cast<DWORD>(sizeof(tmp_buf)), tmp_buf);
    if (len > 0 && len < sizeof(tmp_buf))
        tmp_dir = tmp_buf;
#else
    tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) tmp_dir = "/tmp";
#endif

    if (!tmp_dir || tmp_dir[0] == '\0')
        tmp_dir = ".";

    std::string path = std::string(tmp_dir) + "/test_monitor" + suffix + ".cfg";
    return path;
}

// ---------------------------------------------------------------------------
// Fixture: sets a unique temp path before each test and removes it after
// ---------------------------------------------------------------------------
class ConfigTest : public ::testing::Test
{
protected:
    std::string temp_path_;

    void SetUp() override
    {
        // Use the test name to make paths unique across parallel runs
        temp_path_ = make_temp_path("_" + std::string(
            ::testing::UnitTest::GetInstance()->current_test_info()->name()));

        // Remove any stale file from a previous crashed run
        std::remove(temp_path_.c_str());

        // Tell the module to use this path
        app_config_set_path(temp_path_.c_str());
    }

    void TearDown() override
    {
        // Remove the temp file if it was created during the test
        std::remove(temp_path_.c_str());

        // Reset the module to default (exe-directory) behaviour
        app_config_set_path(nullptr);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// @req SWR-GUI-010 - When no config file exists, sim_enabled defaults to 1
TEST_F(ConfigTest, DefaultWhenFileAbsent)
{
    // Point the module at a path that is guaranteed not to exist
    std::string nonexistent = temp_path_ + "_nonexistent_xyz";
    std::remove(nonexistent.c_str());
    app_config_set_path(nonexistent.c_str());

    int sim_enabled = 99; // sentinel
    int result = app_config_load(&sim_enabled);

    EXPECT_EQ(0, result)        << "Load should return 0 when file is absent";
    EXPECT_EQ(1, sim_enabled)   << "Default sim_enabled must be 1";

    // Clean up just in case
    std::remove(nonexistent.c_str());
}

// @req SWR-GUI-014 - When no config file exists, readability mode defaults to 0
TEST_F(ConfigTest, ReadabilityModeDefaultsOffWhenFileAbsent)
{
    std::string nonexistent = temp_path_ + "_readability_missing";
    std::remove(nonexistent.c_str());
    app_config_set_path(nonexistent.c_str());

    EXPECT_EQ(0, app_config_load_readability_mode())
        << "Default readability_mode must be 0 when the file is absent";

    std::remove(nonexistent.c_str());
}

// @req SWR-GUI-010 - Save sim_enabled=0 then load, expect 0
TEST_F(ConfigTest, SaveZeroThenLoadReturnsZero)
{
    int save_result = app_config_save(0);
    ASSERT_EQ(1, save_result) << "Save should succeed";

    int sim_enabled = 99;
    int load_result = app_config_load(&sim_enabled);
    EXPECT_EQ(1, load_result)  << "Load should succeed after save";
    EXPECT_EQ(0, sim_enabled)  << "sim_enabled should be 0";
    EXPECT_EQ(0, app_config_load_readability_mode())
        << "Saving sim_enabled must preserve the default readability mode";
}

// @req SWR-GUI-010 - Save sim_enabled=1 then load, expect 1
TEST_F(ConfigTest, SaveOneThenLoadReturnsOne)
{
    int save_result = app_config_save(1);
    ASSERT_EQ(1, save_result) << "Save should succeed";

    int sim_enabled = 99;
    int load_result = app_config_load(&sim_enabled);
    EXPECT_EQ(1, load_result)  << "Load should succeed after save";
    EXPECT_EQ(1, sim_enabled)  << "sim_enabled should be 1";
}

// @req SWR-GUI-010 - Multiple save/load cycles preserve the value correctly
TEST_F(ConfigTest, MultipleSaveLoadCyclesPreserveValue)
{
    struct Cycle { int save_val; int expect_val; };
    const Cycle cycles[] = {
        {1, 1},
        {0, 0},
        {1, 1},
        {0, 0},
        {1, 1},
    };

    for (const auto &c : cycles)
    {
        SCOPED_TRACE("save_val=" + std::to_string(c.save_val));

        ASSERT_EQ(1, app_config_save(c.save_val)) << "Save must succeed";

        int sim_enabled = 99;
        ASSERT_EQ(1, app_config_load(&sim_enabled)) << "Load must succeed";
        EXPECT_EQ(c.expect_val, sim_enabled)
            << "Loaded value must match saved value";
    }
}

// @req SWR-GUI-010 - Overwriting an existing file with a different value works
TEST_F(ConfigTest, OverwriteExistingFile)
{
    // Write 1 first
    ASSERT_EQ(1, app_config_save(1));
    {
        int v = 99;
        ASSERT_EQ(1, app_config_load(&v));
        EXPECT_EQ(1, v);
    }

    // Overwrite with 0
    ASSERT_EQ(1, app_config_save(0));
    {
        int v = 99;
        ASSERT_EQ(1, app_config_load(&v));
        EXPECT_EQ(0, v);
    }
}

// @req SWR-GUI-010 - Load from a malformed file returns default sim_enabled=1
TEST_F(ConfigTest, MalformedFileReturnsDefault)
{
    // Write garbage to the config file
    FILE *f = fopen(temp_path_.c_str(), "w");
    ASSERT_NE(nullptr, f) << "Could not create temp file for test";
    fprintf(f, "this is not a valid config line\n");
    fprintf(f, "another garbage line\n");
    fclose(f);

    int sim_enabled = 99;
    int result = app_config_load(&sim_enabled);

    EXPECT_EQ(0, result)      << "Load should return 0 for malformed file";
    EXPECT_EQ(1, sim_enabled) << "Default sim_enabled=1 when file is malformed";
    EXPECT_EQ(0, app_config_load_readability_mode())
        << "Malformed readability_mode input must fall back to 0";
}

// @req SWR-GUI-010 - Load from an empty file returns default sim_enabled=1
TEST_F(ConfigTest, EmptyFileReturnsDefault)
{
    // Create an empty file
    FILE *f = fopen(temp_path_.c_str(), "w");
    ASSERT_NE(nullptr, f) << "Could not create temp file for test";
    fclose(f);

    int sim_enabled = 99;
    int result = app_config_load(&sim_enabled);

    EXPECT_EQ(0, result)      << "Load should return 0 for empty file";
    EXPECT_EQ(1, sim_enabled) << "Default sim_enabled=1 for empty file";
}

// @req SWR-GUI-010 - File with extra keys before sim_enabled is still parsed
TEST_F(ConfigTest, FileWithExtraKeysBeforeTarget)
{
    FILE *f = fopen(temp_path_.c_str(), "w");
    ASSERT_NE(nullptr, f);
    fprintf(f, "some_other_key=42\n");
    fprintf(f, "sim_enabled=0\n");
    fclose(f);

    int sim_enabled = 99;
    int result = app_config_load(&sim_enabled);

    EXPECT_EQ(1, result)      << "Load should succeed";
    EXPECT_EQ(0, sim_enabled) << "sim_enabled=0 must be parsed even after extra keys";
}

// @req SWR-GUI-010 - app_config_set_path(nullptr) resets to default behaviour
//   (we cannot check the exe-directory path in a unit test, but we verify
//    the call does not crash and a subsequent save/load cycle still works
//    when set back to the temp path)
TEST_F(ConfigTest, SetPathNullResetsThenReapply)
{
    // Reset to default (exe-directory) – must not crash
    app_config_set_path(nullptr);

    // Re-apply our temp path so the fixture TearDown can clean up correctly
    app_config_set_path(temp_path_.c_str());

    ASSERT_EQ(1, app_config_save(1));
    int v = 99;
    ASSERT_EQ(1, app_config_load(&v));
    EXPECT_EQ(1, v);
}

// @req SWR-GUI-010 - Non-boolean values (e.g. 2) are treated as truthy (1)
TEST_F(ConfigTest, NonZeroValueTreatedAsEnabled)
{
    // Write a file manually with sim_enabled=2
    FILE *f = fopen(temp_path_.c_str(), "w");
    ASSERT_NE(nullptr, f);
    fprintf(f, "sim_enabled=2\n");
    fclose(f);

    int sim_enabled = 99;
    int result = app_config_load(&sim_enabled);
    EXPECT_EQ(1, result);
    EXPECT_EQ(1, sim_enabled) << "Any non-zero value should be normalised to 1";
}

// @req SWR-GUI-014 - Saving readability mode preserves the existing sibling preferences
TEST_F(ConfigTest, ReadabilityModeSavePreservesSiblingPreferences)
{
    ASSERT_EQ(1, app_config_save(0));
    ASSERT_EQ(1, app_config_save_language(3));
    ASSERT_EQ(1, app_config_save_readability_mode(1));

    int sim_enabled = 99;
    ASSERT_EQ(1, app_config_load(&sim_enabled));
    EXPECT_EQ(0, sim_enabled);
    EXPECT_EQ(3, app_config_load_language());
    EXPECT_EQ(1, app_config_load_readability_mode());

    ASSERT_EQ(1, app_config_save(1));
    EXPECT_EQ(1, app_config_load_readability_mode())
        << "Saving sim_enabled must not drop readability_mode";
}

// @req SWR-GUI-014 - Invalid persisted readability values fall back to 0
TEST_F(ConfigTest, ReadabilityModeInvalidValueFallsBackToOff)
{
    FILE *f = fopen(temp_path_.c_str(), "w");
    ASSERT_NE(nullptr, f);
    fprintf(f, "sim_enabled=1\n");
    fprintf(f, "language=2\n");
    fprintf(f, "readability_mode=7\n");
    fclose(f);

    int sim_enabled = 99;
    ASSERT_EQ(1, app_config_load(&sim_enabled));
    EXPECT_EQ(1, sim_enabled);
    EXPECT_EQ(2, app_config_load_language());
    EXPECT_EQ(0, app_config_load_readability_mode())
        << "Only explicit 0/1 readability values are accepted";
}

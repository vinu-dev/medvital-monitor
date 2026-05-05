#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

extern "C" {
#include "alarm_limits.h"
#include "session_export.h"
}

static std::string make_temp_path(const std::string &suffix)
{
    const char *tmp_dir = nullptr;

#ifdef _WIN32
    char tmp_buf[512] = {0};
    DWORD len = GetTempPathA(static_cast<DWORD>(sizeof(tmp_buf)), tmp_buf);
    if (len > 0 && len < sizeof(tmp_buf)) {
        tmp_dir = tmp_buf;
    }
#else
    tmp_dir = "/tmp";
#endif

    if (!tmp_dir || tmp_dir[0] == '\0') {
        tmp_dir = ".";
    }

    return std::string(tmp_dir) + "/test_session_export" + suffix + ".txt";
}

static std::string read_text_file(const std::string &path)
{
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

static VitalSigns make_warning_reading()
{
    VitalSigns reading = {108, 148, 94, 37.9f, 93, 23};
    return reading;
}

class SessionExportTest : public ::testing::Test
{
protected:
    std::string temp_path_;
    AlarmLimits limits_;
    PatientRecord patient_;

    void SetUp() override
    {
        temp_path_ = make_temp_path("_" + std::string(
            ::testing::UnitTest::GetInstance()->current_test_info()->name()));
        std::remove(temp_path_.c_str());
        alarm_limits_defaults(&limits_);
        std::memset(&patient_, 0, sizeof(patient_));
    }

    void TearDown() override
    {
        std::remove(temp_path_.c_str());
    }
};

TEST_F(SessionExportTest, SWR_EXP_001_BuildPathUsesDeterministicFilename)
{
    char path[SESSION_EXPORT_PATH_MAX];

    ASSERT_EQ(1, session_export_build_path(1001, nullptr, path, sizeof(path)));
    EXPECT_NE(std::string::npos,
              std::string(path).find("session-review-patient-1001.txt"));
}

TEST_F(SessionExportTest, SWR_EXP_003_RefusesWhenNoPatientIsAdmitted)
{
    char path[SESSION_EXPORT_PATH_MAX];

    EXPECT_EQ(SESSION_EXPORT_RESULT_NO_PATIENT,
              session_export_write_snapshot(&patient_, 0, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            path, sizeof(path)));
    EXPECT_TRUE(read_text_file(temp_path_).empty());
}

TEST_F(SessionExportTest, SWR_EXP_003_RefusesWhenNoReadingsExist)
{
    char path[SESSION_EXPORT_PATH_MAX];

    patient_init(&patient_, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);

    EXPECT_EQ(SESSION_EXPORT_RESULT_NO_READINGS,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            path, sizeof(path)));
    EXPECT_TRUE(read_text_file(temp_path_).empty());
}

TEST_F(SessionExportTest, SWR_EXP_002_WritesRequiredSnapshotSections)
{
    std::string content;
    VitalSigns reading = make_warning_reading();

    patient_init(&patient_, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    ASSERT_EQ(SESSION_EXPORT_RESULT_OK,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            nullptr, 0));

    content = read_text_file(temp_path_);
    EXPECT_NE(std::string::npos, content.find("Session Review Snapshot"));
    EXPECT_NE(std::string::npos, content.find("Format Version: 1.0"));
    EXPECT_NE(std::string::npos, content.find("Patient Demographics"));
    EXPECT_NE(std::string::npos, content.find("Mode Context"));
    EXPECT_NE(std::string::npos, content.find("Alarm Limit Context"));
    EXPECT_NE(std::string::npos, content.find("Latest Vital Signs"));
    EXPECT_NE(std::string::npos, content.find("Overall Status : WARNING"));
    EXPECT_NE(std::string::npos, content.find("Active Alerts"));
    EXPECT_NE(std::string::npos, content.find("Reading History"));
    EXPECT_NE(std::string::npos,
              content.find("#1  HR 108 | BP 148/94 | Temp 37.9 C | SpO2 93% | RR 23 br/min  [WARNING]"));
    EXPECT_NE(std::string::npos, content.find("Retention Boundary"));
}

TEST_F(SessionExportTest, SWR_EXP_003_ExistingFileRequiresExplicitOverwrite)
{
    char path[SESSION_EXPORT_PATH_MAX];
    VitalSigns reading = make_warning_reading();
    std::ofstream existing(temp_path_, std::ios::binary);

    existing << "existing snapshot";
    existing.close();

    patient_init(&patient_, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    EXPECT_EQ(SESSION_EXPORT_RESULT_EXISTS,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            path, sizeof(path)));
    EXPECT_EQ("existing snapshot", read_text_file(temp_path_));
    EXPECT_EQ(temp_path_, std::string(path));
}

TEST_F(SessionExportTest, SWR_EXP_001_OverwriteEnabledReplacesExistingSnapshot)
{
    std::string content;
    VitalSigns reading = make_warning_reading();
    std::ofstream existing(temp_path_, std::ios::binary);

    existing << "stale snapshot";
    existing.close();

    patient_init(&patient_, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    ASSERT_EQ(SESSION_EXPORT_RESULT_OK,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 1,
                                            nullptr, 0));

    content = read_text_file(temp_path_);
    EXPECT_EQ(std::string::npos, content.find("stale snapshot"));
    EXPECT_NE(std::string::npos, content.find("Session Review Snapshot"));
}

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <aclapi.h>
#include <windows.h>
#endif

extern "C" {
#include "alarm_limits.h"
#include "gui_utf8.h"
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

static std::string repeat_utf8_unit(const char *utf8_unit, int count)
{
    std::string out;
    for (int i = 0; i < count; ++i) {
        out += utf8_unit;
    }
    return out;
}

static int read_file_mode_bits(const std::string &path, int *mode_bits_out)
{
    struct stat file_info;

    if (mode_bits_out == nullptr) {
        return 0;
    }

    if (::stat(path.c_str(), &file_info) != 0) {
        return 0;
    }

    *mode_bits_out = file_info.st_mode & 0777;
    return 1;
}

#ifdef _WIN32
static int init_well_known_sid(WELL_KNOWN_SID_TYPE sid_type,
                               BYTE *sid_buffer,
                               DWORD sid_buffer_len,
                               PSID *sid_out)
{
    DWORD sid_len = sid_buffer_len;

    if (sid_buffer == nullptr || sid_out == nullptr) {
        return 0;
    }

    *sid_out = sid_buffer;
    return CreateWellKnownSid(sid_type, nullptr, *sid_out, &sid_len) != 0;
}

static int file_has_restricted_windows_acl(const std::string &path)
{
    BYTE everyone_sid_buffer[SECURITY_MAX_SID_SIZE];
    BYTE users_sid_buffer[SECURITY_MAX_SID_SIZE];
    BYTE auth_users_sid_buffer[SECURITY_MAX_SID_SIZE];
    BYTE admin_sid_buffer[SECURITY_MAX_SID_SIZE];
    BYTE system_sid_buffer[SECURITY_MAX_SID_SIZE];
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR security_descriptor = nullptr;
    PSID owner_sid = nullptr;
    PSID everyone_sid = nullptr;
    PSID users_sid = nullptr;
    PSID auth_users_sid = nullptr;
    PSID admin_sid = nullptr;
    PSID system_sid = nullptr;
    SECURITY_DESCRIPTOR_CONTROL control = 0;
    DWORD revision = 0;
    int owner_has_rw = 0;
    DWORD i;

    if (!init_well_known_sid(WinWorldSid,
                             everyone_sid_buffer, sizeof(everyone_sid_buffer),
                             &everyone_sid) ||
        !init_well_known_sid(WinBuiltinUsersSid,
                             users_sid_buffer, sizeof(users_sid_buffer),
                             &users_sid) ||
        !init_well_known_sid(WinAuthenticatedUserSid,
                             auth_users_sid_buffer, sizeof(auth_users_sid_buffer),
                             &auth_users_sid) ||
        !init_well_known_sid(WinBuiltinAdministratorsSid,
                             admin_sid_buffer, sizeof(admin_sid_buffer),
                             &admin_sid) ||
        !init_well_known_sid(WinLocalSystemSid,
                             system_sid_buffer, sizeof(system_sid_buffer),
                             &system_sid)) {
        return 0;
    }

    if (GetNamedSecurityInfoA(path.c_str(), SE_FILE_OBJECT,
                              DACL_SECURITY_INFORMATION |
                                  OWNER_SECURITY_INFORMATION,
                              &owner_sid, nullptr, &dacl, nullptr,
                              &security_descriptor) != ERROR_SUCCESS ||
        dacl == nullptr ||
        !GetSecurityDescriptorControl(security_descriptor, &control,
                                      &revision) ||
        (control & SE_DACL_PROTECTED) == 0) {
        if (security_descriptor != nullptr) {
            LocalFree(security_descriptor);
        }
        return 0;
    }

    for (i = 0; i < dacl->AceCount; ++i) {
        void *ace = nullptr;
        ACCESS_ALLOWED_ACE *allowed_ace;
        PSID ace_sid;

        if (!GetAce(dacl, i, &ace)) {
            LocalFree(security_descriptor);
            return 0;
        }

        if (((ACE_HEADER *)ace)->AceType != ACCESS_ALLOWED_ACE_TYPE) {
            continue;
        }

        allowed_ace = (ACCESS_ALLOWED_ACE *)ace;
        ace_sid = (PSID)&allowed_ace->SidStart;

        if (EqualSid(ace_sid, everyone_sid) ||
            EqualSid(ace_sid, users_sid) ||
            EqualSid(ace_sid, auth_users_sid)) {
            LocalFree(security_descriptor);
            return 0;
        }

        if (EqualSid(ace_sid, owner_sid)) {
            if ((allowed_ace->Mask & (FILE_GENERIC_READ | FILE_GENERIC_WRITE)) ==
                (FILE_GENERIC_READ | FILE_GENERIC_WRITE)) {
                owner_has_rw = 1;
            }
            continue;
        }

        if (EqualSid(ace_sid, admin_sid) || EqualSid(ace_sid, system_sid)) {
            continue;
        }

        LocalFree(security_descriptor);
        return 0;
    }

    LocalFree(security_descriptor);
    return owner_has_rw;
}
#endif

static int make_file_permissions_permissive(const std::string &path)
{
#ifdef _WIN32
    BYTE everyone_sid_buffer[SECURITY_MAX_SID_SIZE];
    EXPLICIT_ACCESSA entries[2];
    PACL acl = nullptr;
    PSECURITY_DESCRIPTOR security_descriptor = nullptr;
    PSID owner_sid = nullptr;
    PSID everyone_sid = nullptr;
    DWORD result;

    if (!init_well_known_sid(WinWorldSid,
                             everyone_sid_buffer, sizeof(everyone_sid_buffer),
                             &everyone_sid)) {
        return 0;
    }

    result = GetNamedSecurityInfoA(path.c_str(), SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION,
                                   &owner_sid, nullptr, nullptr, nullptr,
                                   &security_descriptor);
    if (result != ERROR_SUCCESS || owner_sid == nullptr) {
        if (security_descriptor != nullptr) {
            LocalFree(security_descriptor);
        }
        return 0;
    }

    std::memset(entries, 0, sizeof(entries));
    entries[0].grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE;
    entries[0].grfAccessMode = SET_ACCESS;
    entries[0].grfInheritance = NO_INHERITANCE;
    BuildTrusteeWithSidA(&entries[0].Trustee, owner_sid);

    entries[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    entries[1].grfAccessMode = SET_ACCESS;
    entries[1].grfInheritance = NO_INHERITANCE;
    BuildTrusteeWithSidA(&entries[1].Trustee, everyone_sid);

    result = SetEntriesInAclA(2, entries, nullptr, &acl);
    if (result == ERROR_SUCCESS) {
        result = SetNamedSecurityInfoA(const_cast<LPSTR>(path.c_str()),
                                       SE_FILE_OBJECT,
                                       DACL_SECURITY_INFORMATION |
                                           PROTECTED_DACL_SECURITY_INFORMATION,
                                       nullptr, nullptr, acl, nullptr);
    }

    if (acl != nullptr) {
        LocalFree(acl);
    }
    if (security_descriptor != nullptr) {
        LocalFree(security_descriptor);
    }

    return result == ERROR_SUCCESS;
#else
    return chmod(path.c_str(), 0666) == 0;
#endif
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

TEST_F(SessionExportTest, SWR_EXP_001_CreatesSnapshotWithRestrictedPermissions)
{
    VitalSigns reading = make_warning_reading();

    patient_init(&patient_, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    ASSERT_EQ(SESSION_EXPORT_RESULT_OK,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            nullptr, 0));
#ifdef _WIN32
    EXPECT_EQ(1, file_has_restricted_windows_acl(temp_path_));
#else
    {
        int mode_bits = 0;
        ASSERT_EQ(1, read_file_mode_bits(temp_path_, &mode_bits));
        EXPECT_EQ(0600, mode_bits);
    }
#endif
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
    std::string patient_name = repeat_utf8_unit("\xC3\xA9", 32);
    std::string expected_name = repeat_utf8_unit("\xC3\xA9", 31);

    patient_init(&patient_, 1001, patient_name.c_str(), 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    ASSERT_EQ(SESSION_EXPORT_RESULT_OK,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            nullptr, 0));

    content = read_text_file(temp_path_);
    EXPECT_NE(std::string::npos, content.find("Session Review Snapshot"));
    EXPECT_NE(std::string::npos, content.find("Format Version: 1.0"));
    EXPECT_NE(std::string::npos, content.find("Patient Demographics"));
    EXPECT_NE(std::string::npos,
              content.find("Name           : " + expected_name));
    EXPECT_NE(std::string::npos, content.find("Mode Context"));
    EXPECT_NE(std::string::npos, content.find("Alarm Limit Context"));
    EXPECT_NE(std::string::npos,
              content.find("Heart Rate     : low 40 bpm / high 150 bpm"));
    EXPECT_NE(std::string::npos, content.find("Latest Vital Signs"));
    EXPECT_NE(std::string::npos, content.find("Heart Rate     : 108 bpm [WARNING]"));
    EXPECT_NE(std::string::npos,
              content.find("Status Basis   : Live dashboard clinical thresholds; configured alarm limits are listed above for session context."));
    EXPECT_NE(std::string::npos, content.find("Overall Status : WARNING"));
    EXPECT_NE(std::string::npos, content.find("Active Alerts"));
    EXPECT_NE(std::string::npos,
              content.find("[WARNING]  Heart rate 108 bpm [normal 60-100]"));
    EXPECT_NE(std::string::npos,
              content.find("[WARNING]  BP 148/94 mmHg [normal 90-140 / 60-90]"));
    EXPECT_NE(std::string::npos,
              content.find("[WARNING]  Temp 37.9 C [normal 36.1-37.2]"));
    EXPECT_NE(std::string::npos,
              content.find("[WARNING]  SpO2 93% [normal 95-100%]"));
    EXPECT_NE(std::string::npos,
              content.find("[WARNING]  RR 23 br/min [normal 12-20]"));
    EXPECT_NE(std::string::npos, content.find("Reading History"));
    EXPECT_NE(std::string::npos,
              content.find("#1  HR 108 | BP 148/94 | Temp 37.9 C | SpO2 93% | RR 23 br/min  [WARNING]"));
    EXPECT_NE(std::string::npos, content.find("Retention Boundary"));
}

#ifdef _WIN32
TEST_F(SessionExportTest, SWR_EXP_002_Win32EditPathPreservesUtf8PatientName)
{
    static const char kInitialName[] =
        "\xE6\x82\xA3\xE8\x80\x85\xE6\x9D\x8E\xE9\x9B\xB7";
    static const char kUpdatedName[] =
        "\xE6\x82\xA3\xE8\x80\x85\xE5\xBC\xA0\xE4\xBC\x9F";
    HINSTANCE instance = GetModuleHandleA(nullptr);
    HWND parent = nullptr;
    HWND edit = nullptr;
    VitalSigns reading = make_warning_reading();
    char gui_name[MAX_NAME_LEN];
    std::string content;

    parent = CreateWindowExA(0, "STATIC", "",
                             WS_OVERLAPPEDWINDOW,
                             0, 0, 320, 120,
                             nullptr, nullptr, instance, nullptr);
    ASSERT_NE(nullptr, parent);

    edit = gui_create_edit_utf8(instance, parent, 7001, kInitialName,
                                WS_EX_CLIENTEDGE,
                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                0, 0, 240, 24);
    ASSERT_NE(nullptr, edit);
    EXPECT_NE(FALSE, IsWindowUnicode(edit));

    ASSERT_GT(gui_get_control_text_utf8(parent, 7001, gui_name, sizeof(gui_name)), 0);
    EXPECT_STREQ(kInitialName, gui_name);

    ASSERT_EQ(1, gui_set_control_text_utf8(parent, 7001, kUpdatedName));
    ASSERT_GT(gui_get_control_text_utf8(parent, 7001, gui_name, sizeof(gui_name)), 0);
    EXPECT_STREQ(kUpdatedName, gui_name);

    patient_init(&patient_, 1001, gui_name, 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    ASSERT_EQ(SESSION_EXPORT_RESULT_OK,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 0,
                                            nullptr, 0));

    content = read_text_file(temp_path_);
    EXPECT_NE(std::string::npos,
              content.find(std::string("Name           : ") + kUpdatedName));

    DestroyWindow(parent);
}
#endif

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

TEST_F(SessionExportTest, SWR_EXP_001_OverwriteEnabledRecreatesRestrictedSnapshot)
{
    std::string content;
    VitalSigns reading = make_warning_reading();
    std::ofstream existing(temp_path_, std::ios::binary);

    existing << "stale snapshot";
    existing.close();
    ASSERT_EQ(1, make_file_permissions_permissive(temp_path_));
#ifdef _WIN32
    EXPECT_EQ(0, file_has_restricted_windows_acl(temp_path_));
#else
    {
        int mode_bits = 0;
        ASSERT_EQ(1, read_file_mode_bits(temp_path_, &mode_bits));
        EXPECT_EQ(0666, mode_bits);
    }
#endif

    patient_init(&patient_, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);
    ASSERT_EQ(1, patient_add_reading(&patient_, &reading));

    ASSERT_EQ(SESSION_EXPORT_RESULT_OK,
              session_export_write_snapshot(&patient_, 1, &limits_,
                                            1, 0, temp_path_.c_str(), 1,
                                            nullptr, 0));

    content = read_text_file(temp_path_);
    EXPECT_EQ(std::string::npos, content.find("stale snapshot"));
    EXPECT_NE(std::string::npos, content.find("Session Review Snapshot"));
#ifdef _WIN32
    EXPECT_EQ(1, file_has_restricted_windows_acl(temp_path_));
#else
    {
        int mode_bits = 0;
        ASSERT_EQ(1, read_file_mode_bits(temp_path_, &mode_bits));
        EXPECT_EQ(0600, mode_bits);
    }
#endif
}

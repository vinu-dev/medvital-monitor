/**
 * @file test_auth.cpp
 * @brief Unit tests for GUI authentication (SWR-GUI-001, SWR-GUI-002) and
 *        multi-user management (SWR-SEC-001..003, SWR-GUI-007).
 *
 * Covers: valid credentials, wrong password, wrong username, empty inputs,
 * null inputs, display name formatting, role detection, password change,
 * admin-set-password, add/remove users.
 *
 * Test naming: TEST(Suite, REQ_XXX_NNN_Description)
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#  include <windows.h>
#endif

extern "C" {
#include "gui_auth.h"
#include "gui_users.h"
}

/* -----------------------------------------------------------------------
 * Helper: get path of users.dat (same logic as gui_users.c get_dat_path)
 * ----------------------------------------------------------------------- */
static void get_test_dat_path(char *out, int out_len)
{
#ifdef _WIN32
    char exe[MAX_PATH];
    char *sep;
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    sep = strrchr(exe, '\\');
    if (sep) *(sep + 1) = '\0'; else exe[0] = '\0';
    snprintf(out, out_len, "%s%s", exe, USERS_DAT_FILENAME);
#else
    snprintf(out, out_len, "%s", USERS_DAT_FILENAME);
#endif
}

/* -----------------------------------------------------------------------
 * UsersTest fixture — removes users.dat then calls users_init() so each
 * test always starts from the compiled-in default credentials.
 * ----------------------------------------------------------------------- */
class UsersTest : public ::testing::Test {
protected:
    void SetUp() override {
        char path[512];
        get_test_dat_path(path, (int)sizeof(path));
        remove(path);   /* ignore error if it doesn't exist */
        users_init();   /* will fall back to defaults */
    }
    void TearDown() override {
        char path[512];
        get_test_dat_path(path, (int)sizeof(path));
        remove(path);   /* clean up after each test */
    }
};

/* -----------------------------------------------------------------------
 * SWR-GUI-001 — auth_validate (backward-compatible; calls users_init via fixture)
 * ----------------------------------------------------------------------- */

TEST_F(UsersTest, REQ_GUI_001_ValidCredentialsReturnOne)
{
    EXPECT_EQ(1, auth_validate("admin", "Monitor@2026"));
}

TEST_F(UsersTest, REQ_GUI_001_WrongPasswordReturnsZero)
{
    EXPECT_EQ(0, auth_validate("admin", "wrongpassword"));
}

TEST_F(UsersTest, REQ_GUI_001_WrongUsernameReturnsZero)
{
    EXPECT_EQ(0, auth_validate("nurse", "Monitor@2026"));
}

TEST_F(UsersTest, REQ_GUI_001_BothWrongReturnsZero)
{
    EXPECT_EQ(0, auth_validate("hacker", "password123"));
}

TEST_F(UsersTest, REQ_GUI_001_EmptyUsernameReturnsZero)
{
    EXPECT_EQ(0, auth_validate("", "Monitor@2026"));
}

TEST_F(UsersTest, REQ_GUI_001_EmptyPasswordReturnsZero)
{
    EXPECT_EQ(0, auth_validate("admin", ""));
}

TEST_F(UsersTest, REQ_GUI_001_BothEmptyReturnsZero)
{
    EXPECT_EQ(0, auth_validate("", ""));
}

TEST_F(UsersTest, REQ_GUI_001_NullUsernameReturnsZero)
{
    EXPECT_EQ(0, auth_validate(nullptr, "Monitor@2026"));
}

TEST_F(UsersTest, REQ_GUI_001_NullPasswordReturnsZero)
{
    EXPECT_EQ(0, auth_validate("admin", nullptr));
}

TEST_F(UsersTest, REQ_GUI_001_BothNullReturnsZero)
{
    EXPECT_EQ(0, auth_validate(nullptr, nullptr));
}

/* -----------------------------------------------------------------------
 * SWR-GUI-002 — auth_display_name
 * ----------------------------------------------------------------------- */

TEST_F(UsersTest, REQ_GUI_002_KnownUsernameReturnsDisplayName)
{
    char buf[64] = {0};
    auth_display_name("admin", buf, (int)sizeof(buf));
    EXPECT_STRNE("", buf);            /* not empty */
    EXPECT_EQ(0, buf[sizeof(buf)-1]); /* null-terminated */
}

TEST_F(UsersTest, REQ_GUI_002_UnknownUsernameReturnsEmpty)
{
    char buf[64];
    memset(buf, 0xFF, sizeof(buf)); /* poison */
    auth_display_name("unknown_user", buf, (int)sizeof(buf));
    EXPECT_EQ('\0', buf[0]);
}

TEST_F(UsersTest, REQ_GUI_002_NullUsernameHandledSafely)
{
    char buf[64] = {0};
    auth_display_name(nullptr, buf, (int)sizeof(buf));
    EXPECT_EQ('\0', buf[0]);
}

TEST_F(UsersTest, REQ_GUI_002_NullOutputHandledSafely)
{
    /* Must not crash */
    auth_display_name("admin", nullptr, 0);
}

TEST_F(UsersTest, REQ_GUI_002_ZeroLengthBufferHandledSafely)
{
    char buf[4] = {'X', 'X', 'X', 'X'};
    /* Should not write anything or crash */
    auth_display_name("admin", buf, 0);
    /* Buffer should be untouched (or written empty) — just must not crash */
}

/* -----------------------------------------------------------------------
 * SWR-SEC-001 / SWR-SEC-002 — users_authenticate + role detection
 * ----------------------------------------------------------------------- */

TEST_F(UsersTest, REQ_SEC_001_AdminAuthSucceeds)
{
    UserRole role = ROLE_CLINICAL;
    int ok = users_authenticate("admin", "Monitor@2026", &role);
    EXPECT_EQ(1, ok);
    EXPECT_EQ(ROLE_ADMIN, role);
}

TEST_F(UsersTest, REQ_SEC_001_ClinicalAuthSucceeds)
{
    UserRole role = ROLE_ADMIN;
    int ok = users_authenticate("clinical", "Clinical@2026", &role);
    EXPECT_EQ(1, ok);
    EXPECT_EQ(ROLE_CLINICAL, role);
}

TEST_F(UsersTest, REQ_SEC_001_WrongPasswordFails)
{
    EXPECT_EQ(0, users_authenticate("admin", "badpassword", nullptr));
}

TEST_F(UsersTest, REQ_SEC_001_UnknownUserFails)
{
    EXPECT_EQ(0, users_authenticate("ghost", "Monitor@2026", nullptr));
}

TEST_F(UsersTest, REQ_SEC_001_NullUsernameSafe)
{
    EXPECT_EQ(0, users_authenticate(nullptr, "Monitor@2026", nullptr));
}

TEST_F(UsersTest, REQ_SEC_001_NullPasswordSafe)
{
    EXPECT_EQ(0, users_authenticate("admin", nullptr, nullptr));
}

TEST_F(UsersTest, REQ_SEC_002_RoleOutNullDoesNotCrash)
{
    /* role_out NULL must not crash */
    int ok = users_authenticate("admin", "Monitor@2026", nullptr);
    EXPECT_EQ(1, ok);
}

/* -----------------------------------------------------------------------
 * SWR-SEC-003 — password change
 * ----------------------------------------------------------------------- */

TEST_F(UsersTest, REQ_SEC_003_ChangePasswordSuccess)
{
    int ok = users_change_password("admin", "Monitor@2026", "NewPass@123");
    EXPECT_EQ(1, ok);
    /* Verify new password works */
    EXPECT_EQ(1, users_authenticate("admin", "NewPass@123", nullptr));
    /* Old password no longer works */
    EXPECT_EQ(0, users_authenticate("admin", "Monitor@2026", nullptr));
}

TEST_F(UsersTest, REQ_SEC_003_ChangePasswordWrongOldFails)
{
    EXPECT_EQ(0, users_change_password("admin", "wrongold", "NewPass@123"));
}

TEST_F(UsersTest, REQ_SEC_003_ChangePasswordTooShortFails)
{
    EXPECT_EQ(0, users_change_password("admin", "Monitor@2026", "short"));
}

TEST_F(UsersTest, REQ_SEC_003_ChangePasswordNullSafe)
{
    EXPECT_EQ(0, users_change_password(nullptr, "Monitor@2026", "NewPass@123"));
    EXPECT_EQ(0, users_change_password("admin", nullptr, "NewPass@123"));
    EXPECT_EQ(0, users_change_password("admin", "Monitor@2026", nullptr));
}

TEST_F(UsersTest, REQ_SEC_003_AdminSetPasswordSuccess)
{
    int ok = users_admin_set_password("clinical", "AdminSet@999");
    EXPECT_EQ(1, ok);
    EXPECT_EQ(1, users_authenticate("clinical", "AdminSet@999", nullptr));
}

TEST_F(UsersTest, REQ_SEC_003_AdminSetPasswordTooShortFails)
{
    EXPECT_EQ(0, users_admin_set_password("clinical", "short"));
}

TEST_F(UsersTest, REQ_SEC_003_AdminSetPasswordUnknownUserFails)
{
    EXPECT_EQ(0, users_admin_set_password("ghost", "LongEnough@123"));
}

TEST_F(UsersTest, REQ_SEC_003_AdminSetPasswordNullSafe)
{
    EXPECT_EQ(0, users_admin_set_password(nullptr, "LongEnough@123"));
    EXPECT_EQ(0, users_admin_set_password("admin", nullptr));
}

/* -----------------------------------------------------------------------
 * SWR-GUI-007 — user management (add / remove / count / get_by_index)
 * ----------------------------------------------------------------------- */

TEST_F(UsersTest, REQ_GUI_007_DefaultCountIsTwo)
{
    EXPECT_EQ(2, users_count());
}

TEST_F(UsersTest, REQ_GUI_007_AddUserSucceeds)
{
    int ok = users_add("nurse1", "Nurse One", "Nursing@2026", ROLE_CLINICAL);
    EXPECT_EQ(1, ok);
    EXPECT_EQ(3, users_count());
    EXPECT_EQ(1, users_authenticate("nurse1", "Nursing@2026", nullptr));
}

TEST_F(UsersTest, REQ_GUI_007_AddDuplicateUserFails)
{
    EXPECT_EQ(0, users_add("admin", "Another Admin", "Monitor@2026", ROLE_ADMIN));
}

TEST_F(UsersTest, REQ_GUI_007_AddUserPasswordTooShortFails)
{
    EXPECT_EQ(0, users_add("newuser", "New User", "short", ROLE_CLINICAL));
}

TEST_F(UsersTest, REQ_GUI_007_RemoveClinicianSucceeds)
{
    int ok = users_remove("clinical");
    EXPECT_EQ(1, ok);
    EXPECT_EQ(1, users_count());
    EXPECT_EQ(0, users_authenticate("clinical", "Clinical@2026", nullptr));
}

TEST_F(UsersTest, REQ_GUI_007_RemoveLastAdminFails)
{
    /* Remove clinical user first, then try to remove the last admin */
    users_remove("clinical");
    int ok = users_remove("admin");
    EXPECT_EQ(0, ok);   /* Must not succeed — need at least 1 admin */
    EXPECT_EQ(1, users_count());
}

TEST_F(UsersTest, REQ_GUI_007_GetByIndexReturnsCorrectAccount)
{
    UserAccount acct;
    memset(&acct, 0, sizeof(acct));
    int ok = users_get_by_index(0, &acct);
    EXPECT_EQ(1, ok);
    EXPECT_STRNE("", acct.username);
}

TEST_F(UsersTest, REQ_GUI_007_GetByIndexOutOfRangeFails)
{
    UserAccount acct;
    EXPECT_EQ(0, users_get_by_index(100, &acct));
    EXPECT_EQ(0, users_get_by_index(-1,  &acct));
}

/* -----------------------------------------------------------------------
 * SWR-SEC-004 — Password hashing: never store plaintext
 * ----------------------------------------------------------------------- */

TEST_F(UsersTest, REQ_SEC_004_StoredValueIsNotPlaintext)
{
    /* The password_hash field must never equal the plaintext credential. */
    UserAccount acct;
    memset(&acct, 0, sizeof(acct));
    users_get_by_index(0, &acct);
    EXPECT_STRNE("Monitor@2026", acct.password_hash);
}

TEST_F(UsersTest, REQ_SEC_004_StoredValueIsSHA256Hex)
{
    /* SHA-256 hex digest is exactly 64 lowercase hex characters. */
    UserAccount acct;
    int i;
    memset(&acct, 0, sizeof(acct));
    users_get_by_index(0, &acct);
    EXPECT_EQ(64, (int)strlen(acct.password_hash));
    for (i = 0; i < 64; ++i) {
        char c = acct.password_hash[i];
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character at index " << i << ": '" << c << "'";
    }
}

TEST_F(UsersTest, REQ_SEC_004_AuthStillWorksAfterHashing)
{
    /* Verify hashing is transparent: plaintext credentials passed to
     * users_authenticate() still resolve correctly. */
    EXPECT_EQ(1, users_authenticate("admin",    "Monitor@2026",  nullptr));
    EXPECT_EQ(1, users_authenticate("clinical", "Clinical@2026", nullptr));
    EXPECT_EQ(0, users_authenticate("admin",    "Clinical@2026", nullptr));
}

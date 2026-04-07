/**
 * @file gui_users.h
 * @brief Multi-user account management — Patient Vital Signs Monitor.
 *
 * Static array storage only — no heap allocation (IEC 62304 SYS-012).
 * Up to 8 simultaneous accounts. Persistence via plain-text pipe-delimited
 * file "users.dat" in the executable directory.
 *
 * @req SWR-SEC-001
 * @req SWR-SEC-002
 * @req SWR-SEC-003
 */
#ifndef GUI_USERS_H
#define GUI_USERS_H

#ifdef __cplusplus
extern "C" {
#endif

#define USERS_MAX_ACCOUNTS     8
#define USERS_MAX_USERNAME_LEN 64
#define USERS_MAX_DISPNAME_LEN 64
#define USERS_MIN_PASSWORD_LEN  8
#define USERS_MAX_PASSWORD_LEN  64   /**< Max plaintext password input length */
#define USERS_DAT_FILENAME      "users.dat"

#include "pw_hash.h"   /* PW_HASH_HEX_LEN = 65 */

typedef enum { ROLE_ADMIN = 0, ROLE_CLINICAL = 1 } UserRole;

typedef struct {
    char     username[USERS_MAX_USERNAME_LEN];
    char     display_name[USERS_MAX_DISPNAME_LEN];
    char     password_hash[PW_HASH_HEX_LEN]; /**< SHA-256 hex digest — never plaintext */
    UserRole role;
    int      active;
} UserAccount;

/** Load from users.dat; fall back to built-in defaults if absent. @req SWR-SEC-001 */
void users_init(void);

/** Authenticate. Writes role to *role_out on success if non-NULL. Returns 1/0. @req SWR-SEC-001 */
int  users_authenticate(const char *username, const char *password, UserRole *role_out);

/** Copy display name for username into out[out_len]. @req SWR-SEC-001 */
void users_display_name_for(const char *username, char *out, int out_len);

/** Change own password (requires correct old_password). Returns 1/0. @req SWR-SEC-003 */
int  users_change_password(const char *username, const char *old_password, const char *new_password);

/** Admin: set any user's password without old password. Returns 1/0. @req SWR-SEC-003 @req SWR-GUI-007 */
int  users_admin_set_password(const char *username, const char *new_password);

/** Add new account. Returns 1 on success, 0 if full/duplicate/invalid. @req SWR-GUI-007 */
int  users_add(const char *username, const char *display_name, const char *password, UserRole role);

/** Remove account. Enforces minimum-one-admin invariant. Returns 1/0. @req SWR-GUI-007 */
int  users_remove(const char *username);

/** Return count of active accounts. @req SWR-GUI-007 */
int  users_count(void);

/** Copy account at logical index idx into *out. Returns 1/0. @req SWR-GUI-007 */
int  users_get_by_index(int idx, UserAccount *out);

/** Persist account list to users.dat. Returns 1/0. @req SWR-SEC-001 */
int  users_save(void);

#ifdef __cplusplus
}
#endif

#endif /* GUI_USERS_H */

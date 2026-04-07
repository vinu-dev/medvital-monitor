/**
 * @file gui_users.c
 * @brief Multi-user account management implementation.
 *
 * Security properties:
 *   - Passwords are NEVER stored or compared in plaintext.
 *     pw_hash() (SHA-256) is applied before every store or compare.
 *   - users.dat is created with owner-read/write-only permissions via
 *     _sopen_s(_S_IREAD|_S_IWRITE) — not world-writable.
 *   - Static array storage only — no heap allocation (IEC 62304 SYS-012).
 *   - File format version "v2" — incompatible with v1 (plaintext) files.
 *
 * @req SWR-SEC-001
 * @req SWR-SEC-002
 * @req SWR-SEC-003
 * @req SWR-SEC-004
 * @req SWR-GUI-007
 */
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "gui_users.h"
#include "pw_hash.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <share.h>
#include <windows.h>

/* Sharing-mode constants — defined in <share.h> on MSVC; provide fallbacks
 * for older MinGW versions that may not expose them. */
#ifndef _SH_DENYRW
#  define _SH_DENYRW 0x10   /* deny read+write access to other processes */
#endif
#ifndef _SH_DENYNO
#  define _SH_DENYNO 0x40   /* permit read+write access by other processes */
#endif

/* -----------------------------------------------------------------------
 * Static storage
 * ----------------------------------------------------------------------- */
static UserAccount g_users[USERS_MAX_ACCOUNTS];
static int         g_user_count = 0;

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */
static void safe_copy(char *dst, const char *src, int max_len)
{
    strncpy(dst, src, (size_t)(max_len - 1));
    dst[max_len - 1] = '\0';
}

static void get_dat_path(char *out, int out_len)
{
    char  exe[MAX_PATH];
    char *sep;
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    sep = strrchr(exe, '\\');
    if (sep) *(sep + 1) = '\0'; else exe[0] = '\0';
    snprintf(out, (size_t)out_len, "%s%s", exe, USERS_DAT_FILENAME);
}

/**
 * Open a file for writing with owner-read/write-only permissions.
 * Uses _sopen_s with _S_IREAD|_S_IWRITE (equivalent to POSIX 0600)
 * to prevent world-writable file creation (CWE-732).
 */
static FILE *open_write_restricted(const char *path)
{
    int  fd = -1;
    FILE *f;
    /* _S_IREAD | _S_IWRITE — owner read+write only.
     * _SH_DENYRW             — deny other processes while file is open. */
    if (_sopen_s(&fd, path,
                 _O_CREAT | _O_WRONLY | _O_TRUNC,
                 _SH_DENYRW,
                 _S_IREAD | _S_IWRITE) != 0 || fd == -1)
        return NULL;
    f = _fdopen(fd, "w");
    if (!f) { _close(fd); return NULL; }
    return f;
}

static void load_defaults(void)
{
    g_user_count = 0;
    memset(g_users, 0, sizeof(g_users));

    safe_copy(g_users[0].username,     "admin",         USERS_MAX_USERNAME_LEN);
    safe_copy(g_users[0].display_name, "Administrator", USERS_MAX_DISPNAME_LEN);
    pw_hash(g_users[0].password_hash, "Monitor@2026");
    g_users[0].role   = ROLE_ADMIN;
    g_users[0].active = 1;

    safe_copy(g_users[1].username,     "clinical",      USERS_MAX_USERNAME_LEN);
    safe_copy(g_users[1].display_name, "Clinical User", USERS_MAX_DISPNAME_LEN);
    pw_hash(g_users[1].password_hash, "Clinical@2026");
    g_users[1].role   = ROLE_CLINICAL;
    g_users[1].active = 1;

    g_user_count = 2;
}

static UserAccount *find_user(const char *username)
{
    int i;
    if (!username) return NULL;
    for (i = 0; i < USERS_MAX_ACCOUNTS; ++i) {
        if (g_users[i].active &&
            strcmp(g_users[i].username, username) == 0)
            return &g_users[i];
    }
    return NULL;
}

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */
void users_init(void)
{
    char  path[MAX_PATH];
    char  line[256];
    FILE *f;
    int   parsed = 0;

    get_dat_path(path, MAX_PATH);
    f = fopen(path, "r");
    if (!f) { load_defaults(); return; }

    memset(g_users, 0, sizeof(g_users));
    g_user_count = 0;

    while (fgets(line, (int)sizeof(line), f) &&
           g_user_count < USERS_MAX_ACCOUNTS) {

        const char *u, *d, *h, *r_str;
        int   role_val;
        char *nl = strrchr(line, '\n');
        if (nl) *nl = '\0';

        /* Skip comments and blank lines; reject v1 (plaintext) header */
        if (line[0] == '#' || line[0] == '\0') continue;

        u     = strtok(line, "|");
        d     = strtok(NULL, "|");
        h     = strtok(NULL, "|");   /* SHA-256 hex digest (64 chars) */
        r_str = strtok(NULL, "|");

        if (!u || !d || !h || !r_str) continue;
        /* Validate: hash field must be exactly 64 hex chars (v2 format) */
        if (strlen(h) != 64u) continue;
        role_val = atoi(r_str);
        if (role_val != 0 && role_val != 1) continue;
        if (strlen(u) == 0u) continue;

        safe_copy(g_users[g_user_count].username,     u, USERS_MAX_USERNAME_LEN);
        safe_copy(g_users[g_user_count].display_name, d, USERS_MAX_DISPNAME_LEN);
        safe_copy(g_users[g_user_count].password_hash, h, PW_HASH_HEX_LEN);
        g_users[g_user_count].role   = (UserRole)role_val;
        g_users[g_user_count].active = 1;
        ++g_user_count;
        ++parsed;
    }
    fclose(f);

    if (parsed == 0) load_defaults();
}

int users_authenticate(const char *username, const char *password,
                       UserRole *role_out)
{
    char candidate[PW_HASH_HEX_LEN];
    const UserAccount *u;

    if (!username || !password) return 0;
    u = find_user(username);   /* const: we only read from *u here */
    if (!u) return 0;

    pw_hash(candidate, password);
    if (strcmp(u->password_hash, candidate) != 0) return 0;

    if (role_out) *role_out = u->role;
    return 1;
}

void users_display_name_for(const char *username, char *out, int out_len)
{
    const UserAccount *u;
    if (!out || out_len <= 0) return;
    out[0] = '\0';
    if (!username) return;
    u = find_user(username);
    if (u) snprintf(out, (size_t)out_len, "%s", u->display_name);
}

int users_change_password(const char *username, const char *old_password,
                           const char *new_password)
{
    char candidate[PW_HASH_HEX_LEN];
    UserAccount *u;

    if (!username || !old_password || !new_password) return 0;
    if ((int)strlen(new_password) < USERS_MIN_PASSWORD_LEN) return 0;

    u = find_user(username);
    if (!u) return 0;

    /* Verify current password by comparing hashes */
    pw_hash(candidate, old_password);
    if (strcmp(u->password_hash, candidate) != 0) return 0;

    pw_hash(u->password_hash, new_password);
    users_save();
    return 1;
}

int users_admin_set_password(const char *username, const char *new_password)
{
    UserAccount *u;
    if (!username || !new_password) return 0;
    if ((int)strlen(new_password) < USERS_MIN_PASSWORD_LEN) return 0;
    u = find_user(username);
    if (!u) return 0;
    pw_hash(u->password_hash, new_password);
    users_save();
    return 1;
}

int users_add(const char *username, const char *display_name,
               const char *password, UserRole role)
{
    int i, free_slot = -1;
    if (!username || !display_name || !password) return 0;
    if (strlen(username) == 0u ||
        strlen(username) >= (size_t)USERS_MAX_USERNAME_LEN) return 0;
    if ((int)strlen(password) < USERS_MIN_PASSWORD_LEN) return 0;
    if (find_user(username)) return 0;          /* duplicate */
    if (g_user_count >= USERS_MAX_ACCOUNTS) return 0;

    for (i = 0; i < USERS_MAX_ACCOUNTS; ++i) {
        if (!g_users[i].active) { free_slot = i; break; }
    }
    if (free_slot < 0) return 0;

    memset(&g_users[free_slot], 0, sizeof(UserAccount));
    safe_copy(g_users[free_slot].username,     username,     USERS_MAX_USERNAME_LEN);
    safe_copy(g_users[free_slot].display_name, display_name, USERS_MAX_DISPNAME_LEN);
    pw_hash(g_users[free_slot].password_hash, password);
    g_users[free_slot].role   = role;
    g_users[free_slot].active = 1;
    ++g_user_count;
    users_save();
    return 1;
}

int users_remove(const char *username)
{
    UserAccount *u;
    int i, admin_count = 0;

    if (!username) return 0;
    u = find_user(username);
    if (!u) return 0;

    for (i = 0; i < USERS_MAX_ACCOUNTS; ++i) {
        if (g_users[i].active &&
            g_users[i].role == ROLE_ADMIN &&
            &g_users[i] != u)
            ++admin_count;
    }
    if (u->role == ROLE_ADMIN && admin_count == 0) return 0;

    u->active = 0;
    --g_user_count;
    users_save();
    return 1;
}

int users_count(void) { return g_user_count; }

int users_get_by_index(int idx, UserAccount *out)
{
    int i, seen = 0;
    if (!out || idx < 0) return 0;
    for (i = 0; i < USERS_MAX_ACCOUNTS; ++i) {
        if (!g_users[i].active) continue;
        if (seen == idx) { *out = g_users[i]; return 1; }
        ++seen;
    }
    return 0;
}

int users_save(void)
{
    char  path[MAX_PATH];
    FILE *f;
    int   i;

    get_dat_path(path, MAX_PATH);

    /* Create with owner-only permissions (CWE-732 mitigation) */
    f = open_write_restricted(path);
    if (!f) return 0;

    /* v2 header — passwords are SHA-256 hex digests, not plaintext */
    fprintf(f, "# Patient Monitor Users v2 (passwords: SHA-256)\n");
    for (i = 0; i < USERS_MAX_ACCOUNTS; ++i) {
        if (!g_users[i].active) continue;
        fprintf(f, "%s|%s|%s|%d\n",
                g_users[i].username,
                g_users[i].display_name,
                g_users[i].password_hash,
                (int)g_users[i].role);
    }
    fclose(f);
    return 1;
}

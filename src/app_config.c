/**
 * @file app_config.c
 * @brief Implementation of application configuration persistence.
 *
 * The configuration file is a simple key=value text file:
 *
 *   sim_enabled=1
 *   language=0
 *   idle_timeout_minutes=5
 *
 * File location (in priority order):
 *  1. Path supplied by app_config_set_path() (non-NULL).
 *  2. Directory of the running executable + "/" + APP_CFG_FILENAME,
 *     resolved via GetModuleFileNameA().
 *
 * @req SWR-GUI-010
 */

#include "app_config.h"
#include "session_timeout.h"

#include <stdio.h>
#include <string.h>

/* Windows header for GetModuleFileNameA */
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

/* Maximum path length we will handle */
#define CFG_MAX_PATH 512

/* When non-empty this overrides the exe-derived path (set by tests). */
static char s_override_path[CFG_MAX_PATH] = {0};

typedef struct
{
    int sim_enabled;
    int language;
    int idle_timeout_minutes;
} AppConfigData;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/**
 * Populate @p buf with the path to use for the configuration file.
 * Returns 1 on success, 0 on failure (buf is left empty on failure).
 */
static int get_cfg_path(char *buf, size_t buf_size)
{
    /* Test override takes priority */
    if (s_override_path[0] != '\0')
    {
        strncpy(buf, s_override_path, buf_size - 1u);
        buf[buf_size - 1u] = '\0';
        return 1;
    }

#ifdef _WIN32
    /* Derive path from executable location */
    char exe_path[CFG_MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, exe_path, (DWORD)(sizeof(exe_path) - 1u));
    if (len == 0u || len >= (DWORD)(sizeof(exe_path) - 1u))
        return 0;

    /* Strip the executable filename, keep the directory separator */
    char *last_sep = NULL;
    char *p = exe_path;
    while (*p)
    {
        if (*p == '\\' || *p == '/')
            last_sep = p;
        ++p;
    }

    if (last_sep != NULL)
        *(last_sep + 1) = '\0'; /* truncate after the last separator */
    else
        exe_path[0] = '\0';    /* no separator – use CWD fallback below */

    int written = snprintf(buf, buf_size, "%s%s", exe_path, APP_CFG_FILENAME);
    return (written > 0 && (size_t)written < buf_size) ? 1 : 0;
#else
    /* Non-Windows fallback: place the file in the current directory */
    int written = snprintf(buf, buf_size, "%s", APP_CFG_FILENAME);
    return (written > 0 && (size_t)written < buf_size) ? 1 : 0;
#endif
}

static void app_config_defaults(AppConfigData *cfg)
{
    cfg->sim_enabled = 1;
    cfg->language = 0;
    cfg->idle_timeout_minutes = SESSION_TIMEOUT_DEFAULT_MINUTES;
}

static int parse_named_int(const char *line, const char *key, int *value_out)
{
    const size_t key_len = strlen(key);
    char trailing[2];
    int value = 0;

    if (strncmp(line, key, key_len) != 0 || line[key_len] != '=')
        return 0;

    if (sscanf(line + key_len + 1u, "%d%1s", &value, trailing) == 1)
    {
        *value_out = value;
        return 1;
    }

    return -1;
}

static int app_config_read(AppConfigData *cfg,
                           int *sim_found,
                           int *lang_found,
                           int *idle_timeout_found)
{
    char cfg_path[CFG_MAX_PATH] = {0};
    FILE *f = NULL;
    char line[128];

    if (sim_found != NULL)
        *sim_found = 0;
    if (lang_found != NULL)
        *lang_found = 0;
    if (idle_timeout_found != NULL)
        *idle_timeout_found = 0;

    if (cfg == NULL)
        return 0;

    app_config_defaults(cfg);

    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    f = fopen(cfg_path, "r");
    if (f == NULL)
        return 0;

    while (fgets(line, (int)sizeof(line), f) != NULL)
    {
        int value = 0;
        int rc = parse_named_int(line, "sim_enabled", &value);
        if (rc != 0)
        {
            if (rc > 0)
            {
                cfg->sim_enabled = (value != 0) ? 1 : 0;
                if (sim_found != NULL)
                    *sim_found = 1;
            }
            continue;
        }

        rc = parse_named_int(line, "language", &value);
        if (rc != 0)
        {
            if (rc > 0)
            {
                cfg->language = (value >= 0 && value < 4) ? value : 0;
                if (lang_found != NULL)
                    *lang_found = 1;
            }
            continue;
        }

        rc = parse_named_int(line, "idle_timeout_minutes", &value);
        if (rc != 0)
        {
            if (rc > 0)
            {
                cfg->idle_timeout_minutes =
                    session_timeout_normalize_minutes(value);
                if (idle_timeout_found != NULL &&
                    session_timeout_is_valid_minutes(value))
                {
                    *idle_timeout_found = 1;
                }
            }
            continue;
        }
    }

    fclose(f);
    return 1;
}

static int app_config_write(const AppConfigData *cfg)
{
    char cfg_path[CFG_MAX_PATH] = {0};
    FILE *f = NULL;
    int ok = 0;

    if (cfg == NULL || !get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    f = fopen(cfg_path, "w");
    if (f == NULL)
        return 0;

    ok = (fprintf(f,
                  "sim_enabled=%d\nlanguage=%d\nidle_timeout_minutes=%d\n",
                  cfg->sim_enabled ? 1 : 0,
                  cfg->language,
                  session_timeout_normalize_minutes(cfg->idle_timeout_minutes)) > 0);
    fclose(f);
    return ok ? 1 : 0;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void app_config_set_path(const char *path)
{
    if (path == NULL)
    {
        s_override_path[0] = '\0';
    }
    else
    {
        strncpy(s_override_path, path, CFG_MAX_PATH - 1u);
        s_override_path[CFG_MAX_PATH - 1u] = '\0';
    }
}

int app_config_load(int *sim_enabled_out)
{
    AppConfigData cfg;
    int sim_found = 0;

    if (sim_enabled_out == NULL)
        return 0;

    (void)app_config_read(&cfg, &sim_found, NULL, NULL);
    *sim_enabled_out = cfg.sim_enabled;
    return sim_found ? 1 : 0;
}

int app_config_save(int sim_enabled)
{
    AppConfigData cfg;

    (void)app_config_read(&cfg, NULL, NULL, NULL);
    cfg.sim_enabled = sim_enabled ? 1 : 0;
    return app_config_write(&cfg);
}

int app_config_load_language(void)
{
    AppConfigData cfg;

    (void)app_config_read(&cfg, NULL, NULL, NULL);
    return cfg.language;
}

int app_config_save_language(int language)
{
    AppConfigData cfg;

    (void)app_config_read(&cfg, NULL, NULL, NULL);
    cfg.language = (language >= 0 && language < 4) ? language : 0;
    return app_config_write(&cfg);
}

int app_config_load_idle_timeout_minutes(void)
{
    AppConfigData cfg;

    (void)app_config_read(&cfg, NULL, NULL, NULL);
    return cfg.idle_timeout_minutes;
}

int app_config_save_idle_timeout_minutes(int minutes)
{
    AppConfigData cfg;

    (void)app_config_read(&cfg, NULL, NULL, NULL);
    cfg.idle_timeout_minutes = session_timeout_normalize_minutes(minutes);
    return app_config_write(&cfg);
}

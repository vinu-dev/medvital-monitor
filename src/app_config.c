/**
 * @file app_config.c
 * @brief Implementation of application configuration persistence.
 *
 * The configuration file is a simple key=value text file:
 *
 *   sim_enabled=1
 *
 * File location (in priority order):
 *  1. Path supplied by app_config_set_path() (non-NULL).
 *  2. Directory of the running executable + "/" + APP_CFG_FILENAME,
 *     resolved via GetModuleFileNameA().
 *
 * @req SWR-GUI-010
 */

#include "app_config.h"

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
    if (sim_enabled_out == NULL)
        return 0;

    /* Apply default before attempting to read */
    *sim_enabled_out = 1;

    char cfg_path[CFG_MAX_PATH] = {0};
    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    FILE *f = fopen(cfg_path, "r");
    if (f == NULL)
        return 0; /* file absent – caller gets the default */

    char line[128];
    int parsed = 0;
    while (fgets(line, (int)sizeof(line), f) != NULL)
    {
        int value = 0;
        if (sscanf(line, "sim_enabled=%d", &value) == 1)
        {
            *sim_enabled_out = (value != 0) ? 1 : 0;
            parsed = 1;
            break;
        }
    }

    fclose(f);

    if (!parsed)
    {
        /* File exists but does not contain a recognisable value */
        *sim_enabled_out = 1;
        return 0;
    }

    return 1;
}

int app_config_save(int sim_enabled)
{
    /* Preserve language setting when saving sim_enabled */
    int lang = app_config_load_language();

    char cfg_path[CFG_MAX_PATH] = {0};
    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    FILE *f = fopen(cfg_path, "w");
    if (f == NULL)
        return 0;

    int ok = (fprintf(f, "sim_enabled=%d\nlanguage=%d\n",
                       sim_enabled ? 1 : 0, lang) > 0);
    fclose(f);
    return ok ? 1 : 0;
}

int app_config_load_language(void)
{
    char cfg_path[CFG_MAX_PATH] = {0};
    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    FILE *f = fopen(cfg_path, "r");
    if (f == NULL)
        return 0;

    char line[128];
    int lang = 0;
    while (fgets(line, (int)sizeof(line), f) != NULL)
    {
        int value = 0;
        if (sscanf(line, "language=%d", &value) == 1)
        {
            if (value >= 0 && value < 4)
                lang = value;
            break;
        }
    }
    fclose(f);
    return lang;
}

int app_config_save_language(int language)
{
    /* Preserve sim_enabled setting when saving language */
    int sim = 1;
    app_config_load(&sim);

    if (language < 0 || language > 3)
        language = 0;

    char cfg_path[CFG_MAX_PATH] = {0};
    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    FILE *f = fopen(cfg_path, "w");
    if (f == NULL)
        return 0;

    int ok = (fprintf(f, "sim_enabled=%d\nlanguage=%d\n",
                       sim, language) > 0);
    fclose(f);
    return ok ? 1 : 0;
}

/**
 * @file app_config.c
 * @brief Implementation of application configuration persistence.
 *
 * The configuration file is a simple key=value text file:
 *
 *   sim_enabled=1
 *   language=0
 *   readability_mode=0
 *
 * File location (in priority order):
 *  1. Path supplied by app_config_set_path() (non-NULL).
 *  2. Directory of the running executable + "/" + APP_CFG_FILENAME,
 *     resolved via GetModuleFileNameA().
 *
 * @req SWR-GUI-010
 * @req SWR-GUI-012
 * @req SWR-GUI-014
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

typedef struct
{
    int sim_enabled;
    int language;
    int readability_mode;
    int has_sim_enabled;
    int has_language;
    int has_readability_mode;
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
    if (s_override_path[0] != '\0')
    {
        int written = snprintf(buf, buf_size, "%s", s_override_path);
        return (written > 0 && (size_t)written < buf_size) ? 1 : 0;
    }

#ifdef _WIN32
    {
        char exe_path[CFG_MAX_PATH] = {0};
        char *last_sep = NULL;
        char *p = exe_path;
        DWORD len = GetModuleFileNameA(NULL, exe_path, (DWORD)(sizeof(exe_path) - 1u));
        int written;

        if (len == 0u || len >= (DWORD)(sizeof(exe_path) - 1u))
            return 0;

        while (*p)
        {
            if (*p == '\\' || *p == '/')
                last_sep = p;
            ++p;
        }

        if (last_sep != NULL)
            *(last_sep + 1) = '\0';
        else
            exe_path[0] = '\0';

        written = snprintf(buf, buf_size, "%s%s", exe_path, APP_CFG_FILENAME);
        return (written > 0 && (size_t)written < buf_size) ? 1 : 0;
    }
#else
    {
        int written = snprintf(buf, buf_size, "%s", APP_CFG_FILENAME);
        return (written > 0 && (size_t)written < buf_size) ? 1 : 0;
    }
#endif
}

static void config_defaults(AppConfigData *cfg)
{
    cfg->sim_enabled = 1;
    cfg->language = 0;
    cfg->readability_mode = 0;
    cfg->has_sim_enabled = 0;
    cfg->has_language = 0;
    cfg->has_readability_mode = 0;
}

static int normalize_language(int language)
{
    return (language >= 0 && language < 4) ? language : 0;
}

static int load_config_file(AppConfigData *cfg)
{
    char cfg_path[CFG_MAX_PATH] = {0};
    char line[128];
    FILE *f;

    config_defaults(cfg);

    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    f = fopen(cfg_path, "r");
    if (f == NULL)
        return 0;

    while (fgets(line, (int)sizeof(line), f) != NULL)
    {
        int value = 0;

        if (sscanf(line, "sim_enabled=%d", &value) == 1)
        {
            cfg->sim_enabled = (value != 0) ? 1 : 0;
            cfg->has_sim_enabled = 1;
            continue;
        }

        if (sscanf(line, "language=%d", &value) == 1)
        {
            cfg->language = normalize_language(value);
            cfg->has_language = (value >= 0 && value < 4) ? 1 : 0;
            continue;
        }

        if (sscanf(line, "readability_mode=%d", &value) == 1)
        {
            if (value == 0 || value == 1)
            {
                cfg->readability_mode = value;
                cfg->has_readability_mode = 1;
            }
            else
            {
                cfg->readability_mode = 0;
                cfg->has_readability_mode = 0;
            }
        }
    }

    fclose(f);
    return cfg->has_sim_enabled || cfg->has_language || cfg->has_readability_mode;
}

static int save_config_file(const AppConfigData *cfg)
{
    char cfg_path[CFG_MAX_PATH] = {0};
    FILE *f;
    int ok;

    if (!get_cfg_path(cfg_path, sizeof(cfg_path)))
        return 0;

    f = fopen(cfg_path, "w");
    if (f == NULL)
        return 0;

    /* Rewrite the full file so sibling preferences are never dropped. */
    ok = (fprintf(f,
                  "sim_enabled=%d\nlanguage=%d\nreadability_mode=%d\n",
                  cfg->sim_enabled ? 1 : 0,
                  normalize_language(cfg->language),
                  cfg->readability_mode ? 1 : 0) > 0);
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

    if (sim_enabled_out == NULL)
        return 0;

    load_config_file(&cfg);
    *sim_enabled_out = cfg.sim_enabled;
    return cfg.has_sim_enabled ? 1 : 0;
}

int app_config_save(int sim_enabled)
{
    AppConfigData cfg;

    load_config_file(&cfg);
    cfg.sim_enabled = sim_enabled ? 1 : 0;
    return save_config_file(&cfg);
}

int app_config_load_language(void)
{
    AppConfigData cfg;

    load_config_file(&cfg);
    return cfg.language;
}

int app_config_save_language(int language)
{
    AppConfigData cfg;

    load_config_file(&cfg);
    cfg.language = normalize_language(language);
    return save_config_file(&cfg);
}

int app_config_load_readability_mode(void)
{
    AppConfigData cfg;

    load_config_file(&cfg);
    return cfg.readability_mode;
}

int app_config_save_readability_mode(int enabled)
{
    AppConfigData cfg;

    load_config_file(&cfg);
    cfg.readability_mode = (enabled == 1) ? 1 : 0;
    return save_config_file(&cfg);
}

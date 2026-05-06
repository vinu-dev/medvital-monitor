/**
 * @file app_config.h
 * @brief Application configuration persistence interface.
 *
 * Provides save/load of run-time configuration (simulation mode,
 * language selection, and dashboard readability mode) to/from a
 * plain-text file named APP_CFG_FILENAME.
 * The file is located in the same directory as the running executable
 * unless overridden via app_config_set_path() (intended for unit tests).
 *
 * @req SWR-GUI-010  Configuration shall persist across application restarts.
 * @req SWR-GUI-012
 * @req SWR-GUI-014
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Default configuration file name (placed next to the executable). */
#define APP_CFG_FILENAME "monitor.cfg"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Override the configuration file path.
 *
 * When @p path is non-NULL the module uses that exact path instead of
 * deriving one from the executable location.  Pass NULL to revert to
 * the default (exe-directory) behaviour.
 *
 * This function exists primarily to support unit testing so that tests
 * can redirect reads and writes to a temporary location without touching
 * the production configuration file.
 *
 * @param path  Absolute or relative path to use, or NULL to reset.
 *
 * @req SWR-GUI-010
 */
void app_config_set_path(const char *path);

/**
 * @brief Load the application configuration from disk.
 *
 * Reads the configuration file and populates @p sim_enabled_out.
 * If the file is absent or cannot be parsed the default value of
 * sim_enabled = 1 (simulator active) is written to @p sim_enabled_out
 * and the function returns 0 to indicate that no file was present.
 *
 * @param[out] sim_enabled_out  Receives 1 (simulator on) or 0 (HAL mode).
 *                              Must not be NULL.
 * @return 1 if the file was read and parsed successfully,
 *         0 if the file was missing or could not be parsed
 *           (default value is still written to *sim_enabled_out).
 *
 * @req SWR-GUI-010
 */
int app_config_load(int *sim_enabled_out);

/**
 * @brief Save the application configuration to disk.
 *
 * Writes the current configuration to the configuration file,
 * creating or overwriting it as needed.
 *
 * @param sim_enabled  1 to enable the simulator, 0 for real HAL mode.
 * @return 1 on success, 0 on failure (e.g. file could not be opened).
 *
 * @req SWR-GUI-010
 */
int app_config_save(int sim_enabled);

/**
 * @brief Load the language preference from the configuration file.
 *
 * Reads the "language=N" line from the config file. Returns the language
 * code (0=English, 1=Spanish, 2=French, 3=German). If absent or invalid,
 * returns 0 (English).
 *
 * @return Language code (0-3), or 0 on failure.
 *
 * @req SWR-GUI-012
 */
int app_config_load_language(void);

/**
 * @brief Save the language preference to the configuration file.
 *
 * Appends or updates the "language=N" line in the config file.
 * Preserves existing sim_enabled setting.
 *
 * @param language  Language code (0=English, 1=Spanish, 2=French, 3=German).
 * @return 1 on success, 0 on failure.
 *
 * @req SWR-GUI-012
 */
int app_config_save_language(int language);

/**
 * @brief Load the dashboard readability preference from the configuration file.
 *
 * Reads the "readability_mode=N" line from the config file. Returns 1 when
 * the dashboard readability mode is enabled, or 0 when the key is absent,
 * invalid, or explicitly disabled.
 *
 * @return 1 if readability mode is enabled, otherwise 0.
 *
 * @req SWR-GUI-014
 */
int app_config_load_readability_mode(void);

/**
 * @brief Save the dashboard readability preference to the configuration file.
 *
 * Updates the "readability_mode=N" line while preserving the existing
 * simulation-mode and language preferences.
 *
 * @param enabled  1 to enable readability mode, 0 to disable it.
 * @return 1 on success, 0 on failure.
 *
 * @req SWR-GUI-014
 */
int app_config_save_readability_mode(int enabled);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */

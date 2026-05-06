/**
 * @file test_localization.cpp
 * @brief Unit tests for localization API and language persistence.
 *
 * @req SWR-GUI-012  The application shall support four static UI languages,
 *                   allow language selection, and persist the selection.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

extern "C" {
#include "app_config.h"
#include "localization.h"
}

static std::string make_temp_path(const std::string &suffix = "")
{
    const char *tmp_dir = nullptr;

#ifdef _WIN32
    char tmp_buf[512] = {0};
    DWORD len = GetTempPathA(static_cast<DWORD>(sizeof(tmp_buf)), tmp_buf);
    if (len > 0 && len < sizeof(tmp_buf))
        tmp_dir = tmp_buf;
#else
    tmp_dir = ".";
#endif

    if (!tmp_dir || tmp_dir[0] == '\0')
        tmp_dir = ".";

    return std::string(tmp_dir) + "/test_localization" + suffix + ".cfg";
}

class LocalizationTest : public ::testing::Test
{
protected:
    std::string temp_path_;

    void SetUp() override
    {
        localization_set_language(LOC_LANG_ENGLISH);

        temp_path_ = make_temp_path("_" + std::string(
            ::testing::UnitTest::GetInstance()->current_test_info()->name()));
        std::remove(temp_path_.c_str());
        app_config_set_path(temp_path_.c_str());
    }

    void TearDown() override
    {
        localization_set_language(LOC_LANG_ENGLISH);
        std::remove(temp_path_.c_str());
        app_config_set_path(nullptr);
    }
};

// @req SWR-GUI-012 - Default language is English on startup/reset
TEST_F(LocalizationTest, DefaultsToEnglish)
{
    EXPECT_EQ(LOC_LANG_ENGLISH, localization_get_language());
    EXPECT_STREQ("Settings", localization_get_string(STR_SETTINGS));
}

// @req SWR-GUI-012 - Four supported languages can be selected at runtime
TEST_F(LocalizationTest, SupportsAllApprovedLanguages)
{
    struct Sample {
        Language lang;
        const char *expected_settings;
    };

    const Sample samples[] = {
        {LOC_LANG_ENGLISH, "Settings"},
        {LOC_LANG_SPANISH, "Configuraci"},
        {LOC_LANG_FRENCH, "Param"},
        {LOC_LANG_GERMAN, "Einstellungen"},
    };

    for (const Sample &sample : samples)
    {
        SCOPED_TRACE(static_cast<int>(sample.lang));
        localization_set_language(sample.lang);
        EXPECT_EQ(sample.lang, localization_get_language());
        EXPECT_NE(nullptr, localization_get_string(STR_SETTINGS));
        EXPECT_NE(std::string::npos,
                  std::string(localization_get_string(STR_SETTINGS)).find(sample.expected_settings));
    }
}

// @req SWR-GUI-013 - Header patient identity strings exist across the approved languages
TEST_F(LocalizationTest, HeaderPatientIdentityStringsExistAcrossAllLanguages)
{
    const Language languages[] = {
        LOC_LANG_ENGLISH,
        LOC_LANG_SPANISH,
        LOC_LANG_FRENCH,
        LOC_LANG_GERMAN,
    };
    const StringID ids[] = {STR_ACTIVE_PATIENT, STR_NO_ACTIVE_PATIENT};

    for (Language lang : languages)
    {
        localization_set_language(lang);
        for (StringID id : ids)
        {
            const char *value = localization_get_string(id);
            ASSERT_NE(nullptr, value);
            EXPECT_STRNE("", value);
        }
    }

    localization_set_language(LOC_LANG_ENGLISH);
    EXPECT_STREQ("Active patient", localization_get_string(STR_ACTIVE_PATIENT));
    EXPECT_STREQ("No active patient", localization_get_string(STR_NO_ACTIVE_PATIENT));
}

// @req SWR-GUI-012 - Invalid language values do not change the current selection
TEST_F(LocalizationTest, InvalidLanguageValuesAreIgnored)
{
    localization_set_language(LOC_LANG_GERMAN);
    localization_set_language(static_cast<Language>(-1));
    EXPECT_EQ(LOC_LANG_GERMAN, localization_get_language());

    localization_set_language(static_cast<Language>(LOC_LANG_COUNT));
    EXPECT_EQ(LOC_LANG_GERMAN, localization_get_language());
}

// @req SWR-GUI-012 - Language names expose the approved four-language selector list
TEST_F(LocalizationTest, LanguageNamesMatchApprovedSelectorEntries)
{
    EXPECT_STREQ("English", localization_get_language_name(LOC_LANG_ENGLISH));
    EXPECT_NE(std::string::npos,
              std::string(localization_get_language_name(LOC_LANG_SPANISH)).find("Espa"));
    EXPECT_NE(std::string::npos,
              std::string(localization_get_language_name(LOC_LANG_FRENCH)).find("Fran"));
    EXPECT_STREQ("Deutsch", localization_get_language_name(LOC_LANG_GERMAN));
}

// @req SWR-GUI-012 - Invalid language-name requests return a stable fallback
TEST_F(LocalizationTest, InvalidLanguageNameReturnsUnknown)
{
    EXPECT_STREQ("Unknown", localization_get_language_name(static_cast<Language>(-1)));
    EXPECT_STREQ("Unknown", localization_get_language_name(static_cast<Language>(99)));
}

// @req SWR-GUI-012 - Invalid string IDs return a stable fallback token
TEST_F(LocalizationTest, InvalidStringIdReturnsFallbackToken)
{
    EXPECT_STREQ("???", localization_get_string(static_cast<StringID>(-1)));
    EXPECT_STREQ("???", localization_get_string(static_cast<StringID>(STR_COUNT)));
}

// @req SWR-GUI-012 - The selected language is persisted and restored from monitor.cfg
TEST_F(LocalizationTest, SaveAndLoadLanguagePreference)
{
    ASSERT_EQ(1, app_config_save_language(LOC_LANG_FRENCH));
    EXPECT_EQ(LOC_LANG_FRENCH, app_config_load_language());

    localization_set_language(LOC_LANG_ENGLISH);
    localization_set_language(static_cast<Language>(app_config_load_language()));
    EXPECT_EQ(LOC_LANG_FRENCH, localization_get_language());
}

// @req SWR-GUI-012 - Saving language preserves the existing sim_enabled setting
TEST_F(LocalizationTest, SaveLanguagePreservesSimulationModeAndNormalizesInvalidInput)
{
    ASSERT_EQ(1, app_config_save(0));
    ASSERT_EQ(1, app_config_save_language(99));

    int sim_enabled = 123;
    ASSERT_EQ(1, app_config_load(&sim_enabled));
    EXPECT_EQ(0, sim_enabled);
    EXPECT_EQ(LOC_LANG_ENGLISH, app_config_load_language());
}

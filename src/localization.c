/**
 * @file localization.c
 * @brief Multi-language string support implementation
 *
 * Localization strings for: English, Spanish, French, German
 * IEC 62304 Class B - all strings are static (no heap allocation)
 */

#include "localization.h"
#include <string.h>

/* ===================================================================
 * Global language state (static, no heap)
 * =================================================================== */
static Language g_current_language = LOC_LANG_ENGLISH;

/* ===================================================================
 * String tables (4 languages x 100+ strings)
 * =================================================================== */

/* English strings */
static const char* g_strings_english[STR_COUNT] = {
    "Patient Vital Signs Monitor",  /* STR_APP_TITLE */
    "v2.8.0",                       /* STR_VERSION */
    "Sign In",                      /* STR_SIGN_IN */
    "Logout",                       /* STR_LOGOUT */
    "Settings",                     /* STR_SETTINGS */
    "About",                        /* STR_ABOUT */
    "OK",                           /* STR_OK */
    "Cancel",                       /* STR_CANCEL */
    "Save",                         /* STR_SAVE */
    "Reset",                        /* STR_RESET */

    "Username",                     /* STR_USERNAME */
    "Password",                     /* STR_PASSWORD */
    "Login Error",                  /* STR_LOGIN_ERROR */
    "Invalid username or password", /* STR_INVALID_CREDENTIALS */

    "ID",                           /* STR_PATIENT_ID */
    "Full Name",                    /* STR_PATIENT_NAME */
    "Age",                          /* STR_AGE */
    "Weight (kg)",                  /* STR_WEIGHT_KG */
    "Height (m)",                   /* STR_HEIGHT_M */
    "Admit",                        /* STR_ADMIT */
    "Admit / Refresh",              /* STR_ADMIT_REFRESH */

    "HR (bpm)",                     /* STR_HR_BPM */
    "Systolic",                     /* STR_SYSTOLIC */
    "Diastolic",                    /* STR_DIASTOLIC */
    "Temp (C)",                     /* STR_TEMP_C */
    "SpO2 (%)",                     /* STR_SPO2_PERCENT */
    "RR (br/min)",                  /* STR_RR_BR_MIN */
    "BMI",                          /* STR_BMI */
    "NEWS2",                        /* STR_NEWS2_SCORE */

    "Add Reading",                  /* STR_ADD_READING */
    "Clear Session",                /* STR_CLEAR_SESSION */
    "Demo: Deterioration",          /* STR_DEMO_DETERIORATION */
    "Demo: Bradycardia",            /* STR_DEMO_BRADYCARDIA */
    "Pause Sim",                    /* STR_PAUSE_SIM */
    "Resume Sim",                   /* STR_RESUME_SIM */

    "Active Alerts",                /* STR_ACTIVE_ALERTS */
    "Reading History",              /* STR_READING_HISTORY */
    "Session Alarm Events",         /* STR_SESSION_ALARM_EVENTS */
    "NORMAL",                       /* STR_ALERT_NORMAL */
    "WARNING",                      /* STR_ALERT_WARNING */
    "CRITICAL",                     /* STR_ALERT_CRITICAL */

    "Simulation Mode",              /* STR_SIM_MODE */
    "Simulation disabled",          /* STR_SIMULATION_DISABLED */
    "Simulation enabled",           /* STR_SIMULATION_ENABLED */
    "Device Mode",                  /* STR_DEVICE_MODE */
    "IN SIMULATION MODE",           /* STR_IN_SIMULATION_MODE */

    "Audio",                        /* STR_AUDIO_LABEL */
    "Audible",                      /* STR_AUDIO_AUDIBLE */
    "Silenced",                     /* STR_AUDIO_SILENCED */
    "Unknown",                      /* STR_AUDIO_UNKNOWN */

    "Language",                     /* STR_LANGUAGE */
    "Alarm Limits",                 /* STR_ALARM_LIMITS */
    "User Management",              /* STR_USER_MANAGEMENT */
    "My Account",                   /* STR_MY_ACCOUNT */
    "Change Password",              /* STR_CHANGE_PASSWORD */
    "Add User",                     /* STR_ADD_USER */
    "Remove User",                  /* STR_REMOVE_USER */
    "Role",                         /* STR_ROLE */
    "Admin",                        /* STR_ADMIN */
    "Clinical",                     /* STR_CLINICAL */

    "ALL NORMAL — Patient stable [ SIMULATION MODE ]",  /* STR_STATUS_NORMAL */
    "WARNING — Clinician review required",              /* STR_STATUS_WARNING */
    "!! CRITICAL — Immediate clinical action required !!",  /* STR_STATUS_CRITICAL */
    "DEVICE MODE — Enable simulation in Settings to use synthetic data",  /* STR_DEVICE_MODE_MSG */
    "SIMULATION MODE — Synthetic vital data",           /* STR_SIM_MODE_MSG */
};

/* Spanish strings */
static const char* g_strings_spanish[STR_COUNT] = {
    "Monitor de Signos Vitales del Paciente",
    "v2.8.0",
    "Iniciar Sesión",
    "Cerrar Sesión",
    "Configuración",
    "Acerca de",
    "Aceptar",
    "Cancelar",
    "Guardar",
    "Restablecer",

    "Usuario",
    "Contraseña",
    "Error de Inicio de Sesión",
    "Usuario o contraseña inválido",

    "ID",
    "Nombre Completo",
    "Edad",
    "Peso (kg)",
    "Altura (m)",
    "Admitir",
    "Admitir / Actualizar",

    "FC (lpm)",
    "Sistólica",
    "Diastólica",
    "Temp (°C)",
    "SpO2 (%)",
    "FR (resp/min)",
    "IMC",
    "NEWS2",

    "Agregar Lectura",
    "Limpiar Sesión",
    "Demo: Deterioro",
    "Demo: Bradicardia",
    "Pausar Sim",
    "Reanudar Sim",

    "Alertas Activas",
    "Historial de Lecturas",
    "Eventos de Alarma de Sesion",
    "NORMAL",
    "ADVERTENCIA",
    "CRÍTICO",

    "Modo Simulación",
    "Simulación deshabilitada",
    "Simulación habilitada",
    "Modo Dispositivo",
    "EN MODO SIMULACIÓN",

    "Audio",
    "Audible",
    "Silenciado",
    "Desconocido",

    "Idioma",
    "Límites de Alarma",
    "Gestión de Usuarios",
    "Mi Cuenta",
    "Cambiar Contraseña",
    "Agregar Usuario",
    "Eliminar Usuario",
    "Rol",
    "Administrador",
    "Clínico",

    "TODO NORMAL — Paciente estable [ MODO SIMULACIÓN ]",
    "ADVERTENCIA — Se requiere revisión del clínico",
    "!! CRÍTICO — Se requiere acción clínica inmediata !!",
    "MODO DISPOSITIVO — Habilite la simulación en Configuración",
    "MODO SIMULACIÓN — Datos vitales sintéticos",
};

/* French strings */
static const char* g_strings_french[STR_COUNT] = {
    "Moniteur de Signes Vitaux du Patient",
    "v2.8.0",
    "Se Connecter",
    "Déconnexion",
    "Paramètres",
    "À propos",
    "D'accord",
    "Annuler",
    "Enregistrer",
    "Réinitialiser",

    "Nom d'utilisateur",
    "Mot de passe",
    "Erreur de Connexion",
    "Nom d'utilisateur ou mot de passe invalide",

    "ID",
    "Nom Complet",
    "Âge",
    "Poids (kg)",
    "Hauteur (m)",
    "Admettre",
    "Admettre / Actualiser",

    "FC (bpm)",
    "Systolique",
    "Diastolique",
    "Temp (°C)",
    "SpO2 (%)",
    "FR (resp/min)",
    "IMC",
    "NEWS2",

    "Ajouter une Lecture",
    "Effacer la Session",
    "Démo: Détérioration",
    "Démo: Bradycardie",
    "Pause Sim",
    "Reprendre Sim",

    "Alertes Actives",
    "Historique des Lectures",
    "Evenements d'Alarme de Session",
    "NORMAL",
    "AVERTISSEMENT",
    "CRITIQUE",

    "Mode Simulation",
    "Simulation désactivée",
    "Simulation activée",
    "Mode Appareil",
    "EN MODE SIMULATION",

    "Audio",
    "Audible",
    "Silencieux",
    "Inconnu",

    "Langue",
    "Limites d'Alarme",
    "Gestion des Utilisateurs",
    "Mon Compte",
    "Changer le Mot de Passe",
    "Ajouter un Utilisateur",
    "Supprimer un Utilisateur",
    "Rôle",
    "Administrateur",
    "Clinique",

    "TOUT NORMAL — Patient stable [ MODE SIMULATION ]",
    "AVERTISSEMENT — Examen clinicien requis",
    "!! CRITIQUE — Action clinique immédiate requise !!",
    "MODE APPAREIL — Activez la simulation dans les paramètres",
    "MODE SIMULATION — Données vitales synthétiques",
};

/* German strings */
static const char* g_strings_german[STR_COUNT] = {
    "Patient Vital Signs Monitor",
    "v2.8.0",
    "Anmelden",
    "Abmelden",
    "Einstellungen",
    "Über",
    "OK",
    "Abbrechen",
    "Speichern",
    "Zurücksetzen",

    "Benutzername",
    "Passwort",
    "Anmeldungsfehler",
    "Benutzername oder Passwort ungültig",

    "ID",
    "Vollständiger Name",
    "Alter",
    "Gewicht (kg)",
    "Höhe (m)",
    "Aufnehmen",
    "Aufnehmen / Aktualisieren",

    "HF (bpm)",
    "Systolisch",
    "Diastolisch",
    "Temp (°C)",
    "SpO2 (%)",
    "AF (Atemz/min)",
    "BMI",
    "NEWS2",

    "Messwert Hinzufügen",
    "Sitzung Löschen",
    "Demo: Verschlechterung",
    "Demo: Bradykardie",
    "Sim Pausieren",
    "Sim Fortsetzen",

    "Aktive Warnmeldungen",
    "Messverlauf",
    "Sitzungsalarmereignisse",
    "NORMAL",
    "WARNUNG",
    "KRITISCH",

    "Simulationsmodus",
    "Simulation deaktiviert",
    "Simulation aktiviert",
    "Gerät-Modus",
    "IM SIMULATIONSMODUS",

    "Audio",
    "Hoerbar",
    "Stumm",
    "Unbekannt",

    "Sprache",
    "Alarmlimits",
    "Benutzerverwaltung",
    "Mein Konto",
    "Passwort Ändern",
    "Benutzer Hinzufügen",
    "Benutzer Entfernen",
    "Rolle",
    "Administrator",
    "Klinisch",

    "ALLES NORMAL — Patient stabil [ SIMULATIONSMODUS ]",
    "WARNUNG — Klinische Überprüfung erforderlich",
    "!! KRITISCH — Sofortige klinische Maßnahme erforderlich !!",
    "GERÄT-MODUS — Aktivieren Sie die Simulation in den Einstellungen",
    "SIMULATIONSMODUS — Synthetische Vitaldaten",
};

/* All language tables */
static const char** g_string_tables[LOC_LANG_COUNT] = {
    g_strings_english,
    g_strings_spanish,
    g_strings_french,
    g_strings_german
};

/* ===================================================================
 * Language names (for display in UI)
 * =================================================================== */
static const char* g_language_names[LOC_LANG_COUNT] = {
    "English",
    "Español",
    "Français",
    "Deutsch"
};

/* ===================================================================
 * Public API implementation
 * =================================================================== */

void localization_set_language(Language lang)
{
    if (lang >= 0 && lang < LOC_LANG_COUNT) {
        g_current_language = lang;
    }
}

Language localization_get_language(void)
{
    return g_current_language;
}

const char* localization_get_string(StringID id)
{
    if (id < 0 || id >= STR_COUNT) {
        return "???";  /* Fallback for invalid ID */
    }
    return g_string_tables[g_current_language][id];
}

const char* localization_get_language_name(Language lang)
{
    if (lang >= 0 && lang < LOC_LANG_COUNT) {
        return g_language_names[lang];
    }
    return "Unknown";
}

/**
 * @file gui_main.c
 * @brief Win32 GUI entry point — Patient Vital Signs Monitor.
 *
 * Presents a two-screen application:
 *   1. Login screen  — authenticates the clinician (SWR-GUI-001/002).
 *   2. Dashboard     — live monitoring display with colour-coded vital tiles,
 *                      patient data entry and session history (SWR-GUI-003/004).
 *
 * All memory is static (stack/global) — no heap allocation (SYS-012).
 *
 * @req SWR-GUI-001
 * @req SWR-GUI-002
 * @req SWR-GUI-003
 * @req SWR-GUI-004
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vitals.h"
#include "alerts.h"
#include "patient.h"
#include "gui_auth.h"
#include "hw_vitals.h"

/* =======================================================================
 * Window class names
 * ======================================================================= */
#define CLASS_LOGIN      "PVM_Login"
#define CLASS_DASHBOARD  "PVM_Dashboard"
#define APP_TITLE        "Patient Vital Signs Monitor"

/* =======================================================================
 * Control IDs
 * ======================================================================= */

/* Login */
#define IDC_LGN_USER     100
#define IDC_LGN_PASS     101
#define IDC_LGN_BTN      102
#define IDC_LGN_ERR      103

/* Dashboard — patient identity inputs */
#define IDC_PAT_ID       1001
#define IDC_PAT_NAME     1002
#define IDC_PAT_AGE      1003
#define IDC_PAT_WEIGHT   1004
#define IDC_PAT_HEIGHT   1005
#define IDC_BTN_ADMIT    1010

/* Dashboard — vitals inputs */
#define IDC_VIT_HR       1101
#define IDC_VIT_SYS      1102
#define IDC_VIT_DIA      1103
#define IDC_VIT_TEMP     1104
#define IDC_VIT_SPO2     1105
#define IDC_BTN_ADD      1110
#define IDC_BTN_CLEAR    1111

/* Dashboard — scenario and utility */
#define IDC_BTN_SCEN1    1200
#define IDC_BTN_SCEN2    1201
#define IDC_BTN_LOGOUT   1202
#define IDC_BTN_PAUSE    1203

/* Simulation timer ID */
#define TIMER_SIM        1

/* Dashboard — output lists */
#define IDC_LIST_ALERTS  1300
#define IDC_LIST_HISTORY 1301

/* =======================================================================
 * Layout constants  (all in logical pixels at 96 dpi)
 * ======================================================================= */
#define WIN_CW  920   /* client width  */
#define WIN_CH  850   /* client height */

/* Painted zone heights */
#define HDR_H    56   /* header bar           */
#define PBAR_H   38   /* patient summary bar  */
#define TILE_Y   (HDR_H + PBAR_H)          /*  94 */
#define TILE_H   190  /* 2×2 tile grid total  */
#define STAT_Y   (TILE_Y + TILE_H + 6)     /* 290 */
#define STAT_H   44
#define CTRL_Y   (STAT_Y + STAT_H + 8)     /* 342 */

/* =======================================================================
 * Colour palette
 * ======================================================================= */
#define CLR_NAVY        RGB(30,  58,  138)
#define CLR_SLATE       RGB(51,  65,  85)
#define CLR_NEAR_WHITE  RGB(248, 250, 252)
#define CLR_LIGHT_GRAY  RGB(226, 232, 240)
#define CLR_DARK_TEXT   RGB(30,  41,  59)
#define CLR_WHITE       RGB(255, 255, 255)

/* Alert-level tile colours */
#define CLR_OK_BG   RGB(220, 252, 231)
#define CLR_OK_FG   RGB(21,  128,  61)
#define CLR_WN_BG   RGB(254, 249, 195)
#define CLR_WN_FG   RGB(161,  98,   7)
#define CLR_CR_BG   RGB(254, 226, 226)
#define CLR_CR_FG   RGB(185,  28,  28)

/* =======================================================================
 * Global application state  (all static — no heap)
 * ======================================================================= */
typedef struct {
    HINSTANCE instance;
    HWND      hwnd_login;
    HWND      hwnd_dash;

    /* Auth */
    char logged_user[AUTH_MAX_CREDENTIAL_LEN];

    /* Patient session */
    PatientRecord patient;
    int           has_patient;

    /* Simulation state */
    int           sim_paused;

    /* GDI font handles */
    HFONT font_hdr;       /* 18 pt bold  — header title       */
    HFONT font_tile_val;  /* 18 pt bold  — tile value numbers */
    HFONT font_tile_lbl;  /*  9 pt       — tile labels/badge  */
    HFONT font_status;    /* 11 pt bold  — status banner      */
    HFONT font_ui;        /*  9 pt       — controls + lists   */
} AppState;

static AppState g_app;

/* =======================================================================
 * Forward declarations
 * ======================================================================= */
static LRESULT CALLBACK login_proc (HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK dash_proc  (HWND, UINT, WPARAM, LPARAM);
static void create_dashboard(void);
static void update_dashboard(HWND hwnd);

/* =======================================================================
 * GDI helpers
 * ======================================================================= */

/** Create a Segoe UI font at the requested point size. */
static HFONT make_font(int pt, BOOL bold)
{
    int h = -MulDiv(pt, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    return CreateFontA(h, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                       FALSE, FALSE, FALSE, ANSI_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
}

static void fill_rect(HDC hdc, int x, int y, int w, int h, COLORREF c)
{
    RECT r = {x, y, x + w, y + h};
    HBRUSH br = CreateSolidBrush(c);
    FillRect(hdc, &r, br);
    DeleteObject(br);
}

static void draw_text_ex(HDC hdc, const char *txt,
                         int x, int y, int w, int h,
                         HFONT font, COLORREF fg, UINT fmt)
{
    RECT r = {x, y, x + w, y + h};
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, fg);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextA(hdc, txt, -1, &r, fmt);
    SelectObject(hdc, old);
}

/* =======================================================================
 * Painted zones
 * ======================================================================= */

/**
 * @brief Paint the application header bar.
 * @req SWR-GUI-002
 */
static void paint_header(HDC hdc, int cw)
{
    char buf[96];
    fill_rect(hdc, 0, 0, cw, HDR_H, CLR_NAVY);

    /* Medical cross icon (drawn with GDI lines) */
    fill_rect(hdc, 14, 12, 10, 32, CLR_WHITE); /* vertical   */
    fill_rect(hdc, 9,  22, 20, 12, CLR_WHITE); /* horizontal */

    /* App title */
    draw_text_ex(hdc, "  " APP_TITLE,
                 38, 0, cw - 180, HDR_H,
                 g_app.font_hdr, CLR_WHITE,
                 DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    /* SIM LIVE / SIM PAUSED badge */
    if (g_app.has_patient) {
        const char *sim_txt   = g_app.sim_paused ? "SIM PAUSED" : "* SIM LIVE";
        COLORREF    sim_color = g_app.sim_paused ? RGB(253, 224,  71)
                                                 : RGB(134, 239, 172);
        draw_text_ex(hdc, sim_txt,
                     cw - 360, 0, 130, HDR_H,
                     g_app.font_tile_lbl, sim_color,
                     DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
    }

    /* Logged-in user */
    if (g_app.logged_user[0] != '\0') {
        snprintf(buf, sizeof(buf), "User: %s", g_app.logged_user);
        draw_text_ex(hdc, buf,
                     cw - 220, 0, 130, HDR_H,
                     g_app.font_ui, RGB(186, 230, 253),
                     DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
    }
}

/**
 * @brief Paint the patient summary bar.
 * @req SWR-GUI-003
 */
static void paint_patient_bar(HDC hdc, int cw)
{
    char buf[256];
    fill_rect(hdc, 0, HDR_H, cw, PBAR_H, CLR_SLATE);

    if (g_app.has_patient) {
        float bmi = calculate_bmi(g_app.patient.info.weight_kg,
                                  g_app.patient.info.height_m);
        snprintf(buf, sizeof(buf),
                 "  Patient: %s   |   ID: %d   |   Age: %d yrs"
                 "   |   BMI: %.1f (%s)   |   Readings: %d / %d",
                 g_app.patient.info.name,
                 g_app.patient.info.id,
                 g_app.patient.info.age,
                 bmi, bmi_category(bmi),
                 g_app.patient.reading_count, MAX_READINGS);
    } else {
        snprintf(buf, sizeof(buf), "  Awaiting first simulation reading...");
    }

    draw_text_ex(hdc, buf, 0, HDR_H, cw, PBAR_H,
                 g_app.font_ui,
                 g_app.has_patient ? CLR_LIGHT_GRAY : RGB(148, 163, 184),
                 DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}

/**
 * @brief Paint one colour-coded vital sign tile.
 *
 * Each tile shows the parameter label, measured value + unit, and a
 * colour-coded badge (NORMAL / WARNING / CRITICAL).
 *
 * @req SWR-GUI-003
 */
static void paint_tile(HDC hdc,
                       int tx, int ty, int tw, int th,
                       const char *label,
                       const char *value, const char *unit,
                       AlertLevel level)
{
    COLORREF bg, fg;
    char badge[24];
    HPEN border_pen;
    HPEN old_pen;
    HBRUSH old_br;
    char full_val[48];

    switch (level) {
        case ALERT_WARNING:
            bg = CLR_WN_BG; fg = CLR_WN_FG;
            snprintf(badge, sizeof(badge), "  WARNING");
            break;
        case ALERT_CRITICAL:
            bg = CLR_CR_BG; fg = CLR_CR_FG;
            snprintf(badge, sizeof(badge), "  CRITICAL");
            break;
        default:
            bg = CLR_OK_BG; fg = CLR_OK_FG;
            snprintf(badge, sizeof(badge), "  NORMAL");
            break;
    }

    /* Fill and border */
    fill_rect(hdc, tx, ty, tw, th, bg);
    border_pen = CreatePen(PS_SOLID, 2, fg);
    old_pen    = (HPEN) SelectObject(hdc, border_pen);
    old_br     = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, tx + 1, ty + 1, tx + tw - 1, ty + th - 1, 10, 10);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_br);
    DeleteObject(border_pen);

    /* Parameter label (top-left) */
    draw_text_ex(hdc, label, tx + 10, ty + 8, tw - 20, 18,
                 g_app.font_tile_lbl, fg,
                 DT_SINGLELINE | DT_LEFT);

    /* Measurement value (centre) */
    snprintf(full_val, sizeof(full_val), "%s %s", value, unit);
    draw_text_ex(hdc, full_val, tx + 8, ty + 30, tw - 16, 36,
                 g_app.font_tile_val, CLR_DARK_TEXT,
                 DT_SINGLELINE | DT_LEFT | DT_VCENTER);

    /* Status badge (bottom-left) */
    draw_text_ex(hdc, badge, tx + 8, ty + th - 26, tw - 16, 20,
                 g_app.font_tile_lbl, fg,
                 DT_SINGLELINE | DT_LEFT);
}

/**
 * @brief Paint the 2×2 vital sign tile grid.
 * @req SWR-GUI-003
 */
static void paint_tiles(HDC hdc, int cw)
{
    const VitalSigns *v = NULL;
    char hr_s[16], bp_s[24], tp_s[16], sp_s[16];
    AlertLevel lv_hr = ALERT_NORMAL, lv_bp = ALERT_NORMAL;
    AlertLevel lv_tp = ALERT_NORMAL, lv_sp = ALERT_NORMAL;

    int pad = 10;
    int tw  = (cw - 3 * pad) / 2;
    int th  = (TILE_H - 3 * pad) / 2;

    /* Background */
    fill_rect(hdc, 0, TILE_Y, cw, TILE_H + pad, CLR_NEAR_WHITE);

    if (g_app.has_patient) {
        v = patient_latest_reading(&g_app.patient);
    }

    if (v != NULL) {
        snprintf(hr_s, sizeof(hr_s), "%d",       v->heart_rate);
        snprintf(bp_s, sizeof(bp_s), "%d / %d",  v->systolic_bp, v->diastolic_bp);
        snprintf(tp_s, sizeof(tp_s), "%.1f",     v->temperature);
        snprintf(sp_s, sizeof(sp_s), "%d",       v->spo2);
        lv_hr = check_heart_rate(v->heart_rate);
        lv_bp = check_blood_pressure(v->systolic_bp, v->diastolic_bp);
        lv_tp = check_temperature(v->temperature);
        lv_sp = check_spo2(v->spo2);
    } else {
        strncpy(hr_s, "--",    sizeof(hr_s)  - 1); hr_s[sizeof(hr_s)  - 1] = '\0';
        strncpy(bp_s, "--/--", sizeof(bp_s)  - 1); bp_s[sizeof(bp_s)  - 1] = '\0';
        strncpy(tp_s, "--",    sizeof(tp_s)  - 1); tp_s[sizeof(tp_s)  - 1] = '\0';
        strncpy(sp_s, "--",    sizeof(sp_s)  - 1); sp_s[sizeof(sp_s)  - 1] = '\0';
    }

    /* Row 1 */
    paint_tile(hdc, pad,           TILE_Y + pad, tw, th,
               "HEART RATE",      hr_s, "bpm",  lv_hr);
    paint_tile(hdc, pad + tw + pad, TILE_Y + pad, tw, th,
               "BLOOD PRESSURE",  bp_s, "mmHg", lv_bp);
    /* Row 2 */
    paint_tile(hdc, pad,           TILE_Y + pad + th + pad, tw, th,
               "TEMPERATURE",     tp_s, "C",    lv_tp);
    paint_tile(hdc, pad + tw + pad, TILE_Y + pad + th + pad, tw, th,
               "SpO2",            sp_s, "%",    lv_sp);
}

/**
 * @brief Paint the aggregate status banner.
 * @req SWR-GUI-003
 */
static void paint_status_banner(HDC hdc, int cw)
{
    AlertLevel lvl = g_app.has_patient
                     ? patient_current_status(&g_app.patient)
                     : ALERT_NORMAL;
    COLORREF bg, fg;
    const char *txt;

    switch (lvl) {
        case ALERT_CRITICAL:
            bg = CLR_CR_FG; fg = CLR_WHITE;
            txt = "!! CRITICAL — Immediate clinical action required !!";
            break;
        case ALERT_WARNING:
            bg = CLR_WN_FG; fg = CLR_WHITE;
            txt = "WARNING — Clinician review required";
            break;
        default:
            bg = CLR_OK_FG; fg = CLR_WHITE;
            txt = g_app.has_patient
                  ? "ALL NORMAL — Patient stable"
                  : "Admit a patient and add a reading to begin monitoring";
            break;
    }

    fill_rect(hdc, 0, STAT_Y, cw, STAT_H, bg);
    draw_text_ex(hdc, txt, 0, STAT_Y, cw, STAT_H,
                 g_app.font_status, fg,
                 DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}

/* =======================================================================
 * Control creation helpers
 * ======================================================================= */
static HWND make_label(HWND p, const char *t, int x, int y, int w, int h)
{
    return CreateWindowExA(0, "STATIC", t, WS_CHILD | WS_VISIBLE,
                           x, y, w, h, p, NULL, g_app.instance, NULL);
}

static HWND make_edit(HWND p, int id, const char *t, int x, int y, int w, int h)
{
    return CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", t,
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                           x, y, w, h, p, (HMENU)(INT_PTR)id,
                           g_app.instance, NULL);
}

static HWND make_btn(HWND p, int id, const char *t, int x, int y, int w, int h)
{
    return CreateWindowExA(0, "BUTTON", t,
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                           x, y, w, h, p, (HMENU)(INT_PTR)id,
                           g_app.instance, NULL);
}

static void font_all_children(HWND p, HFONT f)
{
    HWND c = GetWindow(p, GW_CHILD);
    while (c != NULL) {
        SendMessage(c, WM_SETFONT, (WPARAM)f, TRUE);
        c = GetWindow(c, GW_HWNDNEXT);
    }
}

/* =======================================================================
 * Dashboard controls layout
 * ======================================================================= */
#define CY  CTRL_Y   /* shorthand: controls top Y */

/**
 * @brief Create all dashboard child controls.
 * @req SWR-GUI-004
 */
static void create_dash_controls(HWND w)
{
    /* --- Patient identity row --- */
    make_label(w, "ID",             20,  CY,      40,  18);
    make_edit (w, IDC_PAT_ID,   "1001", 20,  CY + 20, 100, 24);
    make_label(w, "Full Name",     130,  CY,      90,  18);
    make_edit (w, IDC_PAT_NAME, "Sarah Johnson", 130, CY + 20, 240, 24);
    make_label(w, "Age",           382,  CY,      40,  18);
    make_edit (w, IDC_PAT_AGE,  "52",  382,  CY + 20,  70, 24);
    make_label(w, "Weight (kg)",   464,  CY,      90,  18);
    make_edit (w, IDC_PAT_WEIGHT, "72.5", 464, CY + 20, 90, 24);
    make_label(w, "Height (m)",    566,  CY,      90,  18);
    make_edit (w, IDC_PAT_HEIGHT, "1.66", 566, CY + 20, 90, 24);
    make_btn  (w, IDC_BTN_ADMIT, "Admit / Refresh", 670, CY + 18, 130, 28);

    /* --- Vitals entry row --- */
    make_label(w, "HR (bpm)",     20,  CY + 62,  80, 18);
    make_edit (w, IDC_VIT_HR,   "78",  20,  CY + 82, 100, 24);
    make_label(w, "Systolic",    132,  CY + 62,  70, 18);
    make_edit (w, IDC_VIT_SYS, "122", 132,  CY + 82, 100, 24);
    make_label(w, "Diastolic",   244,  CY + 62,  70, 18);
    make_edit (w, IDC_VIT_DIA,  "82", 244,  CY + 82, 100, 24);
    make_label(w, "Temp (°C)",   356,  CY + 62,  70, 18);
    make_edit (w, IDC_VIT_TEMP,"36.7", 356, CY + 82, 100, 24);
    make_label(w, "SpO2 (%)",    468,  CY + 62,  70, 18);
    make_edit (w, IDC_VIT_SPO2, "98", 468,  CY + 82, 100, 24);
    make_btn  (w, IDC_BTN_ADD,  "Add Reading",   580, CY + 80, 110, 28);
    make_btn  (w, IDC_BTN_CLEAR,"Clear Session", 702, CY + 80, 110, 28);

    /* --- Scenario buttons --- */
    make_btn(w, IDC_BTN_SCEN1, "Demo: Deterioration", 20,  CY + 124, 175, 26);
    make_btn(w, IDC_BTN_SCEN2, "Demo: Bradycardia",   205, CY + 124, 160, 26);

    /* --- Alerts list --- */
    make_label(w, "Active Alerts", 20, CY + 162, 160, 18);
    CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
                    20, CY + 182, 872, 88,
                    w, (HMENU)(INT_PTR)IDC_LIST_ALERTS,
                    g_app.instance, NULL);

    /* --- History list --- */
    make_label(w, "Reading History", 20, CY + 282, 160, 18);
    CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
                    20, CY + 302, 872, 130,
                    w, (HMENU)(INT_PTR)IDC_LIST_HISTORY,
                    g_app.instance, NULL);
}

/* =======================================================================
 * Dashboard: data update
 * ======================================================================= */

/**
 * @brief Refresh all dynamic dashboard content after any data change.
 * @req SWR-GUI-003
 * @req SWR-GUI-004
 */
static void update_dashboard(HWND w)
{
    char buf[256];
    const VitalSigns *latest = NULL;
    Alert   alerts[MAX_ALERTS];
    int     alert_count = 0;
    int     i;

    SendMessageA(GetDlgItem(w, IDC_LIST_HISTORY), LB_RESETCONTENT, 0, 0);
    SendMessageA(GetDlgItem(w, IDC_LIST_ALERTS),  LB_RESETCONTENT, 0, 0);

    /* Trigger repaint of the painted zones */
    InvalidateRect(w, NULL, FALSE);

    if (!g_app.has_patient) {
        SendMessageA(GetDlgItem(w, IDC_LIST_ALERTS), LB_ADDSTRING, 0,
                     (LPARAM)"No patient admitted yet.");
        return;
    }

    /* History list */
    for (i = 0; i < g_app.patient.reading_count; ++i) {
        const VitalSigns *r = &g_app.patient.readings[i];
        snprintf(buf, sizeof(buf),
                 "#%d   HR %d bpm | BP %d/%d mmHg | Temp %.1f C"
                 " | SpO2 %d%%   [%s]",
                 i + 1,
                 r->heart_rate,
                 r->systolic_bp, r->diastolic_bp,
                 r->temperature,
                 r->spo2,
                 alert_level_str(overall_alert_level(r)));
        SendMessageA(GetDlgItem(w, IDC_LIST_HISTORY), LB_ADDSTRING, 0, (LPARAM)buf);
    }

    /* Alerts list */
    latest = patient_latest_reading(&g_app.patient);
    if (latest != NULL) {
        alert_count = generate_alerts(latest, alerts, MAX_ALERTS);
    }

    if (alert_count == 0) {
        SendMessageA(GetDlgItem(w, IDC_LIST_ALERTS), LB_ADDSTRING, 0,
                     (LPARAM)"No active alerts — all parameters within normal range.");
    } else {
        for (i = 0; i < alert_count; ++i) {
            const char *sev = (alerts[i].level == ALERT_CRITICAL)
                              ? "CRITICAL" : "WARNING ";
            snprintf(buf, sizeof(buf), "[%s]  %s", sev, alerts[i].message);
            SendMessageA(GetDlgItem(w, IDC_LIST_ALERTS), LB_ADDSTRING, 0, (LPARAM)buf);
        }
    }
}

/* =======================================================================
 * Input helpers
 * ======================================================================= */
static int get_txt(HWND p, int id, char *out, int len)
{
    if (len <= 0) return 0;
    out[0] = '\0';
    GetWindowTextA(GetDlgItem(p, id), out, len);
    return (int)strlen(out);
}

static void set_txt(HWND p, int id, const char *s)
{
    SetWindowTextA(GetDlgItem(p, id), s);
}

static int parse_int_field(HWND p, int id, const char *label, int *out)
{
    char buf[64]; char *ep = NULL; long v;
    if (!get_txt(p, id, buf, (int)sizeof(buf))) {
        char m[128]; snprintf(m, sizeof(m), "%s is required.", label);
        MessageBoxA(p, m, APP_TITLE, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(p, id)); return 0;
    }
    v = strtol(buf, &ep, 10);
    if (ep == buf || *ep != '\0') {
        char m[128]; snprintf(m, sizeof(m), "%s must be a whole number.", label);
        MessageBoxA(p, m, APP_TITLE, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(p, id)); return 0;
    }
    *out = (int)v; return 1;
}

static int parse_flt_field(HWND p, int id, const char *label, float *out)
{
    char buf[64]; char *ep = NULL; double v;
    if (!get_txt(p, id, buf, (int)sizeof(buf))) {
        char m[128]; snprintf(m, sizeof(m), "%s is required.", label);
        MessageBoxA(p, m, APP_TITLE, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(p, id)); return 0;
    }
    v = strtod(buf, &ep);
    if (ep == buf || *ep != '\0') {
        char m[128]; snprintf(m, sizeof(m), "%s must be a decimal number.", label);
        MessageBoxA(p, m, APP_TITLE, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(p, id)); return 0;
    }
    *out = (float)v; return 1;
}

/* =======================================================================
 * Dashboard actions
 * ======================================================================= */

/**
 * @brief Admit a patient from the identity fields.
 * @req SWR-GUI-004
 */
static int do_admit(HWND w)
{
    int id, age; float wt, ht; char name[MAX_NAME_LEN];
    if (!parse_int_field(w, IDC_PAT_ID,     "Patient ID",  &id))  return 0;
    if (!parse_int_field(w, IDC_PAT_AGE,    "Age",         &age)) return 0;
    if (!parse_flt_field(w, IDC_PAT_WEIGHT, "Weight (kg)", &wt))  return 0;
    if (!parse_flt_field(w, IDC_PAT_HEIGHT, "Height (m)",  &ht))  return 0;
    if (!get_txt(w, IDC_PAT_NAME, name, (int)sizeof(name))) {
        MessageBoxA(w, "Patient name is required.", APP_TITLE,
                    MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(w, IDC_PAT_NAME)); return 0;
    }
    patient_init(&g_app.patient, id, name, age, wt, ht);
    g_app.has_patient = 1;
    update_dashboard(w);
    return 1;
}

/**
 * @brief Record a new vital signs reading from the vitals entry fields.
 * @req SWR-GUI-004
 */
static void do_add_reading(HWND w)
{
    VitalSigns v;
    if (!g_app.has_patient && !do_admit(w)) return;
    if (!parse_int_field(w, IDC_VIT_HR,   "Heart Rate",   &v.heart_rate))   return;
    if (!parse_int_field(w, IDC_VIT_SYS,  "Systolic BP",  &v.systolic_bp))  return;
    if (!parse_int_field(w, IDC_VIT_DIA,  "Diastolic BP", &v.diastolic_bp)) return;
    if (!parse_flt_field(w, IDC_VIT_TEMP, "Temperature",  &v.temperature))  return;
    if (!parse_int_field(w, IDC_VIT_SPO2, "SpO2",         &v.spo2))         return;

    if (!patient_add_reading(&g_app.patient, &v)) {
        MessageBoxA(w,
                    "Reading buffer is full (10 readings).\n"
                    "Clear the session to start a new monitoring period.",
                    APP_TITLE, MB_OK | MB_ICONWARNING);
        return;
    }
    update_dashboard(w);
}

/** Clear all session data and reset the input fields to default values. */
static void do_clear(HWND w)
{
    ZeroMemory(&g_app.patient, sizeof(g_app.patient));
    g_app.has_patient = 0;
    set_txt(w, IDC_PAT_ID,     "1001");
    set_txt(w, IDC_PAT_NAME,   "Sarah Johnson");
    set_txt(w, IDC_PAT_AGE,    "52");
    set_txt(w, IDC_PAT_WEIGHT, "72.5");
    set_txt(w, IDC_PAT_HEIGHT, "1.66");
    set_txt(w, IDC_VIT_HR,     "78");
    set_txt(w, IDC_VIT_SYS,    "122");
    set_txt(w, IDC_VIT_DIA,    "82");
    set_txt(w, IDC_VIT_TEMP,   "36.7");
    set_txt(w, IDC_VIT_SPO2,   "98");
    update_dashboard(w);
}

/** Load a pre-built demonstration scenario. */
static void do_scenario(HWND w, int s)
{
    /* Scenario 1 — gradual clinical deterioration */
    static const VitalSigns deterioration[3] = {
        {78,  122,  82, 36.7f, 98},
        {108, 148,  94, 37.9f, 93},
        {135, 175, 112, 39.8f, 87}
    };
    /* Scenario 2 — bradycardia episode */
    static const VitalSigns bradycardia[2] = {
        {68, 118, 76, 36.5f, 99},
        {38, 110, 72, 36.6f, 97}
    };
    const VitalSigns *readings;
    int n, i;

    if (s == 1) {
        set_txt(w, IDC_PAT_ID,     "1001");
        set_txt(w, IDC_PAT_NAME,   "Sarah Johnson");
        set_txt(w, IDC_PAT_AGE,    "52");
        set_txt(w, IDC_PAT_WEIGHT, "72.5");
        set_txt(w, IDC_PAT_HEIGHT, "1.66");
        readings = deterioration;
        n = 3;
    } else {
        set_txt(w, IDC_PAT_ID,     "1002");
        set_txt(w, IDC_PAT_NAME,   "David Okonkwo");
        set_txt(w, IDC_PAT_AGE,    "34");
        set_txt(w, IDC_PAT_WEIGHT, "85.0");
        set_txt(w, IDC_PAT_HEIGHT, "1.80");
        readings = bradycardia;
        n = 2;
    }

    if (!do_admit(w)) return;

    for (i = 0; i < n; ++i) {
        patient_add_reading(&g_app.patient, &readings[i]);
    }
    update_dashboard(w);
}

/* =======================================================================
 * Dashboard window procedure
 * ======================================================================= */
static LRESULT CALLBACK dash_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {

        case WM_CREATE: {
            VitalSigns first_v;
            g_app.hwnd_dash = w;
            create_dash_controls(w);
            font_all_children(w, g_app.font_ui);
            /* Header buttons — created after font pass so they get the right font */
            {
                HWND btn = make_btn(w, IDC_BTN_LOGOUT, "Logout",
                                    WIN_CW - 86, 14, 72, 28);
                SendMessage(btn, WM_SETFONT, (WPARAM)g_app.font_ui, TRUE);
            }
            {
                HWND btn = make_btn(w, IDC_BTN_PAUSE, "Pause Sim",
                                    WIN_CW - 176, 14, 86, 28);
                SendMessage(btn, WM_SETFONT, (WPARAM)g_app.font_ui, TRUE);
            }
            /* Initialise simulation and auto-admit demo patient */
            hw_init();
            g_app.sim_paused = 0;
            patient_init(&g_app.patient, 2001, "James Mitchell",
                         45, 78.0f, 1.75f);
            g_app.has_patient = 1;
            /* Pre-populate patient identity fields with demo values */
            set_txt(w, IDC_PAT_ID,     "2001");
            set_txt(w, IDC_PAT_NAME,   "James Mitchell");
            set_txt(w, IDC_PAT_AGE,    "45");
            set_txt(w, IDC_PAT_WEIGHT, "78.0");
            set_txt(w, IDC_PAT_HEIGHT, "1.75");
            /* First reading fires immediately so dashboard is never empty */
            hw_get_next_reading(&first_v);
            patient_add_reading(&g_app.patient, &first_v);
            /* 2-second simulation timer */
            SetTimer(w, TIMER_SIM, 2000, NULL);
            update_dashboard(w);
            return 0;
        }

        case WM_TIMER:
            if (wp == TIMER_SIM && !g_app.sim_paused) {
                VitalSigns v;
                /* If buffer is full, re-init patient keeping same demographics
                 * so monitoring continues uninterrupted across cycle boundaries */
                if (g_app.has_patient
                        && patient_is_full(&g_app.patient)) {
                    patient_init(&g_app.patient,
                                 g_app.patient.info.id,
                                 g_app.patient.info.name,
                                 g_app.patient.info.age,
                                 g_app.patient.info.weight_kg,
                                 g_app.patient.info.height_m);
                }
                hw_get_next_reading(&v);
                patient_add_reading(&g_app.patient, &v);
                g_app.has_patient = 1;
                update_dashboard(w);
            }
            return 0;

        case WM_ERASEBKGND: {
            RECT r; GetClientRect(w, &r);
            HBRUSH br = CreateSolidBrush(CLR_NEAR_WHITE);
            FillRect((HDC)wp, &r, br);
            DeleteObject(br);
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(w, &ps);
            paint_header(hdc, WIN_CW);
            paint_patient_bar(hdc, WIN_CW);
            paint_tiles(hdc, WIN_CW);
            paint_status_banner(hdc, WIN_CW);
            EndPaint(w, &ps);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_BTN_ADMIT:  do_admit(w);       return 0;
                case IDC_BTN_ADD:    do_add_reading(w); return 0;
                case IDC_BTN_CLEAR:  do_clear(w);       return 0;
                case IDC_BTN_SCEN1:  do_scenario(w, 1); return 0;
                case IDC_BTN_SCEN2:  do_scenario(w, 2); return 0;
                case IDC_BTN_PAUSE:
                    g_app.sim_paused = !g_app.sim_paused;
                    SetWindowTextA(GetDlgItem(w, IDC_BTN_PAUSE),
                                   g_app.sim_paused ? "Resume Sim" : "Pause Sim");
                    InvalidateRect(w, NULL, FALSE);
                    return 0;
                case IDC_BTN_LOGOUT:
                    KillTimer(w, TIMER_SIM);
                    ZeroMemory(&g_app.patient, sizeof(g_app.patient));
                    g_app.has_patient      = 0;
                    g_app.sim_paused       = 0;
                    g_app.logged_user[0]   = '\0';
                    DestroyWindow(w);
                    g_app.hwnd_dash = NULL;
                    /* Re-open login */
                    g_app.hwnd_login = CreateWindowExA(
                        WS_EX_APPWINDOW, CLASS_LOGIN, APP_TITLE,
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                        CW_USEDEFAULT, CW_USEDEFAULT, 440, 500,
                        NULL, NULL, g_app.instance, NULL);
                    ShowWindow(g_app.hwnd_login, SW_SHOWNORMAL);
                    return 0;
            }
            break;

        case WM_DESTROY:
            KillTimer(w, TIMER_SIM);
            g_app.hwnd_dash = NULL;
            if (g_app.hwnd_login == NULL) PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(w, msg, wp, lp);
}

/* =======================================================================
 * Login window
 * ======================================================================= */

/**
 * @brief Attempt login from the username/password controls.
 *
 * On success: destroys the login window and opens the dashboard.
 * On failure: shows an error message inside the window.
 *
 * @req SWR-GUI-001
 * @req SWR-GUI-002
 */
static void attempt_login(HWND w)
{
    char user[AUTH_MAX_CREDENTIAL_LEN];
    char pass[AUTH_MAX_CREDENTIAL_LEN];

    get_txt(w, IDC_LGN_USER, user, (int)sizeof(user));
    get_txt(w, IDC_LGN_PASS, pass, (int)sizeof(pass));

    if (auth_validate(user, pass)) {
        auth_display_name(user, g_app.logged_user, (int)sizeof(g_app.logged_user));
        DestroyWindow(w);
        g_app.hwnd_login = NULL;
        create_dashboard();
    } else {
        SetWindowTextA(GetDlgItem(w, IDC_LGN_ERR),
                       "Invalid username or password. Please try again.");
        SetFocus(GetDlgItem(w, IDC_LGN_PASS));
        /* Clear the password field on failure */
        SetWindowTextA(GetDlgItem(w, IDC_LGN_PASS), "");
    }
}

static LRESULT CALLBACK login_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {

        case WM_CREATE:
            g_app.hwnd_login = w;

            /* Username */
            make_label(w, "Username:", 70, 196, 300, 18);
            make_edit (w, IDC_LGN_USER, "admin", 70, 216, 300, 28);

            /* Password */
            make_label(w, "Password:", 70, 256, 300, 18);
            {
                HWND ed = CreateWindowExA(
                    WS_EX_CLIENTEDGE, "EDIT", "",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP
                    | ES_AUTOHSCROLL | ES_PASSWORD,
                    70, 276, 300, 28, w,
                    (HMENU)(INT_PTR)IDC_LGN_PASS,
                    g_app.instance, NULL);
                SendMessage(ed, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
            }

            /* Login button (default — Enter triggers it) */
            CreateWindowExA(0, "BUTTON", "SIGN IN",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                            70, 320, 300, 38, w,
                            (HMENU)(INT_PTR)IDC_LGN_BTN,
                            g_app.instance, NULL);

            /* Error label */
            CreateWindowExA(0, "STATIC", "",
                            WS_CHILD | WS_VISIBLE | SS_LEFT,
                            70, 370, 300, 36, w,
                            (HMENU)(INT_PTR)IDC_LGN_ERR,
                            g_app.instance, NULL);

            font_all_children(w, g_app.font_ui);
            /* Bold sign-in button */
            SendMessage(GetDlgItem(w, IDC_LGN_BTN), WM_SETFONT,
                        (WPARAM)g_app.font_status, TRUE);
            /* Error in red */
            SendMessage(GetDlgItem(w, IDC_LGN_ERR), WM_SETFONT,
                        (WPARAM)g_app.font_ui, TRUE);

            SetFocus(GetDlgItem(w, IDC_LGN_PASS));
            return 0;

        case WM_ERASEBKGND: {
            RECT r; GetClientRect(w, &r);
            HBRUSH br = CreateSolidBrush(CLR_NEAR_WHITE);
            FillRect((HDC)wp, &r, br);
            DeleteObject(br);
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(w, &ps);

            /* Dark header region */
            fill_rect(hdc, 0, 0, 440, 160, CLR_NAVY);

            /* Medical cross */
            fill_rect(hdc, 196,  24,  8, 36, CLR_WHITE);   /* vertical   */
            fill_rect(hdc, 182,  36, 36,  8, CLR_WHITE);   /* horizontal */

            /* App name */
            draw_text_ex(hdc, APP_TITLE,
                         20, 70, 400, 30,
                         g_app.font_status, CLR_WHITE,
                         DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            /* Subtitle */
            draw_text_ex(hdc, "Authorized Clinical Use Only",
                         20, 110, 400, 22,
                         g_app.font_ui, RGB(186, 230, 253),
                         DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            /* "Sign In" heading */
            draw_text_ex(hdc, "Sign In",
                         70, 163, 300, 28,
                         g_app.font_status, CLR_DARK_TEXT,
                         DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            EndPaint(w, &ps);
            return 0;
        }

        case WM_CTLCOLORSTATIC:
            if ((HWND)lp == GetDlgItem(w, IDC_LGN_ERR)) {
                SetTextColor((HDC)wp, CLR_CR_FG);
                SetBkMode((HDC)wp, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wp) == IDC_LGN_BTN) {
                attempt_login(w);
                return 0;
            }
            break;

        case WM_DESTROY:
            g_app.hwnd_login = NULL;
            if (g_app.hwnd_dash == NULL) PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(w, msg, wp, lp);
}

/* =======================================================================
 * Dashboard creation
 * ======================================================================= */
static void create_dashboard(void)
{
    RECT wr = {0, 0, WIN_CW, WIN_CH};
    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    AdjustWindowRect(&wr, style, FALSE);

    HWND w = CreateWindowExA(
        WS_EX_APPWINDOW, CLASS_DASHBOARD, APP_TITLE, style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right  - wr.left,
        wr.bottom - wr.top,
        NULL, NULL, g_app.instance, NULL);

    ShowWindow(w, SW_SHOWNORMAL);
    UpdateWindow(w);
}

/* =======================================================================
 * WinMain
 * ======================================================================= */
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev,
                   LPSTR cmd, int show)
{
    WNDCLASSEXA wc;
    MSG msg;
    (void)prev; (void)cmd;

    g_app.instance = inst;

    /* Create GDI resources */
    g_app.font_hdr      = make_font(18,  TRUE);
    g_app.font_tile_val = make_font(18,  TRUE);
    g_app.font_tile_lbl = make_font(9,   FALSE);
    g_app.font_status   = make_font(11,  TRUE);
    g_app.font_ui       = make_font(9,   FALSE);

    /* Register login window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = login_proc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_LOGIN;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExA(&wc);

    /* Register dashboard window class */
    wc.lpfnWndProc   = dash_proc;
    wc.lpszClassName = CLASS_DASHBOARD;
    RegisterClassExA(&wc);

    /* Show login first */
    g_app.hwnd_login = CreateWindowExA(
        WS_EX_APPWINDOW, CLASS_LOGIN, APP_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 440, 500,
        NULL, NULL, inst, NULL);
    ShowWindow(g_app.hwnd_login, show);
    UpdateWindow(g_app.hwnd_login);

    /* Message loop — IsDialogMessage enables Tab and Enter in login window */
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (g_app.hwnd_login != NULL
            && IsDialogMessage(g_app.hwnd_login, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* Clean up GDI resources */
    if (g_app.font_hdr)      DeleteObject(g_app.font_hdr);
    if (g_app.font_tile_val) DeleteObject(g_app.font_tile_val);
    if (g_app.font_tile_lbl) DeleteObject(g_app.font_tile_lbl);
    if (g_app.font_status)   DeleteObject(g_app.font_status);
    if (g_app.font_ui)       DeleteObject(g_app.font_ui);

    return (int)msg.wParam;
}

/**
 * @file gui_main.c
 * @brief Win32 GUI — Patient Vital Signs Monitor v2.7.0
 *
 * Windows:
 *   1. Login (PVM_Login)      — auth with role detection
 *   2. Dashboard (PVM_Dash)   — live monitoring + role-based controls
 *   3. Settings (PVM_Settings) — admin-only user management
 *   4. PassChange (PVM_PwdDlg) — password change / admin-set
 *   5. AddUser (PVM_AddUser)   — admin: create new account
 *
 * All memory is static (no heap). IEC 62304 Class B.
 *
 * @req SWR-GUI-001  @req SWR-GUI-002  @req SWR-GUI-003  @req SWR-GUI-004
 * @req SWR-SEC-001  @req SWR-SEC-002  @req SWR-SEC-003
 * @req SWR-GUI-007  @req SWR-GUI-008  @req SWR-GUI-009  @req SWR-GUI-010
 * @req SWR-VIT-008  @req SWR-NEW-001
 */
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vitals.h"
#include "alerts.h"
#include "patient.h"
#include "news2.h"
#include "alarm_limits.h"
#include "trend.h"
#include "gui_auth.h"
#include "gui_users.h"
#include "hw_vitals.h"
#include "app_config.h"

/* ===================================================================
 * App metadata
 * =================================================================== */
#define APP_TITLE   "Patient Vital Signs Monitor"
#define APP_VERSION "v2.7.0"
#define IDI_APPICON 101

/* ===================================================================
 * Window class names
 * =================================================================== */
#define CLASS_LOGIN    "PVM_Login"
#define CLASS_DASH     "PVM_Dash"
#define CLASS_SETTINGS "PVM_Settings"
#define CLASS_PWDDLG   "PVM_PwdDlg"
#define CLASS_ADDUSER  "PVM_AddUser"

/* ===================================================================
 * Control IDs — Login
 * =================================================================== */
#define IDC_LGN_USER    100
#define IDC_LGN_PASS    101
#define IDC_LGN_BTN     102
#define IDC_LGN_ERR     103
#define IDC_LGN_VER     104

/* ===================================================================
 * Control IDs — Dashboard patient
 * =================================================================== */
#define IDC_PAT_ID      1001
#define IDC_PAT_NAME    1002
#define IDC_PAT_AGE     1003
#define IDC_PAT_WEIGHT  1004
#define IDC_PAT_HEIGHT  1005
#define IDC_BTN_ADMIT   1010

/* ===================================================================
 * Control IDs — Dashboard vitals
 * =================================================================== */
#define IDC_VIT_HR      1101
#define IDC_VIT_SYS     1102
#define IDC_VIT_DIA     1103
#define IDC_VIT_TEMP    1104
#define IDC_VIT_SPO2    1105
#define IDC_VIT_RR      1106  /**< Respiration rate field @req SWR-VIT-008 */
#define IDC_BTN_ADD     1110
#define IDC_BTN_CLEAR   1111

/* ===================================================================
 * Control IDs — Dashboard header buttons + lists
 * =================================================================== */
#define IDC_BTN_SCEN1    1200
#define IDC_BTN_SCEN2    1201
#define IDC_BTN_LOGOUT   1202
#define IDC_BTN_PAUSE    1203   /**< Only shown when simulation is enabled */
#define IDC_BTN_SETTINGS 1204
#define IDC_BTN_ACCOUNT  1205
#define IDC_LIST_ALERTS  1300
#define IDC_LIST_HISTORY 1301
#define TIMER_SIM        1

/* ===================================================================
 * Control IDs — Settings window
 * =================================================================== */
#define IDC_TAB_SETTINGS   1210
#define IDC_LST_USERS      1220
#define IDC_BTN_USER_ADD   1221
#define IDC_BTN_USER_EDIT  1222
#define IDC_BTN_USER_REM   1223
#define IDC_BTN_USER_PWD   1224
#define IDC_BTN_MY_PWD     1227  /**< My Account: change own password */
/* Simulation tab in Settings @req SWR-GUI-010 */
#define IDC_BTN_SIM_TOGGLE 1225
#define IDC_STC_SIM_STATUS 1226

/* ===================================================================
 * Control IDs — Password change dialog
 * =================================================================== */
#define IDC_EDT_CURPWD    1240
#define IDC_EDT_NEWPWD    1241
#define IDC_EDT_CONFPWD   1242
#define IDC_BTN_PWOK      1243
#define IDC_BTN_PWCANCEL  1244
#define IDC_STC_PWERR     1245

/* ===================================================================
 * Control IDs — Alarm Limits tab (Settings)  @req SWR-ALM-001
 * =================================================================== */
#define IDC_ALM_HR_LOW    1400
#define IDC_ALM_HR_HIGH   1401
#define IDC_ALM_SBP_LOW   1402
#define IDC_ALM_SBP_HIGH  1403
#define IDC_ALM_DBP_LOW   1404
#define IDC_ALM_DBP_HIGH  1405
#define IDC_ALM_TMP_LOW   1406
#define IDC_ALM_TMP_HIGH  1407
#define IDC_ALM_SPO2_LOW  1408
#define IDC_ALM_RR_LOW    1409
#define IDC_ALM_RR_HIGH   1410
#define IDC_ALM_BTN_APPLY 1411
#define IDC_ALM_BTN_DEF   1412

/* ===================================================================
 * Control IDs — Add User dialog
 * =================================================================== */
#define IDC_EDT_NEWUSER   1230
#define IDC_EDT_NEWDISP   1231
#define IDC_EDT_NEWPWD2   1232
#define IDC_CMB_NEWROLE   1233
#define IDC_BTN_ADDOK     1234
#define IDC_BTN_ADDCANCEL 1235
#define IDC_STC_ADDERR    1236

/* ===================================================================
 * Layout
 * =================================================================== */
#define WIN_CW  920
#define WIN_CH  850
#define HDR_H    56
#define PBAR_H   38
#define TILE_Y   (HDR_H + PBAR_H)
#define TILE_H   190
#define STAT_Y   (TILE_Y + TILE_H + 6)
#define STAT_H   44
#define CTRL_Y   (STAT_Y + STAT_H + 8)
#define CY       CTRL_Y

/* ===================================================================
 * Colour palette
 * =================================================================== */
#define CLR_NAVY       RGB(30,  58,  138)
#define CLR_SLATE      RGB(51,  65,   85)
#define CLR_NEAR_WHITE RGB(248, 250, 252)
#define CLR_LIGHT_GRAY RGB(226, 232, 240)
#define CLR_DARK_TEXT  RGB(30,  41,   59)
#define CLR_WHITE      RGB(255, 255, 255)
#define CLR_OK_BG      RGB(220, 252, 231)
#define CLR_OK_FG      RGB(21,  128,  61)
#define CLR_WN_BG      RGB(254, 249, 195)
#define CLR_WN_FG      RGB(161,  98,   7)
#define CLR_CR_BG      RGB(254, 226, 226)
#define CLR_CR_FG      RGB(185,  28,  28)
#define CLR_GOLD       RGB(218, 165,  32)
#define CLR_TEAL       RGB( 32, 178, 170)

/* ===================================================================
 * Global application state
 * =================================================================== */
typedef struct {
    HINSTANCE inst;
    HWND      hwnd_login;
    HWND      hwnd_dash;
    HWND      hwnd_settings;
    HWND      hwnd_pwddlg;
    HWND      hwnd_adduser;

    char     logged_user[USERS_MAX_USERNAME_LEN];     /* display name for UI */
    char     logged_username[USERS_MAX_USERNAME_LEN]; /* actual username for auth */
    UserRole logged_role;

    PatientRecord patient;
    int           has_patient;
    int           sim_paused;
    int           sim_enabled;  /**< 1=simulation mode, 0=device/HAL mode @req SWR-GUI-010 */
    int           sim_msg_scroll_offset; /**< Offset for rolling message in simulation mode */
    AlarmLimits   alarm_limits; /**< Configurable per-parameter alarm limits @req SWR-ALM-001 */

    HFONT font_hdr;
    HFONT font_tile_val;
    HFONT font_tile_lbl;
    HFONT font_status;
    HFONT font_ui;
    HICON app_icon;
} AppState;

static AppState g_app;

/* Password-change dialog context (static — no heap) */
static struct {
    char target_user[USERS_MAX_USERNAME_LEN];
    int  admin_mode;   /* 0 = self-change, 1 = admin-set */
} g_pwd_ctx;

/* ===================================================================
 * Forward declarations
 * =================================================================== */
static LRESULT CALLBACK login_proc   (HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK dash_proc    (HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK settings_proc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK pwddlg_proc  (HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK adduser_proc (HWND, UINT, WPARAM, LPARAM);
static void create_dashboard(void);
static void update_dashboard(HWND w);
static void apply_sim_mode(HWND dash);
static void open_settings(HWND parent);
static void open_pwddlg(HWND parent, const char *user, int admin_mode);
static void open_adduser(HWND parent);

/* ===================================================================
 * GDI helpers
 * =================================================================== */
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
    RECT r = {x, y, x+w, y+h};
    HBRUSH br = CreateSolidBrush(c);
    FillRect(hdc, &r, br);
    DeleteObject(br);
}

static void draw_text_ex(HDC hdc, const char *txt,
                          int x, int y, int w, int h,
                          HFONT font, COLORREF fg, UINT fmt)
{
    RECT r = {x, y, x+w, y+h};
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, fg);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextA(hdc, txt, -1, &r, fmt);
    SelectObject(hdc, old);
}

static void draw_pill(HDC hdc, int x, int y, int w, int h,
                       COLORREF bg, const char *txt, HFONT font)
{
    HBRUSH br  = CreateSolidBrush(bg);
    HPEN   pen = CreatePen(PS_SOLID, 1, bg);
    HBRUSH obr = (HBRUSH)SelectObject(hdc, br);
    HPEN   open = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x+w, y+h, h, h);
    SelectObject(hdc, obr); SelectObject(hdc, open);
    DeleteObject(br); DeleteObject(pen);
    draw_text_ex(hdc, txt, x, y, w, h, font, CLR_WHITE,
                 DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

/* ===================================================================
 * Painted zones — header
 * =================================================================== */
static void paint_header(HDC hdc, int cw)
{
    char buf[128];
    fill_rect(hdc, 0, 0, cw, HDR_H, CLR_NAVY);

    /* Medical cross (white GDI rects) */
    fill_rect(hdc, 14,  12, 10, 32, CLR_WHITE);
    fill_rect(hdc,  9,  22, 20, 12, CLR_WHITE);

    /* App title */
    draw_text_ex(hdc, "  " APP_TITLE,
                 38, 0, cw - 480, HDR_H,
                 g_app.font_hdr, CLR_WHITE,
                 DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    /* Info block — placed left of the header buttons (rightmost ~280px reserved for buttons) */
    if (g_app.logged_user[0]) {
        COLORREF badge_bg = (g_app.logged_role == ROLE_ADMIN) ? CLR_GOLD : CLR_TEAL;
        const char *badge_txt = (g_app.logged_role == ROLE_ADMIN) ? "ADMIN" : "CLINICAL";
        snprintf(buf, sizeof(buf), "  %s", g_app.logged_user);
        draw_text_ex(hdc, buf,
                     cw - 560, 0, 160, HDR_H,
                     g_app.font_ui, RGB(186, 230, 253),
                     DT_SINGLELINE | DT_VCENTER | DT_LEFT);
        draw_pill(hdc, cw - 400, 15, 86, 26, badge_bg, badge_txt, g_app.font_tile_lbl);
    }

    /* Sim status indicator — only shown when simulation is active */
    if (g_app.sim_enabled) {
        const char *mode_txt = g_app.sim_paused ? "SIM PAUSED" : "* SIM LIVE";
        COLORREF    mode_clr = g_app.sim_paused ? RGB(253,224,71) : RGB(134,239,172);
        draw_text_ex(hdc, mode_txt,
                     cw - 560, 32, 108, 22,
                     g_app.font_tile_lbl, mode_clr,
                     DT_SINGLELINE | DT_LEFT);
    }
}

/* ===================================================================
 * Painted zones — patient bar, tiles, status banner
 * =================================================================== */
static void paint_patient_bar(HDC hdc, int cw)
{
    char buf[256];
    fill_rect(hdc, 0, HDR_H, cw, PBAR_H, CLR_SLATE);
    if (!g_app.sim_enabled) {
        snprintf(buf, sizeof(buf),
                 "  DEVICE MODE — Simulation disabled. Connect real hardware for live data.");
    } else if (g_app.has_patient) {
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
    {
        COLORREF pbar_fg = (!g_app.sim_enabled)  ? RGB(100, 116, 139) :
                           (g_app.has_patient)   ? CLR_LIGHT_GRAY     :
                                                   RGB(148, 163, 184);
        draw_text_ex(hdc, buf, 0, HDR_H, cw, PBAR_H,
                     g_app.font_ui, pbar_fg,
                     DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    }
}

/**
 * @brief Draw a mini sparkline from @p vals[0..n-1] into a bounding rectangle.
 *
 * The sparkline occupies the bottom strip of a tile (between the value text and
 * the badge row). Points are scaled to fit the full width and height of the
 * designated strip. Uses GDI Polyline for hardware-accelerated rendering.
 *
 * @req SWR-TRD-001
 */
static void paint_sparkline(HDC hdc, int sx, int sy, int sw, int sh,
                             const int *vals, int n, COLORREF clr)
{
    POINT pts[MAX_READINGS];
    int v_min, v_max, range, i;
    HPEN pen, open;

    if (n < 2 || sw < 2 || sh < 2) return;

    /* Find min/max for scaling */
    v_min = vals[0]; v_max = vals[0];
    for (i = 1; i < n; ++i) {
        if (vals[i] < v_min) v_min = vals[i];
        if (vals[i] > v_max) v_max = vals[i];
    }
    range = v_max - v_min;
    if (range == 0) range = 1; /* flat line in centre */

    for (i = 0; i < n; ++i) {
        pts[i].x = sx + (i * (sw - 1)) / (n - 1);
        /* Invert Y: high value = top of strip */
        pts[i].y = sy + sh - 1 - ((vals[i] - v_min) * (sh - 1)) / range;
    }

    pen  = CreatePen(PS_SOLID, 1, clr);
    open = (HPEN)SelectObject(hdc, pen);
    Polyline(hdc, pts, n);
    SelectObject(hdc, open);
    DeleteObject(pen);
}

static void paint_tile(HDC hdc,
                        int tx, int ty, int tw, int th,
                        const char *label, const char *value, const char *unit,
                        AlertLevel level,
                        const int *spark_vals, int spark_n)
{
    COLORREF bg, fg;
    char badge[24], full_val[48];
    /* vertical split: label top, value middle, sparkline, badge bottom */
    int lbl_y  = ty + 6;                  /* label row top */
    int val_y  = ty + 26;                 /* value row top (below label) */
    int val_h  = th - 26 - 26;           /* value height: leaves room for sparkline + badge */
    int spk_y  = ty + th - 30;           /* sparkline strip top */
    int spk_h  = 12;                      /* sparkline strip height */
    int bdg_y  = ty + th - 18;           /* badge row top (bottom-anchored) */
    HPEN pen; HPEN open; HBRUSH obr;

    switch (level) {
        case ALERT_WARNING:  bg=CLR_WN_BG; fg=CLR_WN_FG; snprintf(badge,sizeof(badge),"  WARNING");  break;
        case ALERT_CRITICAL: bg=CLR_CR_BG; fg=CLR_CR_FG; snprintf(badge,sizeof(badge),"  CRITICAL"); break;
        default:             bg=CLR_OK_BG; fg=CLR_OK_FG; snprintf(badge,sizeof(badge),"  NORMAL");   break;
    }
    fill_rect(hdc, tx, ty, tw, th, bg);
    pen  = CreatePen(PS_SOLID, 2, fg);
    open = (HPEN) SelectObject(hdc, pen);
    obr  = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, tx+1, ty+1, tx+tw-1, ty+th-1, 10, 10);
    SelectObject(hdc, open); SelectObject(hdc, obr); DeleteObject(pen);

    /* Don't append unit when value is N/A */
    if (strcmp(value, "N/A") == 0)
        snprintf(full_val, sizeof(full_val), "N/A");
    else
        snprintf(full_val, sizeof(full_val), "%s %s", value, unit);

    draw_text_ex(hdc, label,     tx+10, lbl_y,  tw-20, 18,    g_app.font_tile_lbl, fg,           DT_SINGLELINE|DT_LEFT);
    draw_text_ex(hdc, full_val,  tx+8,  val_y,  tw-16, val_h, g_app.font_tile_val, CLR_DARK_TEXT, DT_SINGLELINE|DT_LEFT|DT_VCENTER);
    /* Sparkline strip — draw only when we have at least 2 data points */
    if (spark_n >= 2)
        paint_sparkline(hdc, tx+8, spk_y, tw-16, spk_h, spark_vals, spark_n, fg);
    draw_text_ex(hdc, badge,     tx+8,  bdg_y,  tw-16, 18,    g_app.font_tile_lbl, fg,           DT_SINGLELINE|DT_LEFT);
}

static void paint_tiles(HDC hdc, int cw)
{
    const VitalSigns *v = NULL;
    char hr_s[16], bp_s[24], tp_s[16], sp_s[16], rr_s[16];
    AlertLevel lhr=ALERT_NORMAL, lbp=ALERT_NORMAL, ltp=ALERT_NORMAL,
               lsp=ALERT_NORMAL, lrr=ALERT_NORMAL;
    /* 3-column layout — 5 vital tiles, 6th slot = NEWS2 */
    int pad=10, tw=(cw-4*pad)/3, th=(TILE_H-3*pad)/2;
    /* Sparkline arrays (last up to MAX_READINGS points per parameter) @req SWR-TRD-001 */
    int spk_hr[MAX_READINGS], spk_sbp[MAX_READINGS], spk_tp[MAX_READINGS];
    int spk_sp[MAX_READINGS], spk_rr[MAX_READINGS];
    int n_hr=0, n_sbp=0, n_tp=0, n_sp=0, n_rr=0;

    fill_rect(hdc, 0, TILE_Y, cw, TILE_H+pad, CLR_NEAR_WHITE);
    if (!g_app.sim_enabled) {
        /* Device / HAL mode — no simulation data available */
        strncpy(hr_s,"N/A",sizeof(hr_s)-1);  hr_s[sizeof(hr_s)-1]='\0';
        strncpy(bp_s,"N/A",sizeof(bp_s)-1);  bp_s[sizeof(bp_s)-1]='\0';
        strncpy(tp_s,"N/A",sizeof(tp_s)-1);  tp_s[sizeof(tp_s)-1]='\0';
        strncpy(sp_s,"N/A",sizeof(sp_s)-1);  sp_s[sizeof(sp_s)-1]='\0';
        strncpy(rr_s,"N/A",sizeof(rr_s)-1);  rr_s[sizeof(rr_s)-1]='\0';
    } else {
        if (g_app.has_patient) v = patient_latest_reading(&g_app.patient);
        if (v) {
            snprintf(hr_s, sizeof(hr_s), "%d",      v->heart_rate);
            snprintf(bp_s, sizeof(bp_s), "%d / %d", v->systolic_bp, v->diastolic_bp);
            snprintf(tp_s, sizeof(tp_s), "%.1f",    v->temperature);
            snprintf(sp_s, sizeof(sp_s), "%d",      v->spo2);
            if (v->respiration_rate != 0)
                snprintf(rr_s, sizeof(rr_s), "%d", v->respiration_rate);
            else
                strncpy(rr_s, "--", sizeof(rr_s)-1), rr_s[sizeof(rr_s)-1]='\0';
            lhr=check_heart_rate(v->heart_rate);
            lbp=check_blood_pressure(v->systolic_bp, v->diastolic_bp);
            ltp=check_temperature(v->temperature);
            lsp=check_spo2(v->spo2);
            if (v->respiration_rate != 0)
                lrr=check_respiration_rate(v->respiration_rate);
            /* Extract sparkline data from patient history @req SWR-TRD-001 */
            {
                const VitalSigns *rd = g_app.patient.readings;
                int cnt = g_app.patient.reading_count;
                n_hr  = trend_extract_hr  (rd, cnt, spk_hr,  MAX_READINGS);
                n_sbp = trend_extract_sbp (rd, cnt, spk_sbp, MAX_READINGS);
                n_tp  = trend_extract_temp(rd, cnt, spk_tp,  MAX_READINGS);
                n_sp  = trend_extract_spo2(rd, cnt, spk_sp,  MAX_READINGS);
                n_rr  = trend_extract_rr  (rd, cnt, spk_rr,  MAX_READINGS);
            }
        } else {
            strncpy(hr_s,"--",   sizeof(hr_s)-1);  hr_s[sizeof(hr_s)-1]='\0';
            strncpy(bp_s,"--/--",sizeof(bp_s)-1);  bp_s[sizeof(bp_s)-1]='\0';
            strncpy(tp_s,"--",   sizeof(tp_s)-1);  tp_s[sizeof(tp_s)-1]='\0';
            strncpy(sp_s,"--",   sizeof(sp_s)-1);  sp_s[sizeof(sp_s)-1]='\0';
            strncpy(rr_s,"--",   sizeof(rr_s)-1);  rr_s[sizeof(rr_s)-1]='\0';
        }
    }
    /* Row 1 */
    paint_tile(hdc, pad,             TILE_Y+pad,        tw, th, "HEART RATE",     hr_s, "bpm",    lhr, spk_hr,  n_hr);
    paint_tile(hdc, pad+tw+pad,      TILE_Y+pad,        tw, th, "BLOOD PRESSURE", bp_s, "mmHg",   lbp, spk_sbp, n_sbp);
    paint_tile(hdc, pad+2*(tw+pad),  TILE_Y+pad,        tw, th, "TEMPERATURE",    tp_s, "C",      ltp, spk_tp,  n_tp);
    /* Row 2 */
    paint_tile(hdc, pad,             TILE_Y+pad+th+pad, tw, th, "SpO2",           sp_s, "%",      lsp, spk_sp,  n_sp);
    paint_tile(hdc, pad+tw+pad,      TILE_Y+pad+th+pad, tw, th, "RESP RATE",      rr_s, "br/min", lrr, spk_rr,  n_rr);
    /* 6th tile: NEWS2 Early Warning Score @req SWR-NEW-001 */
    {
        char n2_score[8] = "--";
        AlertLevel n2_lvl = ALERT_NORMAL;
        const char *n2_unit = "";
        if (g_app.sim_enabled && v) {
            News2Result n2;
            news2_calculate(v, 0, &n2);
            snprintf(n2_score, sizeof(n2_score), "%d", n2.total_score);
            n2_unit = n2.risk_label;
            switch (n2.risk) {
                case NEWS2_HIGH:   n2_lvl = ALERT_CRITICAL; break;
                case NEWS2_MEDIUM: n2_lvl = ALERT_WARNING;  break;
                default:           n2_lvl = ALERT_NORMAL;   break;
            }
        } else if (!g_app.sim_enabled) {
            strncpy(n2_score, "N/A", sizeof(n2_score)-1);
            n2_score[sizeof(n2_score)-1] = '\0';
        }
        paint_tile(hdc, pad+2*(tw+pad), TILE_Y+pad+th+pad, tw, th,
                   "NEWS2 SCORE", n2_score, n2_unit, n2_lvl, NULL, 0);
    }
}

static void paint_status_banner(HDC hdc, int cw)
{
    COLORREF bg, fg; const char *txt;
    char rolling_msg[256];
    int offset;

    if (!g_app.sim_enabled) {
        bg  = CLR_SLATE;
        fg  = CLR_LIGHT_GRAY;
        txt = "DEVICE MODE — Enable simulation in Settings to use synthetic data";
        fill_rect(hdc, 0, STAT_Y, cw, STAT_H, bg);
        draw_text_ex(hdc, txt, 0, STAT_Y, cw, STAT_H,
                     g_app.font_status, fg, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    } else {
        /* In simulation mode, show rolling message */
        offset = g_app.sim_msg_scroll_offset;

        /* Create a long repeating message for rolling effect */
        snprintf(rolling_msg, sizeof(rolling_msg),
                 "   ✦  hi lee how are you  ✦   hi lee how are you  ✦   hi lee how are you  ✦   hi lee how are you  ✦");

        AlertLevel lvl = g_app.has_patient ? patient_current_status(&g_app.patient) : ALERT_NORMAL;
        switch (lvl) {
            case ALERT_CRITICAL: bg=CLR_CR_FG; fg=CLR_WHITE; break;
            case ALERT_WARNING:  bg=CLR_WN_FG; fg=CLR_WHITE; break;
            default:
                bg=CLR_OK_FG; fg=CLR_WHITE;
                break;
        }

        fill_rect(hdc, 0, STAT_Y, cw, STAT_H, bg);

        /* Draw rolling message with offset */
        RECT r = {-offset, STAT_Y, cw + 500, STAT_Y + STAT_H};
        SetTextColor(hdc, fg);
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, g_app.font_status);
        DrawTextA(hdc, rolling_msg, -1, &r, DT_SINGLELINE|DT_VCENTER|DT_LEFT);
    }
}

/* ===================================================================
 * Control helpers
 * =================================================================== */
static HWND make_label(HWND p, const char *t, int x, int y, int w, int h)
{
    return CreateWindowExA(0,"STATIC",t,WS_CHILD|WS_VISIBLE,x,y,w,h,p,NULL,g_app.inst,NULL);
}
static HWND make_edit(HWND p, int id, const char *t, int x, int y, int w, int h)
{
    return CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT",t,
                           WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
                           x,y,w,h,p,(HMENU)(INT_PTR)id,g_app.inst,NULL);
}
static HWND make_btn(HWND p, int id, const char *t, int x, int y, int w, int h)
{
    return CreateWindowExA(0,"BUTTON",t,
                           WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
                           x,y,w,h,p,(HMENU)(INT_PTR)id,g_app.inst,NULL);
}
static void font_children(HWND p, HFONT f)
{
    HWND c = GetWindow(p, GW_CHILD);
    while (c) { SendMessage(c,WM_SETFONT,(WPARAM)f,TRUE); c=GetWindow(c,GW_HWNDNEXT); }
}
static int get_txt(HWND p, int id, char *out, int len)
{
    if (len<=0) return 0;
    out[0]='\0';
    GetWindowTextA(GetDlgItem(p,id), out, len);
    return (int)strlen(out);
}
static void set_txt(HWND p, int id, const char *s) { SetWindowTextA(GetDlgItem(p,id),s); }

/* ===================================================================
 * Dashboard: controls
 * =================================================================== */
static void create_dash_controls(HWND w)
{
    make_label(w,"ID",             20, CY,      40, 18);
    make_edit (w,IDC_PAT_ID, "1001",  20, CY+20, 100, 24);
    make_label(w,"Full Name",     130, CY,      90, 18);
    make_edit (w,IDC_PAT_NAME,"Sarah Johnson",130,CY+20,240,24);
    make_label(w,"Age",           382, CY,      40, 18);
    make_edit (w,IDC_PAT_AGE, "52",  382,CY+20,  70, 24);
    make_label(w,"Weight (kg)",   464, CY,      90, 18);
    make_edit (w,IDC_PAT_WEIGHT,"72.5",464,CY+20,90,24);
    make_label(w,"Height (m)",    566, CY,      90, 18);
    make_edit (w,IDC_PAT_HEIGHT,"1.66",566,CY+20,90,24);
    make_btn  (w,IDC_BTN_ADMIT,"Admit / Refresh",670,CY+18,130,28);

    make_label(w,"HR (bpm)",     20, CY+62,  80, 18);
    make_edit (w,IDC_VIT_HR,  "78", 20, CY+82, 90, 24);
    make_label(w,"Systolic",    122,CY+62,  70, 18);
    make_edit (w,IDC_VIT_SYS,"122",122,CY+82,  90, 24);
    make_label(w,"Diastolic",   224,CY+62,  70, 18);
    make_edit (w,IDC_VIT_DIA, "82",224,CY+82,  90, 24);
    make_label(w,"Temp (C)",    326,CY+62,  70, 18);
    make_edit (w,IDC_VIT_TEMP,"36.7",326,CY+82, 90,24);
    make_label(w,"SpO2 (%)",    428,CY+62,  70, 18);
    make_edit (w,IDC_VIT_SPO2,"98", 428,CY+82,  90, 24);
    make_label(w,"RR (br/min)", 530,CY+62,  80, 18);
    make_edit (w,IDC_VIT_RR,  "15", 530,CY+82,  90, 24);
    make_btn  (w,IDC_BTN_ADD, "Add Reading",  634,CY+80,110,28);
    make_btn  (w,IDC_BTN_CLEAR,"Clear Session",756,CY+80,110,28);

    make_btn(w,IDC_BTN_SCEN1,"Demo: Deterioration",20, CY+124,175,26);
    make_btn(w,IDC_BTN_SCEN2,"Demo: Bradycardia",  205,CY+124,160,26);

    make_label(w,"Active Alerts",20,CY+162,160,18);
    CreateWindowExA(WS_EX_CLIENTEDGE,"LISTBOX","",
                    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,
                    20,CY+182,872,88,w,(HMENU)(INT_PTR)IDC_LIST_ALERTS,g_app.inst,NULL);

    make_label(w,"Reading History",20,CY+282,160,18);
    CreateWindowExA(WS_EX_CLIENTEDGE,"LISTBOX","",
                    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,
                    20,CY+302,872,130,w,(HMENU)(INT_PTR)IDC_LIST_HISTORY,g_app.inst,NULL);
}

/* ===================================================================
 * Dashboard: dynamic control repositioning on WM_SIZE
 * All header buttons anchor to right edge; listboxes stretch to fill.
 * =================================================================== */
static void reposition_dash_controls(HWND w, int cw)
{
    HWND btn;
    /* Header buttons — right-edge anchored */
    SetWindowPos(GetDlgItem(w, IDC_BTN_LOGOUT), NULL, cw - 86,  14, 72, 28, SWP_NOZORDER|SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(w, IDC_BTN_PAUSE),  NULL, cw - 176, 14, 86, 28, SWP_NOZORDER|SWP_NOACTIVATE);

    btn = GetDlgItem(w, IDC_BTN_SETTINGS);
    if (btn)  SetWindowPos(btn, NULL, cw - 272, 14, 86, 28, SWP_NOZORDER|SWP_NOACTIVATE);

    /* Wide list boxes — stretch horizontally */
    SetWindowPos(GetDlgItem(w, IDC_LIST_ALERTS),  NULL, 20, CY+182, cw - 40, 88,  SWP_NOZORDER|SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(w, IDC_LIST_HISTORY), NULL, 20, CY+302, cw - 40, 130, SWP_NOZORDER|SWP_NOACTIVATE);
}

/* ===================================================================
 * Dashboard: data update
 * =================================================================== */
static void update_dashboard(HWND w)
{
    char buf[256];
    const VitalSigns *latest = NULL;
    Alert alerts[MAX_ALERTS];
    int   ac = 0, i;

    SendMessageA(GetDlgItem(w,IDC_LIST_HISTORY),LB_RESETCONTENT,0,0);
    SendMessageA(GetDlgItem(w,IDC_LIST_ALERTS), LB_RESETCONTENT,0,0);
    InvalidateRect(w, NULL, FALSE);

    if (!g_app.has_patient) {
        SendMessageA(GetDlgItem(w,IDC_LIST_ALERTS),LB_ADDSTRING,0,(LPARAM)"No patient admitted yet.");
        return;
    }
    for (i = 0; i < g_app.patient.reading_count; ++i) {
        const VitalSigns *r = &g_app.patient.readings[i];
        if (r->respiration_rate != 0)
            snprintf(buf, sizeof(buf),
                     "#%d  HR %d | BP %d/%d | Temp %.1f C | SpO2 %d%% | RR %d br/min  [%s]",
                     i+1, r->heart_rate, r->systolic_bp, r->diastolic_bp,
                     r->temperature, r->spo2, r->respiration_rate,
                     alert_level_str(overall_alert_level(r)));
        else
            snprintf(buf, sizeof(buf),
                     "#%d  HR %d | BP %d/%d | Temp %.1f C | SpO2 %d%%  [%s]",
                     i+1, r->heart_rate, r->systolic_bp, r->diastolic_bp,
                     r->temperature, r->spo2,
                     alert_level_str(overall_alert_level(r)));
        SendMessageA(GetDlgItem(w,IDC_LIST_HISTORY),LB_ADDSTRING,0,(LPARAM)buf);
    }
    latest = patient_latest_reading(&g_app.patient);
    if (latest) ac = generate_alerts(latest, alerts, MAX_ALERTS);
    if (ac == 0) {
        SendMessageA(GetDlgItem(w,IDC_LIST_ALERTS),LB_ADDSTRING,0,
                     (LPARAM)"No active alerts — all parameters within normal range.");
    } else {
        for (i = 0; i < ac; ++i) {
            const char *sev = (alerts[i].level==ALERT_CRITICAL)?"CRITICAL":"WARNING ";
            snprintf(buf, sizeof(buf), "[%s]  %s", sev, alerts[i].message);
            SendMessageA(GetDlgItem(w,IDC_LIST_ALERTS),LB_ADDSTRING,0,(LPARAM)buf);
        }
    }
}

/* ===================================================================
 * Dashboard: input helpers
 * =================================================================== */
static int parse_int_field(HWND p, int id, const char *label, int *out)
{
    char buf[64]; char *ep=NULL; long v;
    if (!get_txt(p,id,buf,(int)sizeof(buf))) {
        char m[128]; snprintf(m,sizeof(m),"%s is required.",label);
        MessageBoxA(p,m,APP_TITLE,MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(p,id)); return 0;
    }
    v=strtol(buf,&ep,10);
    if (ep==buf||*ep!='\0') {
        char m[128]; snprintf(m,sizeof(m),"%s must be a whole number.",label);
        MessageBoxA(p,m,APP_TITLE,MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(p,id)); return 0;
    }
    *out=(int)v; return 1;
}
static int parse_flt_field(HWND p, int id, const char *label, float *out)
{
    char buf[64]; char *ep=NULL; double v;
    if (!get_txt(p,id,buf,(int)sizeof(buf))) {
        char m[128]; snprintf(m,sizeof(m),"%s is required.",label);
        MessageBoxA(p,m,APP_TITLE,MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(p,id)); return 0;
    }
    v=strtod(buf,&ep);
    if (ep==buf||*ep!='\0') {
        char m[128]; snprintf(m,sizeof(m),"%s must be a decimal number.",label);
        MessageBoxA(p,m,APP_TITLE,MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(p,id)); return 0;
    }
    *out=(float)v; return 1;
}

/* ===================================================================
 * Dashboard: actions
 * =================================================================== */
static int do_admit(HWND w)
{
    int id,age; float wt,ht; char name[MAX_NAME_LEN];
    if (!parse_int_field(w,IDC_PAT_ID,    "Patient ID", &id))  return 0;
    if (!parse_int_field(w,IDC_PAT_AGE,   "Age",        &age)) return 0;
    if (!parse_flt_field(w,IDC_PAT_WEIGHT,"Weight (kg)",&wt))  return 0;
    if (!parse_flt_field(w,IDC_PAT_HEIGHT,"Height (m)", &ht))  return 0;
    if (!get_txt(w,IDC_PAT_NAME,name,(int)sizeof(name))) {
        MessageBoxA(w,"Patient name is required.",APP_TITLE,MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(w,IDC_PAT_NAME)); return 0;
    }
    patient_init(&g_app.patient,id,name,age,wt,ht);
    g_app.has_patient=1;
    update_dashboard(w); return 1;
}
static void do_add_reading(HWND w)
{
    VitalSigns v;
    if (!g_app.has_patient && !do_admit(w)) return;
    if (!parse_int_field(w,IDC_VIT_HR,  "Heart Rate",   &v.heart_rate))     return;
    if (!parse_int_field(w,IDC_VIT_SYS, "Systolic BP",  &v.systolic_bp))    return;
    if (!parse_int_field(w,IDC_VIT_DIA, "Diastolic BP", &v.diastolic_bp))   return;
    if (!parse_flt_field(w,IDC_VIT_TEMP,"Temperature",  &v.temperature))    return;
    if (!parse_int_field(w,IDC_VIT_SPO2,"SpO2",         &v.spo2))           return;
    if (!parse_int_field(w,IDC_VIT_RR,  "Resp Rate",    &v.respiration_rate)) return;
    if (!patient_add_reading(&g_app.patient,&v)) {
        MessageBoxA(w,"Reading buffer full (10 readings). Clear session to continue.",
                    APP_TITLE,MB_OK|MB_ICONWARNING); return;
    }
    update_dashboard(w);
}
static void do_clear(HWND w)
{
    ZeroMemory(&g_app.patient,sizeof(g_app.patient));
    g_app.has_patient=0;
    set_txt(w,IDC_PAT_ID,    "1001"); set_txt(w,IDC_PAT_NAME,"Sarah Johnson");
    set_txt(w,IDC_PAT_AGE,   "52");   set_txt(w,IDC_PAT_WEIGHT,"72.5");
    set_txt(w,IDC_PAT_HEIGHT,"1.66"); set_txt(w,IDC_VIT_HR,"78");
    set_txt(w,IDC_VIT_SYS,  "122");   set_txt(w,IDC_VIT_DIA,"82");
    set_txt(w,IDC_VIT_TEMP, "36.7");  set_txt(w,IDC_VIT_SPO2,"98");
    set_txt(w,IDC_VIT_RR,   "15");
    update_dashboard(w);
}
static void do_scenario(HWND w, int s)
{
    static const VitalSigns det[3]={{78,122,82,36.7f,98,15},{108,148,94,37.9f,93,23},{135,175,112,39.8f,87,27}};
    static const VitalSigns bra[2]={{68,118,76,36.5f,99,14},{38,110,72,36.6f,97,8}};
    const VitalSigns *rd; int n,i;
    if (s==1) {
        set_txt(w,IDC_PAT_ID,"1001"); set_txt(w,IDC_PAT_NAME,"Sarah Johnson");
        set_txt(w,IDC_PAT_AGE,"52");  set_txt(w,IDC_PAT_WEIGHT,"72.5");
        set_txt(w,IDC_PAT_HEIGHT,"1.66"); rd=det; n=3;
    } else {
        set_txt(w,IDC_PAT_ID,"1002"); set_txt(w,IDC_PAT_NAME,"David Okonkwo");
        set_txt(w,IDC_PAT_AGE,"34");  set_txt(w,IDC_PAT_WEIGHT,"85.0");
        set_txt(w,IDC_PAT_HEIGHT,"1.80"); rd=bra; n=2;
    }
    if (!do_admit(w)) return;
    for (i=0;i<n;++i) patient_add_reading(&g_app.patient,&rd[i]);
    update_dashboard(w);
}

/* ===================================================================
 * Settings window — user list refresh helper
 * =================================================================== */
static void settings_refresh_list(HWND hw_list)
{
    UserAccount acct;
    char row[192];
    int i, cnt;
    SendMessageA(hw_list, LB_RESETCONTENT, 0, 0);
    cnt = users_count();
    for (i = 0; i < cnt; ++i) {
        if (!users_get_by_index(i, &acct)) continue;
        snprintf(row, sizeof(row), "%-20s  |  %-24s  |  %s",
                 acct.username, acct.display_name,
                 acct.role == ROLE_ADMIN ? "ADMIN" : "CLINICAL");
        SendMessageA(hw_list, LB_ADDSTRING, 0, (LPARAM)row);
    }
}

/* ===================================================================
 * Settings window procedure
 * =================================================================== */
static LRESULT CALLBACK settings_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    static HWND hw_tab, hw_list;
    static HWND hw_btn_add, hw_btn_edit, hw_btn_rem, hw_btn_pwd;
    static HWND sim_ctrls[6];
    static int  sim_count  = 0;
    static HWND about_ctrls[8];
    static int  about_count = 0;
    /* Alarm limits tab control handles */
    static HWND alm_ctrls[48];
    static int  alm_count = 0;
    /* My Account tab control handles */
    static HWND acct_ctrls[4];
    static int  acct_count = 0;

    switch (msg) {
    case WM_CREATE: {
        TCITEMA ti;
        int i;
        g_app.hwnd_settings = w;

        hw_tab = CreateWindowExA(0, WC_TABCONTROLA, "",
                     WS_CHILD|WS_VISIBLE|TCS_TABS,
                     8, 8, 534, 460, w, (HMENU)(INT_PTR)IDC_TAB_SETTINGS,
                     g_app.inst, NULL);
        SendMessage(hw_tab, WM_SETFONT, (WPARAM)g_app.font_ui, TRUE);

        ZeroMemory(&ti, sizeof(ti)); ti.mask = TCIF_TEXT;
        if (g_app.logged_role == ROLE_ADMIN) {
            ti.pszText = "Users";         TabCtrl_InsertItem(hw_tab, 0, &ti);
            ti.pszText = "Simulation";    TabCtrl_InsertItem(hw_tab, 1, &ti);
            ti.pszText = "About";         TabCtrl_InsertItem(hw_tab, 2, &ti);
            ti.pszText = "Alarm Limits";  TabCtrl_InsertItem(hw_tab, 3, &ti);
            ti.pszText = "My Account";    TabCtrl_InsertItem(hw_tab, 4, &ti);
        } else {
            /* Clinical users: no Users tab */
            ti.pszText = "Simulation";    TabCtrl_InsertItem(hw_tab, 0, &ti);
            ti.pszText = "Alarm Limits";  TabCtrl_InsertItem(hw_tab, 1, &ti);
            ti.pszText = "My Account";    TabCtrl_InsertItem(hw_tab, 2, &ti);
            ti.pszText = "About";         TabCtrl_InsertItem(hw_tab, 3, &ti);
        }

        /* --- Users tab controls --- */
        hw_list = CreateWindowExA(WS_EX_CLIENTEDGE,"LISTBOX","",
                      WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,
                      16, 52, 390, 340, w, (HMENU)(INT_PTR)IDC_LST_USERS,
                      g_app.inst, NULL);
        hw_btn_add  = make_btn(w, IDC_BTN_USER_ADD,  "Add User",    420, 52,  118, 28);
        hw_btn_edit = make_btn(w, IDC_BTN_USER_EDIT, "Edit Name",   420, 88,  118, 28);
        hw_btn_rem  = make_btn(w, IDC_BTN_USER_REM,  "Remove",      420, 124, 118, 28);
        hw_btn_pwd  = make_btn(w, IDC_BTN_USER_PWD,  "Set Password",420, 160, 118, 28);
        EnableWindow(hw_btn_edit, FALSE);
        EnableWindow(hw_btn_rem,  FALSE);
        EnableWindow(hw_btn_pwd,  FALSE);

        /* --- Simulation tab controls @req SWR-GUI-010 --- */
        sim_count = 0;
        sim_ctrls[sim_count++] = make_label(w,
            "Simulation Mode",
            16, 58, 520, 22);
        sim_ctrls[sim_count++] = make_label(w,
            "When ENABLED: synthetic vital signs are generated every 2 seconds,\r\n"
            "cycling through a clinical scenario (Normal \x2192 Warning \x2192 Critical \x2192 Recovery).",
            16, 86, 520, 40);
        sim_ctrls[sim_count++] = make_label(w,
            "When DISABLED: all vital tiles show N/A. Connect real hardware\r\n"
            "(implement hw_driver.c against hw_vitals.h) for live data.",
            16, 134, 520, 40);
        {
            char status_txt[64];
            snprintf(status_txt, sizeof(status_txt),
                     "Current status:  %s",
                     g_app.sim_enabled ? "ENABLED" : "DISABLED");
            sim_ctrls[sim_count++] = CreateWindowExA(0, "STATIC", status_txt,
                WS_CHILD|WS_VISIBLE|SS_LEFT,
                16, 190, 520, 22, w, (HMENU)(INT_PTR)IDC_STC_SIM_STATUS,
                g_app.inst, NULL);
        }
        sim_ctrls[sim_count++] = make_btn(w, IDC_BTN_SIM_TOGGLE,
            g_app.sim_enabled ? "Disable Simulation" : "Enable Simulation",
            16, 222, 180, 30);
        sim_ctrls[sim_count++] = make_label(w,
            "Note: The selected mode is saved and restored on next launch.",
            16, 264, 520, 20);

        for (i = 0; i < sim_count; ++i)
            ShowWindow(sim_ctrls[i], SW_HIDE);

        /* --- About tab controls --- */
        about_count = 0;
        about_ctrls[about_count++] = make_label(w,"Patient Vital Signs Monitor",          16,52,520,24);
        about_ctrls[about_count++] = make_label(w,"Version " APP_VERSION,                 16,84,520,20);
        about_ctrls[about_count++] = make_label(w,"IEC 62304 Class B",                    16,112,520,20);
        about_ctrls[about_count++] = make_label(w,"Requirements Revision: SWR-001-REV-E", 16,136,520,20);
        about_ctrls[about_count++] = make_label(w,"Architecture: docs/ARCHITECTURE.md",   16,156,520,20);
        about_ctrls[about_count++] = make_label(w,"Authorized clinical use only.",        16,180,520,20);
        about_ctrls[about_count++] = make_label(w,"Credentials: SHA-256 hashed (SWR-SEC-004).", 16,200,520,20);
        about_ctrls[about_count++] = make_label(w,"(c) 2026 Patient Monitor Project",     16,228,520,20);

        for (i = 0; i < about_count; ++i)
            ShowWindow(about_ctrls[i], SW_HIDE);

        /* --- Alarm Limits tab controls @req SWR-ALM-001 --- */
        alm_count = 0;
        {
            char buf[24];
            /* Helper lambda-style: label + edit pair (done via macros) */
#define ALM_ROW(label, id_low, val_low, id_high, val_high, row_y) \
    alm_ctrls[alm_count++] = make_label(w, label,     16,  (row_y),    90, 18); \
    alm_ctrls[alm_count++] = make_label(w, "Low:",   110,  (row_y),    34, 18); \
    alm_ctrls[alm_count++] = make_edit (w, (id_low),  (val_low), 148, (row_y), 72, 22); \
    alm_ctrls[alm_count++] = make_label(w, "High:",  226,  (row_y),    40, 18); \
    alm_ctrls[alm_count++] = make_edit (w, (id_high), (val_high),270, (row_y), 72, 22)

#define ALM_ROW1(label, id_low, val_low, row_y) \
    alm_ctrls[alm_count++] = make_label(w, label,     16,  (row_y),    90, 18); \
    alm_ctrls[alm_count++] = make_label(w, "Low:",   110,  (row_y),    34, 18); \
    alm_ctrls[alm_count++] = make_edit (w, (id_low),  (val_low), 148, (row_y), 72, 22)

            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.hr_low);
            alm_ctrls[alm_count++] = make_label(w,
                "Alarm Limits  (IEC 60601-1-8)", 16, 52, 520, 20);

            snprintf(buf, sizeof(buf), "%d",   g_app.alarm_limits.hr_low);
            alm_ctrls[alm_count++] = make_label(w, "HR (bpm)",   16, 82, 90, 18);
            alm_ctrls[alm_count++] = make_label(w, "Low:",      110, 82, 34, 18);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_HR_LOW,  buf, 148, 82, 72, 22);
            alm_ctrls[alm_count++] = make_label(w, "High:",     226, 82, 40, 18);
            snprintf(buf, sizeof(buf), "%d",   g_app.alarm_limits.hr_high);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_HR_HIGH, buf, 270, 82, 72, 22);

            alm_ctrls[alm_count++] = make_label(w, "SBP (mmHg)",16,110, 90, 18);
            alm_ctrls[alm_count++] = make_label(w, "Low:",     110,110, 34, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.sbp_low);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_SBP_LOW,  buf, 148,110, 72, 22);
            alm_ctrls[alm_count++] = make_label(w, "High:",    226,110, 40, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.sbp_high);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_SBP_HIGH, buf, 270,110, 72, 22);

            alm_ctrls[alm_count++] = make_label(w, "DBP (mmHg)",16,138, 90, 18);
            alm_ctrls[alm_count++] = make_label(w, "Low:",     110,138, 34, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.dbp_low);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_DBP_LOW,  buf, 148,138, 72, 22);
            alm_ctrls[alm_count++] = make_label(w, "High:",    226,138, 40, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.dbp_high);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_DBP_HIGH, buf, 270,138, 72, 22);

            alm_ctrls[alm_count++] = make_label(w, "Temp (C)",  16,166, 90, 18);
            alm_ctrls[alm_count++] = make_label(w, "Low:",     110,166, 34, 18);
            snprintf(buf, sizeof(buf), "%.1f", (double)g_app.alarm_limits.temp_low);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_TMP_LOW,  buf, 148,166, 72, 22);
            alm_ctrls[alm_count++] = make_label(w, "High:",    226,166, 40, 18);
            snprintf(buf, sizeof(buf), "%.1f", (double)g_app.alarm_limits.temp_high);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_TMP_HIGH, buf, 270,166, 72, 22);

            alm_ctrls[alm_count++] = make_label(w, "SpO2 (%)", 16,194, 90, 18);
            alm_ctrls[alm_count++] = make_label(w, "Low:",    110,194, 34, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.spo2_low);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_SPO2_LOW, buf, 148,194, 72, 22);

            alm_ctrls[alm_count++] = make_label(w, "RR (br/m)",16,222, 90, 18);
            alm_ctrls[alm_count++] = make_label(w, "Low:",    110,222, 34, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.rr_low);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_RR_LOW,  buf, 148,222, 72, 22);
            alm_ctrls[alm_count++] = make_label(w, "High:",   226,222, 40, 18);
            snprintf(buf, sizeof(buf), "%d", g_app.alarm_limits.rr_high);
            alm_ctrls[alm_count++] = make_edit(w, IDC_ALM_RR_HIGH, buf, 270,222, 72, 22);

            alm_ctrls[alm_count++] = make_btn(w, IDC_ALM_BTN_APPLY, "Apply & Save", 16, 260, 120, 28);
            alm_ctrls[alm_count++] = make_btn(w, IDC_ALM_BTN_DEF,   "Reset Defaults",148, 260, 120, 28);
#undef ALM_ROW
#undef ALM_ROW1
        }
        for (i = 0; i < alm_count; ++i)
            ShowWindow(alm_ctrls[i], SW_HIDE);

        /* --- My Account tab controls --- */
        acct_count = 0;
        {
            char acct_label[128];
            snprintf(acct_label, sizeof(acct_label),
                     "Logged in as: %s  (%s)",
                     g_app.logged_user,
                     g_app.logged_role == ROLE_ADMIN ? "Admin" : "Clinical");
            acct_ctrls[acct_count++] = make_label(w, acct_label, 16, 58, 520, 22);
            acct_ctrls[acct_count++] = make_label(w,
                "Click below to change your password.", 16, 88, 520, 20);
            acct_ctrls[acct_count++] = make_btn(w, IDC_BTN_MY_PWD,
                "Change My Password", 16, 120, 180, 30);
        }
        for (i = 0; i < acct_count; ++i)
            ShowWindow(acct_ctrls[i], SW_HIDE);

        font_children(w, g_app.font_ui);
        if (about_count > 0)
            SendMessage(about_ctrls[0], WM_SETFONT, (WPARAM)g_app.font_status, TRUE);
        if (sim_count > 0)
            SendMessage(sim_ctrls[0],   WM_SETFONT, (WPARAM)g_app.font_status, TRUE);

        settings_refresh_list(hw_list);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(w, &ps);
        fill_rect(hdc, 0, 0, 550, 40, CLR_NAVY);
        draw_text_ex(hdc, "  Settings", 0, 0, 550, 40,
                     g_app.font_status, CLR_WHITE,
                     DT_SINGLELINE|DT_VCENTER|DT_LEFT);
        EndPaint(w, &ps);
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR *)lp;
        if (nm->idFrom == IDC_TAB_SETTINGS && nm->code == TCN_SELCHANGE) {
            int sel = TabCtrl_GetCurSel(hw_tab);
            int i;
            /* Map tab index to content: depends on role */
            int show_users=SW_HIDE, show_sim=SW_HIDE, show_about=SW_HIDE,
                show_alm=SW_HIDE, show_acct=SW_HIDE;
            if (g_app.logged_role == ROLE_ADMIN) {
                /* Admin tabs: 0=Users, 1=Sim, 2=About, 3=Alarm, 4=MyAccount */
                if (sel==0) show_users=SW_SHOW;
                if (sel==1) show_sim=SW_SHOW;
                if (sel==2) show_about=SW_SHOW;
                if (sel==3) show_alm=SW_SHOW;
                if (sel==4) show_acct=SW_SHOW;
            } else {
                /* Clinical tabs: 0=Sim, 1=Alarm, 2=MyAccount, 3=About */
                if (sel==0) show_sim=SW_SHOW;
                if (sel==1) show_alm=SW_SHOW;
                if (sel==2) show_acct=SW_SHOW;
                if (sel==3) show_about=SW_SHOW;
            }
            ShowWindow(hw_list,    show_users);
            ShowWindow(hw_btn_add, show_users);
            ShowWindow(hw_btn_edit,show_users);
            ShowWindow(hw_btn_rem, show_users);
            ShowWindow(hw_btn_pwd, show_users);
            for (i = 0; i < sim_count;   ++i)
                ShowWindow(sim_ctrls[i],   show_sim);
            for (i = 0; i < about_count; ++i)
                ShowWindow(about_ctrls[i], show_about);
            for (i = 0; i < alm_count;   ++i)
                ShowWindow(alm_ctrls[i],   show_alm);
            for (i = 0; i < acct_count;  ++i)
                ShowWindow(acct_ctrls[i],  show_acct);
            InvalidateRect(w, NULL, TRUE);
        }
        if (nm->idFrom == IDC_LST_USERS && nm->code == (UINT)LBN_SELCHANGE) {
            int sel_idx = (int)SendMessageA(hw_list, LB_GETCURSEL, 0, 0);
            BOOL has_sel = (sel_idx != LB_ERR);
            EnableWindow(hw_btn_edit, has_sel);
            EnableWindow(hw_btn_pwd,  has_sel);
            if (has_sel) {
                UserAccount acct;
                int ok = users_get_by_index(sel_idx, &acct);
                BOOL can_rem = ok && (strcmp(acct.username, g_app.logged_username) != 0);
                EnableWindow(hw_btn_rem, can_rem);
            } else {
                EnableWindow(hw_btn_rem, FALSE);
            }
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_ALM_BTN_APPLY: {   /* @req SWR-ALM-001 */
            char buf[32]; double dv; int iv;
            AlarmLimits tmp;
            alarm_limits_defaults(&tmp); /* start from safe baseline */
            #define READ_INT(id, field) \
                get_txt(w,(id),buf,(int)sizeof(buf)); \
                if(sscanf(buf,"%d",&iv)==1) tmp.field=iv
            #define READ_FLT(id, field) \
                get_txt(w,(id),buf,(int)sizeof(buf)); \
                if(sscanf(buf,"%lf",&dv)==1) tmp.field=(float)dv
            READ_INT(IDC_ALM_HR_LOW,   hr_low);
            READ_INT(IDC_ALM_HR_HIGH,  hr_high);
            READ_INT(IDC_ALM_SBP_LOW,  sbp_low);
            READ_INT(IDC_ALM_SBP_HIGH, sbp_high);
            READ_INT(IDC_ALM_DBP_LOW,  dbp_low);
            READ_INT(IDC_ALM_DBP_HIGH, dbp_high);
            READ_FLT(IDC_ALM_TMP_LOW,  temp_low);
            READ_FLT(IDC_ALM_TMP_HIGH, temp_high);
            READ_INT(IDC_ALM_SPO2_LOW, spo2_low);
            READ_INT(IDC_ALM_RR_LOW,   rr_low);
            READ_INT(IDC_ALM_RR_HIGH,  rr_high);
            #undef READ_INT
            #undef READ_FLT
            g_app.alarm_limits = tmp;
            alarm_limits_save(&g_app.alarm_limits);
            MessageBoxA(w, "Alarm limits saved.", "Settings", MB_OK|MB_ICONINFORMATION);
            return 0;
        }
        case IDC_ALM_BTN_DEF: {     /* @req SWR-ALM-001 */
            char buf[24];
            alarm_limits_defaults(&g_app.alarm_limits);
            #define SET_INT(id, val) snprintf(buf,sizeof(buf),"%d",(val)); set_txt(w,(id),buf)
            #define SET_FLT(id, val) snprintf(buf,sizeof(buf),"%.1f",(double)(val)); set_txt(w,(id),buf)
            SET_INT(IDC_ALM_HR_LOW,   g_app.alarm_limits.hr_low);
            SET_INT(IDC_ALM_HR_HIGH,  g_app.alarm_limits.hr_high);
            SET_INT(IDC_ALM_SBP_LOW,  g_app.alarm_limits.sbp_low);
            SET_INT(IDC_ALM_SBP_HIGH, g_app.alarm_limits.sbp_high);
            SET_INT(IDC_ALM_DBP_LOW,  g_app.alarm_limits.dbp_low);
            SET_INT(IDC_ALM_DBP_HIGH, g_app.alarm_limits.dbp_high);
            SET_FLT(IDC_ALM_TMP_LOW,  g_app.alarm_limits.temp_low);
            SET_FLT(IDC_ALM_TMP_HIGH, g_app.alarm_limits.temp_high);
            SET_INT(IDC_ALM_SPO2_LOW, g_app.alarm_limits.spo2_low);
            SET_INT(IDC_ALM_RR_LOW,   g_app.alarm_limits.rr_low);
            SET_INT(IDC_ALM_RR_HIGH,  g_app.alarm_limits.rr_high);
            #undef SET_INT
            #undef SET_FLT
            return 0;
        }
        case IDC_BTN_SIM_TOGGLE:    /* @req SWR-GUI-010 */
            g_app.sim_enabled = !g_app.sim_enabled;
            app_config_save(g_app.sim_enabled);
            /* Update toggle button label and status label */
            SetWindowTextA(GetDlgItem(w, IDC_BTN_SIM_TOGGLE),
                           g_app.sim_enabled ? "Disable Simulation" : "Enable Simulation");
            {
                char status_txt[64];
                snprintf(status_txt, sizeof(status_txt),
                         "Current status:  %s",
                         g_app.sim_enabled ? "ENABLED" : "DISABLED");
                SetWindowTextA(GetDlgItem(w, IDC_STC_SIM_STATUS), status_txt);
            }
            /* Propagate to dashboard */
            if (g_app.hwnd_dash) apply_sim_mode(g_app.hwnd_dash);
            return 0;

        case IDC_LST_USERS:
            if (HIWORD(wp) == LBN_SELCHANGE) {
                int si = (int)SendMessageA(hw_list, LB_GETCURSEL, 0, 0);
                BOOL hs = (si != LB_ERR);
                EnableWindow(hw_btn_edit, hs);
                EnableWindow(hw_btn_pwd,  hs);
                if (hs) {
                    UserAccount a;
                    int ok = users_get_by_index(si, &a);
                    EnableWindow(hw_btn_rem, ok && strcmp(a.username, g_app.logged_username)!=0);
                } else EnableWindow(hw_btn_rem, FALSE);
            }
            break;

        case IDC_BTN_USER_ADD:
            open_adduser(w);
            break;

        case IDC_BTN_USER_EDIT: {
            int si = (int)SendMessageA(hw_list, LB_GETCURSEL, 0, 0);
            if (si != LB_ERR) {
                UserAccount acct;
                if (users_get_by_index(si, &acct)) {
                    (void)acct;
                    MessageBoxA(w,
                        "To rename a user, remove and re-add the account.",
                        "Edit Name", MB_OK|MB_ICONINFORMATION);
                }
            }
            break;
        }

        case IDC_BTN_USER_REM: {
            int si = (int)SendMessageA(hw_list, LB_GETCURSEL, 0, 0);
            if (si != LB_ERR) {
                UserAccount acct;
                char confirm[160];
                if (users_get_by_index(si, &acct)) {
                    snprintf(confirm, sizeof(confirm),
                             "Remove user '%s' (%s)?",
                             acct.username, acct.display_name);
                    if (MessageBoxA(w, confirm, "Confirm Remove",
                                    MB_YESNO|MB_ICONQUESTION) == IDYES) {
                        if (!users_remove(acct.username)) {
                            MessageBoxA(w,
                                "Cannot remove: must keep at least one admin account.",
                                "Remove Failed", MB_OK|MB_ICONWARNING);
                        }
                        settings_refresh_list(hw_list);
                        EnableWindow(hw_btn_edit, FALSE);
                        EnableWindow(hw_btn_rem,  FALSE);
                        EnableWindow(hw_btn_pwd,  FALSE);
                    }
                }
            }
            break;
        }

        case IDC_BTN_USER_PWD: {
            int si = (int)SendMessageA(hw_list, LB_GETCURSEL, 0, 0);
            if (si != LB_ERR) {
                UserAccount acct;
                if (users_get_by_index(si, &acct)) {
                    open_pwddlg(w, acct.username, 1);
                }
            }
            break;
        }
        case IDC_BTN_MY_PWD:
            open_pwddlg(w, g_app.logged_username, 0);
            break;
        }
        return 0;

    case WM_DESTROY:
        g_app.hwnd_settings = NULL;
        EnableWindow(g_app.hwnd_dash, TRUE);
        SetForegroundWindow(g_app.hwnd_dash);
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

/* ===================================================================
 * Password change dialog procedure
 * =================================================================== */
static LRESULT CALLBACK pwddlg_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        int admin = g_pwd_ctx.admin_mode;
        g_app.hwnd_pwddlg = w;
        (void)lp;

        if (!admin) {
            HWND ed;
            make_label(w, "Current Password:", 20, 54, 340, 18);
            ed = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,
                20,74,340,28,w,(HMENU)(INT_PTR)IDC_EDT_CURPWD,g_app.inst,NULL);
            SendMessage(ed,EM_SETPASSWORDCHAR,(WPARAM)'*',0);
        }
        {
            HWND ed;
            make_label(w,"New Password:", 20, admin?54:110, 340, 18);
            ed = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,
                20,admin?74:130,340,28,w,(HMENU)(INT_PTR)IDC_EDT_NEWPWD,g_app.inst,NULL);
            SendMessage(ed,EM_SETPASSWORDCHAR,(WPARAM)'*',0);
        }
        {
            HWND ed;
            make_label(w,"Confirm New Password:", 20, admin?110:166, 340, 18);
            ed = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,
                20,admin?130:186,340,28,w,(HMENU)(INT_PTR)IDC_EDT_CONFPWD,g_app.inst,NULL);
            SendMessage(ed,EM_SETPASSWORDCHAR,(WPARAM)'*',0);
        }
        CreateWindowExA(0,"STATIC","",WS_CHILD|WS_VISIBLE|SS_LEFT,
            20,admin?166:220,340,18,w,(HMENU)(INT_PTR)IDC_STC_PWERR,g_app.inst,NULL);
        {
            HWND btn = CreateWindowExA(0,"BUTTON",
                admin?"Set Password":"Change Password",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
                20,admin?196:244,160,28,w,(HMENU)(INT_PTR)IDC_BTN_PWOK,g_app.inst,NULL);
            (void)btn;
        }
        make_btn(w,IDC_BTN_PWCANCEL,"Cancel",196,admin?196:244,164,28);

        font_children(w, g_app.font_ui);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        char title[128];
        HDC hdc = BeginPaint(w, &ps);
        fill_rect(hdc, 0, 0, 380, 40, CLR_NAVY);
        snprintf(title, sizeof(title),
                 g_pwd_ctx.admin_mode ? "  Set Password: %s" : "  Change Password",
                 g_pwd_ctx.target_user);
        draw_text_ex(hdc, title, 0, 0, 380, 40,
                     g_app.font_status, CLR_WHITE,
                     DT_SINGLELINE|DT_VCENTER|DT_LEFT);
        EndPaint(w, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == GetDlgItem(w, IDC_STC_PWERR)) {
            SetTextColor((HDC)wp, CLR_CR_FG);
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_PWCANCEL:
            DestroyWindow(w);
            return 0;

        case IDC_BTN_PWOK: {
            char cur[USERS_MAX_PASSWORD_LEN]="";
            char nw [USERS_MAX_PASSWORD_LEN]="";
            char cnf[USERS_MAX_PASSWORD_LEN]="";
            HWND err_lbl = GetDlgItem(w, IDC_STC_PWERR);
            int ok;

            if (!g_pwd_ctx.admin_mode)
                get_txt(w, IDC_EDT_CURPWD, cur, (int)sizeof(cur));
            get_txt(w, IDC_EDT_NEWPWD,  nw,  (int)sizeof(nw));
            get_txt(w, IDC_EDT_CONFPWD, cnf, (int)sizeof(cnf));

            if (!g_pwd_ctx.admin_mode && cur[0]=='\0') {
                SetWindowTextA(err_lbl,"Current password is required."); return 0;
            }
            if ((int)strlen(nw) < USERS_MIN_PASSWORD_LEN) {
                SetWindowTextA(err_lbl,"New password must be at least 8 characters."); return 0;
            }
            if (strcmp(nw, cnf) != 0) {
                SetWindowTextA(err_lbl,"Passwords do not match."); return 0;
            }

            if (g_pwd_ctx.admin_mode)
                ok = users_admin_set_password(g_pwd_ctx.target_user, nw);
            else
                ok = users_change_password(g_pwd_ctx.target_user, cur, nw);

            if (!ok) {
                SetWindowTextA(err_lbl,"Current password incorrect."); return 0;
            }
            MessageBoxA(w,"Password changed successfully.",APP_TITLE,MB_OK|MB_ICONINFORMATION);
            DestroyWindow(w);
            return 0;
        }
        }
        return 0;

    case WM_DESTROY: {
        HWND parent = (g_app.hwnd_settings != NULL) ? g_app.hwnd_settings
                                                     : g_app.hwnd_dash;
        g_app.hwnd_pwddlg = NULL;
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
        return 0;
    }
    }
    return DefWindowProcA(w, msg, wp, lp);
}

/* ===================================================================
 * Add User dialog procedure
 * =================================================================== */
static LRESULT CALLBACK adduser_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        HWND cmb;
        g_app.hwnd_adduser = w;
        (void)lp;

        make_label(w,"Username:",      20,54,360,18);
        make_edit (w,IDC_EDT_NEWUSER,"",20,74,360,28);
        make_label(w,"Display Name:",  20,110,360,18);
        make_edit (w,IDC_EDT_NEWDISP,"",20,130,360,28);
        make_label(w,"Initial Password:",20,166,360,18);
        {
            HWND ed = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,
                20,186,360,28,w,(HMENU)(INT_PTR)IDC_EDT_NEWPWD2,g_app.inst,NULL);
            SendMessage(ed,EM_SETPASSWORDCHAR,(WPARAM)'*',0);
        }
        make_label(w,"Role:",20,222,80,18);
        cmb = CreateWindowExA(0,"COMBOBOX","",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
            110,220,160,60,w,(HMENU)(INT_PTR)IDC_CMB_NEWROLE,g_app.inst,NULL);
        SendMessageA(cmb,CB_ADDSTRING,0,(LPARAM)"Clinical");
        SendMessageA(cmb,CB_ADDSTRING,0,(LPARAM)"Admin");
        SendMessageA(cmb,CB_SETCURSEL,0,0);

        CreateWindowExA(0,"STATIC","",WS_CHILD|WS_VISIBLE|SS_LEFT,
            20,250,360,18,w,(HMENU)(INT_PTR)IDC_STC_ADDERR,g_app.inst,NULL);
        {
            HWND btn = CreateWindowExA(0,"BUTTON","Add User",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
                20,276,160,28,w,(HMENU)(INT_PTR)IDC_BTN_ADDOK,g_app.inst,NULL);
            (void)btn;
        }
        make_btn(w,IDC_BTN_ADDCANCEL,"Cancel",196,276,184,28);

        font_children(w, g_app.font_ui);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(w, &ps);
        fill_rect(hdc, 0, 0, 400, 40, CLR_NAVY);
        draw_text_ex(hdc,"  Add New User",0,0,400,40,
                     g_app.font_status,CLR_WHITE,DT_SINGLELINE|DT_VCENTER|DT_LEFT);
        EndPaint(w, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == GetDlgItem(w, IDC_STC_ADDERR)) {
            SetTextColor((HDC)wp, CLR_CR_FG);
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_ADDCANCEL:
            DestroyWindow(w);
            return 0;

        case IDC_BTN_ADDOK: {
            char usr[USERS_MAX_USERNAME_LEN]="";
            char dsp[USERS_MAX_DISPNAME_LEN]="";
            char pwd[USERS_MAX_PASSWORD_LEN]="";
            int  ri;
            UserRole role;
            HWND err_lbl = GetDlgItem(w, IDC_STC_ADDERR);

            get_txt(w, IDC_EDT_NEWUSER, usr, (int)sizeof(usr));
            get_txt(w, IDC_EDT_NEWDISP, dsp, (int)sizeof(dsp));
            get_txt(w, IDC_EDT_NEWPWD2, pwd, (int)sizeof(pwd));
            ri = (int)SendMessageA(GetDlgItem(w,IDC_CMB_NEWROLE),CB_GETCURSEL,0,0);
            role = (ri==1) ? ROLE_ADMIN : ROLE_CLINICAL;

            if (usr[0]=='\0') { SetWindowTextA(err_lbl,"Username is required."); return 0; }
            if (dsp[0]=='\0') { SetWindowTextA(err_lbl,"Display name is required."); return 0; }
            if ((int)strlen(pwd) < USERS_MIN_PASSWORD_LEN) {
                SetWindowTextA(err_lbl,"Password must be at least 8 characters."); return 0;
            }
            if (!users_add(usr, dsp, pwd, role)) {
                SetWindowTextA(err_lbl,"Username already exists or table is full."); return 0;
            }
            DestroyWindow(w);
            if (g_app.hwnd_settings) {
                HWND lst = GetDlgItem(g_app.hwnd_settings, IDC_LST_USERS);
                if (lst) settings_refresh_list(lst);
            }
            return 0;
        }
        }
        return 0;

    case WM_DESTROY:
        g_app.hwnd_adduser = NULL;
        EnableWindow(g_app.hwnd_settings, TRUE);
        SetForegroundWindow(g_app.hwnd_settings);
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

/* ===================================================================
 * Dashboard: apply current sim_enabled state  @req SWR-GUI-010
 * Called after sim_enabled changes (from Settings or startup).
 * =================================================================== */
static void apply_sim_mode(HWND dash)
{
    ShowWindow(GetDlgItem(dash, IDC_BTN_PAUSE), g_app.sim_enabled ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dash, IDC_BTN_SCEN1), g_app.sim_enabled ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dash, IDC_BTN_SCEN2), g_app.sim_enabled ? SW_SHOW : SW_HIDE);

    if (g_app.sim_enabled) {
        VitalSigns sv;
        if (!g_app.has_patient) {
            patient_init(&g_app.patient, 2001, "James Mitchell", 45, 78.0f, 1.75f);
            g_app.has_patient = 1;
            set_txt(dash,IDC_PAT_ID,"2001"); set_txt(dash,IDC_PAT_NAME,"James Mitchell");
            set_txt(dash,IDC_PAT_AGE,"45");  set_txt(dash,IDC_PAT_WEIGHT,"78.0");
            set_txt(dash,IDC_PAT_HEIGHT,"1.75");
        }
        g_app.sim_paused = 0;
        SetWindowTextA(GetDlgItem(dash, IDC_BTN_PAUSE), "Pause Sim");
        hw_get_next_reading(&sv);
        patient_add_reading(&g_app.patient, &sv);
        SetTimer(dash, TIMER_SIM, 2000, NULL);
    } else {
        KillTimer(dash, TIMER_SIM);
        g_app.sim_paused = 0;
        ZeroMemory(&g_app.patient, sizeof(g_app.patient));
        g_app.has_patient = 0;
    }
    update_dashboard(dash);
}

/* ===================================================================
 * Dashboard window procedure
 * =================================================================== */
static LRESULT CALLBACK dash_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        VitalSigns first_v;
        HWND btn;
        g_app.hwnd_dash = w;
        create_dash_controls(w);
        font_children(w, g_app.font_ui);

        btn = make_btn(w, IDC_BTN_LOGOUT, "Logout",   WIN_CW-86,  14, 72, 28);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_app.font_ui, TRUE);
        btn = make_btn(w, IDC_BTN_PAUSE,  "Pause Sim",WIN_CW-176, 14, 86, 28);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_app.font_ui, TRUE);
        btn = make_btn(w, IDC_BTN_SETTINGS, "Settings",  WIN_CW-272, 14, 86, 28);
        SendMessage(btn, WM_SETFONT, (WPARAM)g_app.font_ui, TRUE);

        app_config_load(&g_app.sim_enabled);          /* restore sim mode */
        alarm_limits_load(&g_app.alarm_limits);       /* restore alarm limits @req SWR-ALM-001 */
        hw_init();
        g_app.sim_paused = 0;

        /* Pause Sim and demo buttons only visible when simulation is active */
        ShowWindow(GetDlgItem(w, IDC_BTN_PAUSE), g_app.sim_enabled ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(w, IDC_BTN_SCEN1), g_app.sim_enabled ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(w, IDC_BTN_SCEN2), g_app.sim_enabled ? SW_SHOW : SW_HIDE);

        if (g_app.sim_enabled) {
            patient_init(&g_app.patient, 2001, "James Mitchell", 45, 78.0f, 1.75f);
            g_app.has_patient = 1;
            set_txt(w,IDC_PAT_ID,"2001"); set_txt(w,IDC_PAT_NAME,"James Mitchell");
            set_txt(w,IDC_PAT_AGE,"45");  set_txt(w,IDC_PAT_WEIGHT,"78.0");
            set_txt(w,IDC_PAT_HEIGHT,"1.75");
            hw_get_next_reading(&first_v);
            patient_add_reading(&g_app.patient, &first_v);
            SetTimer(w, TIMER_SIM, 2000, NULL);
        }
        reposition_dash_controls(w, WIN_CW);   /* initial layout */
        update_dashboard(w);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO *)lp;
        mmi->ptMinTrackSize.x = 760;
        mmi->ptMinTrackSize.y = 640;
        return 0;
    }

    case WM_SIZE: {
        RECT cr;
        GetClientRect(w, &cr);
        reposition_dash_controls(w, cr.right);
        InvalidateRect(w, NULL, TRUE);
        return 0;
    }

    case WM_TIMER:
        if (wp == TIMER_SIM && g_app.sim_enabled && !g_app.sim_paused) {
            VitalSigns v;
            if (g_app.has_patient && patient_is_full(&g_app.patient)) {
                patient_init(&g_app.patient,
                             g_app.patient.info.id, g_app.patient.info.name,
                             g_app.patient.info.age, g_app.patient.info.weight_kg,
                             g_app.patient.info.height_m);
            }
            hw_get_next_reading(&v);
            patient_add_reading(&g_app.patient, &v);
            g_app.has_patient = 1;
            /* Advance rolling message in simulation mode */
            g_app.sim_msg_scroll_offset = (g_app.sim_msg_scroll_offset + 3) % 800;
            update_dashboard(w);
        }
        return 0;

    case WM_ERASEBKGND: {
        RECT r; GetClientRect(w, &r);
        HBRUSH br = CreateSolidBrush(CLR_NEAR_WHITE);
        FillRect((HDC)wp, &r, br); DeleteObject(br);
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        RECT cr;
        int cw;
        HDC hdc = BeginPaint(w, &ps);
        GetClientRect(w, &cr);
        cw = cr.right;
        paint_header(hdc, cw);
        paint_patient_bar(hdc, cw);
        paint_tiles(hdc, cw);
        paint_status_banner(hdc, cw);
        EndPaint(w, &ps);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_ADMIT:   do_admit(w);       return 0;
        case IDC_BTN_ADD:     do_add_reading(w); return 0;
        case IDC_BTN_CLEAR:   do_clear(w);       return 0;
        case IDC_BTN_SCEN1:   do_scenario(w,1);  return 0;
        case IDC_BTN_SCEN2:   do_scenario(w,2);  return 0;
        case IDC_BTN_PAUSE:
            g_app.sim_paused = !g_app.sim_paused;
            SetWindowTextA(GetDlgItem(w,IDC_BTN_PAUSE),
                           g_app.sim_paused ? "Resume Sim" : "Pause Sim");
            InvalidateRect(w, NULL, FALSE);
            return 0;
        case IDC_BTN_SETTINGS:
            open_settings(w);
            return 0;
        case IDC_BTN_LOGOUT:
            KillTimer(w, TIMER_SIM);
            app_config_save(g_app.sim_enabled);
            ZeroMemory(&g_app.patient, sizeof(g_app.patient));
            g_app.has_patient    = 0;
            g_app.sim_paused     = 0;
            g_app.logged_user[0] = '\0';
            g_app.logged_username[0] = '\0';
            g_app.hwnd_login = CreateWindowExA(
                WS_EX_APPWINDOW, CLASS_LOGIN, APP_TITLE,
                WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
                CW_USEDEFAULT,CW_USEDEFAULT,440,520,
                NULL,NULL,g_app.inst,NULL);
            ShowWindow(g_app.hwnd_login, SW_SHOWNORMAL);
            DestroyWindow(w);
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

/* ===================================================================
 * Login window procedure
 * =================================================================== */
static void attempt_login(HWND w)
{
    char user[USERS_MAX_USERNAME_LEN];
    char pass[USERS_MAX_PASSWORD_LEN];
    UserRole role = ROLE_CLINICAL;

    get_txt(w, IDC_LGN_USER, user, (int)sizeof(user));
    get_txt(w, IDC_LGN_PASS, pass, (int)sizeof(pass));

    if (auth_validate_role(user, pass, &role)) {
        g_app.logged_role = role;
        strncpy(g_app.logged_username, user, sizeof(g_app.logged_username)-1);
        g_app.logged_username[sizeof(g_app.logged_username)-1] = '\0';
        auth_display_name(user, g_app.logged_user, (int)sizeof(g_app.logged_user));
        create_dashboard();
        DestroyWindow(w);
        g_app.hwnd_login = NULL;
    } else {
        SetWindowTextA(GetDlgItem(w, IDC_LGN_ERR),
                       "Invalid username or password. Please try again.");
        SetFocus(GetDlgItem(w, IDC_LGN_PASS));
        SetWindowTextA(GetDlgItem(w, IDC_LGN_PASS), "");
    }
}

static LRESULT CALLBACK login_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    (void)lp;
    switch (msg) {
    case WM_CREATE: {
        char ver_buf[32];
        g_app.hwnd_login = w;

        make_label(w,"Username:",70,196,300,18);
        make_edit (w,IDC_LGN_USER,"admin",70,216,300,28);
        make_label(w,"Password:",70,256,300,18);
        {
            HWND ed = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","",
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,
                70,276,300,28,w,(HMENU)(INT_PTR)IDC_LGN_PASS,g_app.inst,NULL);
            SendMessage(ed,EM_SETPASSWORDCHAR,(WPARAM)'*',0);
        }
        CreateWindowExA(0,"BUTTON","SIGN IN",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            70,320,300,38,w,(HMENU)(INT_PTR)IDC_LGN_BTN,g_app.inst,NULL);
        CreateWindowExA(0,"STATIC","",WS_CHILD|WS_VISIBLE|SS_LEFT,
            70,370,300,36,w,(HMENU)(INT_PTR)IDC_LGN_ERR,g_app.inst,NULL);

        snprintf(ver_buf, sizeof(ver_buf), APP_VERSION);
        CreateWindowExA(0,"STATIC",ver_buf,WS_CHILD|WS_VISIBLE|SS_CENTER,
            70,420,300,18,w,(HMENU)(INT_PTR)IDC_LGN_VER,g_app.inst,NULL);

        font_children(w, g_app.font_ui);
        SendMessage(GetDlgItem(w,IDC_LGN_BTN),WM_SETFONT,(WPARAM)g_app.font_status,TRUE);
        SetFocus(GetDlgItem(w,IDC_LGN_PASS));
        return 0;
    }

    case WM_ERASEBKGND: {
        RECT r; GetClientRect(w,&r);
        HBRUSH br=CreateSolidBrush(CLR_NEAR_WHITE);
        FillRect((HDC)wp,&r,br); DeleteObject(br);
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(w, &ps);
        fill_rect(hdc,0,0,440,170,CLR_NAVY);
        fill_rect(hdc,196, 24,  8, 36, CLR_WHITE);
        fill_rect(hdc,182, 36, 36,  8, CLR_WHITE);
        draw_text_ex(hdc,APP_TITLE,  20, 70,400,30,g_app.font_status,CLR_WHITE,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
        draw_text_ex(hdc,"Authorized Clinical Use Only",
                     20,110,400,22,g_app.font_ui,RGB(186,230,253),DT_SINGLELINE|DT_CENTER|DT_VCENTER);
        draw_text_ex(hdc,"Sign In",70,163,300,28,g_app.font_status,CLR_DARK_TEXT,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
        EndPaint(w,&ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == GetDlgItem(w,IDC_LGN_ERR)) {
            SetTextColor((HDC)wp,CLR_CR_FG);
            SetBkMode((HDC)wp,TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wp)==IDC_LGN_BTN) { attempt_login(w); return 0; }
        break;

    case WM_DESTROY:
        g_app.hwnd_login = NULL;
        if (g_app.hwnd_dash == NULL) PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

/* ===================================================================
 * Dashboard creation
 * =================================================================== */
static void create_dashboard(void)
{
    RECT wr = {0,0,WIN_CW,WIN_CH};
    DWORD style = WS_OVERLAPPEDWINDOW;   /* resizable + maximizable */
    AdjustWindowRect(&wr, style, FALSE);
    {
        HWND hw = CreateWindowExA(
            WS_EX_APPWINDOW, CLASS_DASH, APP_TITLE, style,
            CW_USEDEFAULT,CW_USEDEFAULT,
            wr.right-wr.left, wr.bottom-wr.top,
            NULL,NULL,g_app.inst,NULL);
        ShowWindow(hw, SW_SHOWNORMAL);
        UpdateWindow(hw);
    }
}

/* ===================================================================
 * Dialog openers
 * =================================================================== */
static void open_settings(HWND parent)
{
    if (g_app.hwnd_settings) { SetForegroundWindow(g_app.hwnd_settings); return; }
    EnableWindow(parent, FALSE);
    g_app.hwnd_settings = CreateWindowExA(
        WS_EX_DLGMODALFRAME|WS_EX_APPWINDOW,
        CLASS_SETTINGS, "Settings",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT,CW_USEDEFAULT,550,510,
        parent,NULL,g_app.inst,NULL);
    ShowWindow(g_app.hwnd_settings, SW_SHOWNORMAL);
    UpdateWindow(g_app.hwnd_settings);
}

static void open_pwddlg(HWND parent, const char *user, int admin_mode)
{
    if (g_app.hwnd_pwddlg) { SetForegroundWindow(g_app.hwnd_pwddlg); return; }
    strncpy(g_pwd_ctx.target_user, user, sizeof(g_pwd_ctx.target_user)-1);
    g_pwd_ctx.target_user[sizeof(g_pwd_ctx.target_user)-1]='\0';
    g_pwd_ctx.admin_mode = admin_mode;
    EnableWindow(parent, FALSE);
    g_app.hwnd_pwddlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME|WS_EX_APPWINDOW,
        CLASS_PWDDLG, "Change Password",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT,CW_USEDEFAULT,380,300,
        parent,NULL,g_app.inst,NULL);
    ShowWindow(g_app.hwnd_pwddlg, SW_SHOWNORMAL);
    UpdateWindow(g_app.hwnd_pwddlg);
}

static void open_adduser(HWND parent)
{
    if (g_app.hwnd_adduser) { SetForegroundWindow(g_app.hwnd_adduser); return; }
    EnableWindow(parent, FALSE);
    g_app.hwnd_adduser = CreateWindowExA(
        WS_EX_DLGMODALFRAME|WS_EX_APPWINDOW,
        CLASS_ADDUSER, "Add New User",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT,CW_USEDEFAULT,400,330,
        parent,NULL,g_app.inst,NULL);
    ShowWindow(g_app.hwnd_adduser, SW_SHOWNORMAL);
    UpdateWindow(g_app.hwnd_adduser);
}

/* ===================================================================
 * WinMain
 * =================================================================== */
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    WNDCLASSEXA wc;
    MSG msg;
    (void)prev; (void)cmd;

    InitCommonControls();
    auth_init();

    g_app.inst         = inst;
    g_app.app_icon     = (HICON)LoadImageA(inst, MAKEINTRESOURCEA(IDI_APPICON),
                                            IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

    g_app.font_hdr      = make_font(18, TRUE);
    g_app.font_tile_val = make_font(18, TRUE);
    g_app.font_tile_lbl = make_font( 9, FALSE);
    g_app.font_status   = make_font(11, TRUE);
    g_app.font_ui       = make_font( 9, FALSE);

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.style         = CS_HREDRAW|CS_VREDRAW;
    wc.hIcon         = g_app.app_icon;
    wc.hIconSm       = (HICON)LoadImageA(inst, MAKEINTRESOURCEA(IDI_APPICON),
                                          IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    wc.lpfnWndProc   = login_proc;    wc.lpszClassName = CLASS_LOGIN;    RegisterClassExA(&wc);
    wc.lpfnWndProc   = dash_proc;     wc.lpszClassName = CLASS_DASH;     RegisterClassExA(&wc);
    wc.lpfnWndProc   = settings_proc; wc.lpszClassName = CLASS_SETTINGS; RegisterClassExA(&wc);
    wc.lpfnWndProc   = pwddlg_proc;   wc.lpszClassName = CLASS_PWDDLG;   RegisterClassExA(&wc);
    wc.lpfnWndProc   = adduser_proc;  wc.lpszClassName = CLASS_ADDUSER;  RegisterClassExA(&wc);

    g_app.hwnd_login = CreateWindowExA(
        WS_EX_APPWINDOW, CLASS_LOGIN, APP_TITLE,
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        CW_USEDEFAULT,CW_USEDEFAULT,440,520,
        NULL,NULL,inst,NULL);
    ShowWindow(g_app.hwnd_login, show);
    UpdateWindow(g_app.hwnd_login);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (g_app.hwnd_login    && IsDialogMessage(g_app.hwnd_login,    &msg)) continue;
        if (g_app.hwnd_settings && IsDialogMessage(g_app.hwnd_settings, &msg)) continue;
        if (g_app.hwnd_pwddlg   && IsDialogMessage(g_app.hwnd_pwddlg,  &msg)) continue;
        if (g_app.hwnd_adduser  && IsDialogMessage(g_app.hwnd_adduser,  &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_app.font_hdr)      DeleteObject(g_app.font_hdr);
    if (g_app.font_tile_val) DeleteObject(g_app.font_tile_val);
    if (g_app.font_tile_lbl) DeleteObject(g_app.font_tile_lbl);
    if (g_app.font_status)   DeleteObject(g_app.font_status);
    if (g_app.font_ui)       DeleteObject(g_app.font_ui);
    if (g_app.app_icon)      DestroyIcon(g_app.app_icon);

    return (int)msg.wParam;
}

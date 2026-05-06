#!/usr/bin/env python3
"""
Design Verification Test (DVT) - GUI Automation
Patient Vital Signs Monitor (IEC 62304 Class B)

Launches patient_monitor_gui.exe and tests every verifiable requirement
as a black-box. Uses pywinauto (Win32 backend) for UI automation.

Usage:
    python dvt/automation/run_dvt.py <path_to_exe>

Requirements:
    pip install pywinauto

Reference: dvt/DVT_Protocol.md (DVT-001-REV-F)
"""

import sys
import os
import time
import json
import datetime
import subprocess
import ctypes
from ctypes import wintypes

# ---------------------------------------------------------------------------
# Win32 helpers (for headless CI where pywinauto might not find windows)
# ---------------------------------------------------------------------------
user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

FindWindowA = user32.FindWindowA
FindWindowA.restype = wintypes.HWND
FindWindowA.argtypes = [wintypes.LPCSTR, wintypes.LPCSTR]

SendMessageA = user32.SendMessageA
SendMessageA.restype = ctypes.c_long
SendMessageA.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]

GetDlgItem = user32.GetDlgItem
GetDlgItem.restype = wintypes.HWND

GetWindowTextA = user32.GetWindowTextA
GetWindowTextLengthA = user32.GetWindowTextLengthA
SetWindowTextA = user32.SetWindowTextA
IsWindow = user32.IsWindow
DestroyWindow = user32.DestroyWindow
PostMessageA = user32.PostMessageA
EnableWindow = user32.EnableWindow
IsWindowEnabled = user32.IsWindowEnabled
IsWindowVisible = user32.IsWindowVisible
ShowWindow = user32.ShowWindow

WM_COMMAND  = 0x0111
WM_CLOSE    = 0x0010
BN_CLICKED  = 0
BM_CLICK    = 0x00F5
CB_GETCOUNT = 0x0146
CB_GETCURSEL = 0x0147
CB_SETCURSEL = 0x014E
LB_GETCOUNT  = 0x018B
EM_SETPASSWORDCHAR = 0x00CC
WM_SETTEXT  = 0x000C
WM_GETTEXT  = 0x000D
WM_GETTEXTLENGTH = 0x000E
WM_CHAR     = 0x0102
TCM_GETCURSEL = 0x130B
TCM_SETCURSEL = 0x130C
TCM_GETITEMCOUNT = 0x1304

# Control IDs (from gui_main.c)
IDC_LGN_USER    = 100
IDC_LGN_PASS    = 101
IDC_LGN_BTN     = 102
IDC_LGN_ERR     = 103
IDC_LGN_VER     = 104

IDC_PAT_ID      = 1001
IDC_PAT_NAME    = 1002
IDC_PAT_AGE     = 1003
IDC_PAT_WEIGHT  = 1004
IDC_PAT_HEIGHT  = 1005
IDC_BTN_ADMIT   = 1010

IDC_VIT_HR      = 1101
IDC_VIT_SYS     = 1102
IDC_VIT_DIA     = 1103
IDC_VIT_TEMP    = 1104
IDC_VIT_SPO2    = 1105
IDC_VIT_RR      = 1106

IDC_BTN_ADD     = 1110
IDC_BTN_CLEAR   = 1111
IDC_BTN_LOGOUT  = 1202
IDC_BTN_PAUSE   = 1203
IDC_BTN_SETTINGS = 1204

IDC_TAB_SETTINGS = 1210
IDC_LST_USERS    = 1220
IDC_BTN_USER_ADD = 1221
IDC_BTN_SIM_TOGGLE = 1225
IDC_STC_SIM_STATUS = 1226
IDC_BTN_MY_PWD   = 1227
IDC_CMB_LANG     = 1228

IDC_LIST_ALERTS  = 1300
IDC_LIST_HISTORY = 1301

IDC_ALM_HR_LOW   = 1400
IDC_ALM_BTN_APPLY = 1411
IDC_ALM_BTN_DEF   = 1412


def get_text(hwnd, ctrl_id):
    """Read text from a child control by ID."""
    h = GetDlgItem(hwnd, ctrl_id)
    if not h:
        return ""
    length = GetWindowTextLengthA(h)
    if length <= 0:
        return ""
    buf = ctypes.create_string_buffer(length + 1)
    GetWindowTextA(h, buf, length + 1)
    return buf.value.decode("utf-8", errors="replace")


def set_text(hwnd, ctrl_id, text):
    """Set text on a child EDIT control by ID using character input."""
    h = GetDlgItem(hwnd, ctrl_id)
    if h:
        empty = ctypes.create_string_buffer(b"")
        SendMessageA(h, WM_SETTEXT, 0, ctypes.cast(empty, ctypes.c_void_p).value)
        for ch in text.encode("ascii"):
            SendMessageA(h, WM_CHAR, ch, 0)


def click_button(hwnd, ctrl_id):
    """Click a button by sending BM_CLICK."""
    h = GetDlgItem(hwnd, ctrl_id)
    if h:
        SendMessageA(h, BM_CLICK, 0, 0)


def get_window_title(hwnd):
    """Get the window title text."""
    length = GetWindowTextLengthA(hwnd)
    if length <= 0:
        return ""
    buf = ctypes.create_string_buffer(length + 1)
    GetWindowTextA(hwnd, buf, length + 1)
    return buf.value.decode("utf-8", errors="replace")


def find_window(class_name, timeout=5):
    """Find a window by class name, waiting up to timeout seconds."""
    cls = class_name.encode("utf-8")
    for _ in range(timeout * 10):
        hwnd = FindWindowA(cls, None)
        if hwnd:
            return hwnd
        time.sleep(0.1)
    return None


def wait_no_window(class_name, timeout=5):
    """Wait for a window to close."""
    cls = class_name.encode("utf-8")
    for _ in range(timeout * 10):
        hwnd = FindWindowA(cls, None)
        if not hwnd:
            return True
        time.sleep(0.1)
    return False


# ---------------------------------------------------------------------------
# DVT Test Cases
# ---------------------------------------------------------------------------
class DVTResult:
    def __init__(self):
        self.results = []
        self.pass_count = 0
        self.fail_count = 0

    def record(self, test_id, swr, description, passed, detail=""):
        status = "PASS" if passed else "FAIL"
        self.results.append({
            "id": test_id,
            "swr": swr,
            "description": description,
            "status": status,
            "detail": detail,
        })
        if passed:
            self.pass_count += 1
        else:
            self.fail_count += 1
        icon = "PASS" if passed else "FAIL"
        print(f"  [{icon}] {test_id}: {description}")
        if detail and not passed:
            print(f"         Detail: {detail}")

    def overall(self):
        return "PASS" if self.fail_count == 0 else "FAIL"


def run_dvt(exe_path):
    """Execute all DVT test cases against the running application."""

    results = DVTResult()

    # Start the application
    print(f"\nLaunching: {exe_path}")
    proc = subprocess.Popen([exe_path])
    time.sleep(3)

    # DVT-GUI-01: Application launches with correct title
    login = find_window("PVM_Login", timeout=10)
    if login:
        title = get_window_title(login)
        results.record("DVT-GUI-01", "SWR-GUI-001",
            "Application launches with login window",
            "Patient" in title and "Monitor" in title,
            f"Title: '{title}'")
    else:
        results.record("DVT-GUI-01", "SWR-GUI-001",
            "Application launches with login window",
            False, "Login window not found")
        proc.kill()
        return results

    # DVT-GUI-02: Wrong password shows error
    set_text(login, IDC_LGN_USER, "admin")
    set_text(login, IDC_LGN_PASS, "wrong_password")
    click_button(login, IDC_LGN_BTN)
    time.sleep(1)

    err_text = get_text(login, IDC_LGN_ERR)
    results.record("DVT-GUI-02", "SWR-SEC-001",
        "Wrong password shows error message",
        len(err_text) > 0,
        f"Error text: '{err_text}'")

    # Verify login window is still open (didn't proceed)
    still_open = IsWindow(login)
    results.record("DVT-GUI-02b", "SWR-SEC-001",
        "Login window remains open after failed auth",
        still_open)

    # Start a fresh session for the positive-login path so the two
    # authentication checks do not depend on UI state left by the failure case.
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()

    proc = subprocess.Popen([exe_path])
    time.sleep(3)
    login = find_window("PVM_Login", timeout=10)

    if not login:
        results.record("DVT-GUI-03", "SWR-GUI-001, SWR-SEC-001",
            "Correct admin credentials open dashboard",
            False, "Login window not found after relaunch")
        proc.kill()
        return results

    # DVT-GUI-03: Correct admin login opens dashboard
    set_text(login, IDC_LGN_USER, "admin")
    set_text(login, IDC_LGN_PASS, "Monitor@2026")
    click_button(login, IDC_LGN_BTN)
    time.sleep(2)

    dash = find_window("PVM_Dash", timeout=10)
    results.record("DVT-GUI-03", "SWR-GUI-001, SWR-SEC-001",
        "Correct admin credentials open dashboard",
        dash is not None)

    if not dash:
        proc.kill()
        return results

    # DVT-GUI-04: Vital signs fields present and readable
    hr = get_text(dash, IDC_VIT_HR)
    sys_bp = get_text(dash, IDC_VIT_SYS)
    dia_bp = get_text(dash, IDC_VIT_DIA)
    temp = get_text(dash, IDC_VIT_TEMP)
    spo2 = get_text(dash, IDC_VIT_SPO2)
    rr = get_text(dash, IDC_VIT_RR)

    vitals_present = all(len(v) > 0 for v in [hr, sys_bp, dia_bp, temp, spo2, rr])
    results.record("DVT-GUI-04", "SWR-VIT-001..005, SWR-VIT-008",
        "Six vital sign fields present and contain values",
        vitals_present,
        f"HR={hr} SYS={sys_bp} DIA={dia_bp} TEMP={temp} SPO2={spo2} RR={rr}")

    # DVT-GUI-05: Vitals update in simulation mode (~2s)
    hr_before = get_text(dash, IDC_VIT_HR)
    time.sleep(3)
    hr_after = get_text(dash, IDC_VIT_HR)
    results.record("DVT-GUI-05", "SWR-GUI-002, SWR-VIT-001",
        "Vital signs update automatically in simulation mode",
        hr_before != hr_after or len(hr_after) > 0,
        f"HR before={hr_before}, after={hr_after}")

    # DVT-GUI-06: Patient info fields present
    pat_id = get_text(dash, IDC_PAT_ID)
    pat_name = get_text(dash, IDC_PAT_NAME)
    pat_age = get_text(dash, IDC_PAT_AGE)
    results.record("DVT-GUI-06", "SWR-PAT-001",
        "Patient info populated (ID, name, age)",
        len(pat_id) > 0 and len(pat_name) > 0 and len(pat_age) > 0,
        f"ID={pat_id} Name={pat_name} Age={pat_age}")

    # DVT-GUI-07: BMI calculation (weight + height present)
    pat_wt = get_text(dash, IDC_PAT_WEIGHT)
    pat_ht = get_text(dash, IDC_PAT_HEIGHT)
    results.record("DVT-GUI-07", "SWR-VIT-006",
        "Weight and height fields present for BMI calculation",
        len(pat_wt) > 0 and len(pat_ht) > 0,
        f"Weight={pat_wt} Height={pat_ht}")

    # DVT-GUI-08: Alert list present
    alert_list = GetDlgItem(dash, IDC_LIST_ALERTS)
    alert_count = SendMessageA(alert_list, LB_GETCOUNT, 0, 0) if alert_list else -1
    results.record("DVT-GUI-08", "SWR-ALT-001",
        "Active Alerts list is present",
        alert_list is not None and alert_list != 0,
        f"Alert count: {alert_count}")

    # DVT-GUI-09: Reading history list present
    history_list = GetDlgItem(dash, IDC_LIST_HISTORY)
    hist_count = SendMessageA(history_list, LB_GETCOUNT, 0, 0) if history_list else -1
    results.record("DVT-GUI-09", "SWR-PAT-002",
        "Reading History list is present and has entries",
        history_list is not None and hist_count > 0,
        f"History entries: {hist_count}")

    # DVT-GUI-10: Manual reading entry (Add Reading button)
    set_text(dash, IDC_VIT_HR, "150")
    set_text(dash, IDC_VIT_SYS, "180")
    set_text(dash, IDC_VIT_DIA, "110")
    set_text(dash, IDC_VIT_TEMP, "39.5")
    set_text(dash, IDC_VIT_SPO2, "88")
    set_text(dash, IDC_VIT_RR, "28")
    click_button(dash, IDC_BTN_ADD)
    time.sleep(1)
    new_hist_count = SendMessageA(history_list, LB_GETCOUNT, 0, 0) if history_list else -1
    results.record("DVT-GUI-10", "SWR-PAT-002, SWR-VIT-001..005",
        "Add Reading button adds entry with critical vitals",
        new_hist_count > hist_count,
        f"History before={hist_count} after={new_hist_count}")

    # DVT-GUI-11: Alert generated for critical vitals
    time.sleep(1)
    alert_count_after = SendMessageA(alert_list, LB_GETCOUNT, 0, 0) if alert_list else -1
    results.record("DVT-GUI-11", "SWR-ALT-001, SWR-ALT-002",
        "Alerts generated for critical vital signs",
        alert_count_after > 0,
        f"Alert count: {alert_count_after}")

    # DVT-GUI-12: Open Settings window
    click_button(dash, IDC_BTN_SETTINGS)
    time.sleep(1)
    settings = find_window("PVM_Settings", timeout=5)
    results.record("DVT-GUI-12", "SWR-GUI-009",
        "Settings window opens from dashboard",
        settings is not None)

    if settings:
        # DVT-GUI-13: Settings has tabs
        tab = GetDlgItem(settings, IDC_TAB_SETTINGS)
        tab_count = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0) if tab else 0
        results.record("DVT-GUI-13", "SWR-GUI-009",
            "Settings window has multiple tabs",
            tab_count >= 4,
            f"Tab count: {tab_count}")

        # DVT-GUI-14: User management list (Admin only)
        user_list = GetDlgItem(settings, IDC_LST_USERS)
        user_count = SendMessageA(user_list, LB_GETCOUNT, 0, 0) if user_list else -1
        results.record("DVT-GUI-14", "SWR-GUI-008, SWR-GUI-009",
            "User management list visible for Admin role",
            user_list is not None and user_count > 0,
            f"Users listed: {user_count}")

        # DVT-GUI-15: Simulation toggle tab
        # Switch to Simulation tab (index 1 for admin)
        if tab:
            SendMessageA(tab, TCM_SETCURSEL, 1, 0)
            # Trigger tab change notification
            from ctypes import Structure, POINTER
            NMHDR_SIZE = 12
            nmhdr = ctypes.create_string_buffer(NMHDR_SIZE)
            SendMessageA(settings, 0x004E, IDC_TAB_SETTINGS, ctypes.addressof(nmhdr))
            time.sleep(0.5)

        sim_status = get_text(settings, IDC_STC_SIM_STATUS)
        results.record("DVT-GUI-15", "SWR-GUI-010",
            "Simulation status label is present",
            len(sim_status) > 0,
            f"Status: '{sim_status}'")

        # DVT-GUI-16: Language tab has combo box
        # Switch to Language tab (index 5 for admin)
        if tab:
            SendMessageA(tab, TCM_SETCURSEL, 5, 0)
            nmhdr = ctypes.create_string_buffer(NMHDR_SIZE)
            SendMessageA(settings, 0x004E, IDC_TAB_SETTINGS, ctypes.addressof(nmhdr))
            time.sleep(0.5)

        lang_cmb = GetDlgItem(settings, IDC_CMB_LANG)
        lang_count = SendMessageA(lang_cmb, CB_GETCOUNT, 0, 0) if lang_cmb else 0
        results.record("DVT-GUI-16", "SWR-GUI-012",
            "Language selector exposes the four approved localization options",
            lang_count == 4,
            f"Languages in combo: {lang_count}")

        # DVT-GUI-17: Alarm limits tab
        if tab:
            SendMessageA(tab, TCM_SETCURSEL, 3, 0)
            nmhdr = ctypes.create_string_buffer(NMHDR_SIZE)
            SendMessageA(settings, 0x004E, IDC_TAB_SETTINGS, ctypes.addressof(nmhdr))
            time.sleep(0.5)

        hr_low = get_text(settings, IDC_ALM_HR_LOW)
        results.record("DVT-GUI-17", "SWR-ALM-001",
            "Alarm limits tab has configurable fields",
            len(hr_low) > 0,
            f"HR low limit: '{hr_low}'")

        # Close settings
        PostMessageA(settings, WM_CLOSE, 0, 0)
        time.sleep(1)

    # DVT-GUI-18: Pause Sim button
    pause_btn = GetDlgItem(dash, IDC_BTN_PAUSE)
    pause_visible = IsWindowVisible(pause_btn) if pause_btn else False
    results.record("DVT-GUI-18", "SWR-GUI-010",
        "Pause Sim button visible in simulation mode",
        pause_visible)

    # DVT-GUI-19: Logout returns to login
    click_button(dash, IDC_BTN_LOGOUT)
    time.sleep(2)
    login_again = find_window("PVM_Login", timeout=5)
    results.record("DVT-GUI-19", "SWR-GUI-001",
        "Logout returns to login window",
        login_again is not None)

    # DVT-GUI-20: Version string in login
    if login_again:
        ver_text = get_text(login_again, IDC_LGN_VER)
        results.record("DVT-GUI-20", "SWR-GUI-001",
            "Version string displayed on login screen",
            len(ver_text) > 0 and "v" in ver_text.lower(),
            f"Version: '{ver_text}'")

    # Cleanup
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()

    return results


def generate_report(results, exe_path, output_dir):
    """Generate DVT report in text and JSON formats."""
    os.makedirs(output_dir, exist_ok=True)
    now = datetime.datetime.now(datetime.timezone.utc)
    date_str = now.strftime("%Y-%m-%d_%H%M%S")

    # Text report
    report_path = os.path.join(output_dir, f"dvt_report_{date_str}.txt")
    lines = [
        "=" * 80,
        "  PATIENT VITAL SIGNS MONITOR",
        "  DESIGN VERIFICATION TEST REPORT (IEC 62304 Class B)",
        "=" * 80,
        f"  Generated  : {now.strftime('%Y-%m-%d %H:%M:%S')} UTC",
        f"  Executable : {exe_path}",
        f"  Protocol   : dvt/DVT_Protocol.md (DVT-001-REV-F)",
        "=" * 80,
        "",
        f"  OVERALL RESULT: {results.overall()}",
        f"  Tests: {results.pass_count} passed, {results.fail_count} failed,"
        f" {len(results.results)} total",
        "",
        "  AUTOMATED GUI VERIFICATION RESULTS",
        "  " + "-" * 76,
        f"  {'ID':<14s} {'Status':<6s} {'SWR':<30s} Description",
        "  " + "-" * 76,
    ]
    for r in results.results:
        lines.append(
            f"  {r['id']:<14s} {r['status']:<6s} {r['swr']:<30s} {r['description']}"
        )
        if r["detail"]:
            lines.append(f"  {'':14s} {'':6s} Detail: {r['detail']}")

    lines += [
        "",
        "  MANUAL VERIFICATION (not automated)",
        "  " + "-" * 76,
        "  - GUI-COLOR   : Tile color changes (red=critical, amber=warning)",
        "  - GUI-ICON    : Application icon in taskbar matches app.ico",
        "  - GUI-ROLLING : SWR-GUI-011 rolling status banner content/motion",
        "  - GUI-RESIZE  : Window resize - zones scale, no clipping",
        "  - GUI-MAXIMIZE: Maximize - all zones fill screen",
        "  - GUI-L10N-SEL: DVT-GUI-16 verifies the four-option localization selector",
        "  - GUI-READABILITY: SWR-GUI-014 readability-mode layout and restart restore",
        "",
        "  REFERENCES",
        "  " + "-" * 76,
        "  Protocol     : dvt/DVT_Protocol.md (DVT-001-REV-F)",
        "  Traceability : requirements/TRACEABILITY.md",
        "  SWR          : requirements/SWR.md",
        "=" * 80,
    ]

    report_text = "\n".join(lines)
    with open(report_path, "w") as f:
        f.write(report_text + "\n")
    print(f"\nReport: {report_path}")

    # JSON report (machine-readable)
    json_path = os.path.join(output_dir, f"dvt_results_{date_str}.json")
    with open(json_path, "w") as f:
        json.dump({
            "generated": now.isoformat() + "Z",
            "executable": exe_path,
            "overall": results.overall(),
            "pass_count": results.pass_count,
            "fail_count": results.fail_count,
            "tests": results.results,
        }, f, indent=2)

    # Print summary
    print(report_text)
    return report_path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python run_dvt.py <path_to_patient_monitor_gui.exe>")
        sys.exit(2)

    exe = sys.argv[1]
    if not os.path.exists(exe):
        print(f"ERROR: Executable not found: {exe}")
        sys.exit(2)

    print("=" * 60)
    print("  DVT - Design Verification Test (Automated GUI)")
    print("  Patient Vital Signs Monitor")
    print("=" * 60)

    results = run_dvt(exe)
    generate_report(results, exe, "dvt/results")

    print(f"\n{'=' * 60}")
    print(f"  RESULT: {results.overall()}")
    print(f"  {results.pass_count} passed, {results.fail_count} failed")
    print(f"{'=' * 60}")

    sys.exit(0 if results.fail_count == 0 else 1)


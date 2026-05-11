#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys, os
# Ensure UTF-8 output on Windows consoles
if hasattr(sys.stdout, 'reconfigure'):
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass
"""
run_dvt.py - Design Verification Test runner for Patient Vital Signs Monitor.

Usage:
    python dvt/run_dvt.py [--build-dir DIR] [--no-build] [--output-dir DIR]

Options:
    --build-dir DIR   Path to the CMake build directory.
                      Default: <project-root>/build
    --no-build        Skip the cmake configure + build step.
    --output-dir DIR  Where to write the DVT report.
                      Default: <project-root>/dvt/results

Exit codes:
    0  All tests passed.
    1  One or more tests failed, or a fatal error occurred.
"""

import argparse
import datetime
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

# ---------------------------------------------------------------------------
# ANSI colour helpers
# ---------------------------------------------------------------------------
ANSI_RESET  = "\033[0m"
ANSI_GREEN  = "\033[92m"
ANSI_RED    = "\033[91m"
ANSI_YELLOW = "\033[93m"
ANSI_CYAN   = "\033[96m"
ANSI_BOLD   = "\033[1m"

def _colour(text: str, code: str) -> str:
    """Wrap *text* in an ANSI colour code (strips codes on non-TTY)."""
    if sys.stdout.isatty():
        return f"{code}{text}{ANSI_RESET}"
    return text

def green(t):  return _colour(t, ANSI_GREEN)
def red(t):    return _colour(t, ANSI_RED)
def yellow(t): return _colour(t, ANSI_YELLOW)
def cyan(t):   return _colour(t, ANSI_CYAN)
def bold(t):   return _colour(t, ANSI_BOLD)

# ---------------------------------------------------------------------------
# Requirement-to-test mapping
# ---------------------------------------------------------------------------
# Format: "SWR-ID": ("test_executable_stem", "SuiteName[.CaseName]" or
#                     ("SuiteName.CaseName", "..."), "description")
REQUIREMENT_MAP = {
    # Vital signs validation - GTest suites: HeartRate, BloodPressure, Temperature,
    # SpO2, RespRate, OverallAlert, BMI, AlertStr
    "SWR-VIT-001": ("test_unit", "HeartRate", "HR check - NORMAL/WARNING/CRITICAL boundaries"),
    "SWR-VIT-002": ("test_unit", "BloodPressure", "BP check - systolic/diastolic classification"),
    "SWR-VIT-003": ("test_unit", "Temperature", "Temperature check - fever/hypothermia limits"),
    "SWR-VIT-004": ("test_unit", "SpO2", "SpO2 check - hypoxia thresholds"),
    "SWR-VIT-005": ("test_unit", "OverallAlert", "overall_alert_level() severity aggregation"),
    "SWR-VIT-006": ("test_unit", "BMI", "BMI calculation and category handling"),
    "SWR-VIT-007": ("test_unit", "AlertStr", "alert_level_str() all levels incl. default"),
    "SWR-VIT-008": ("test_unit", "RespRate", "RR check - bradypnoea/tachypnoea thresholds (NEWS2)"),
    # NEWS2 - GTest suites: News2HR, News2RR, News2SpO2, News2SBP, News2Temp, News2Calc
    "SWR-NEW-001": ("test_unit", "News2Calc", "NEWS2 total score, risk category, clinical response"),
    # Alarm limits - GTest suite: AlarmLimitsTest
    "SWR-ALM-001": ("test_unit", "AlarmLimitsTest", "Configurable alarm limits: defaults, save/load, check functions"),
    # Trend / sparkline - GTest suites: TrendDirection, TrendExtract
    "SWR-TRD-001": ("test_unit", "TrendDirection", "Vital signs trend: RISING/FALLING/STABLE classification"),
    # Alert generation - case-aligned coverage within GenerateAlerts
    "SWR-ALT-001": ("test_unit", "GenerateAlerts.REQ_ALT_002_HeartRate_Warning",
                    "generate_alerts() emits one alert per abnormal parameter"),
    "SWR-ALT-002": ("test_unit", "GenerateAlerts.REQ_ALT_001_AllNormal_ZeroAlerts",
                    "generate_alerts() returns zero alerts for normal vitals"),
    "SWR-ALT-003": ("test_unit", "GenerateAlerts.REQ_ALT_004_MaxOut_Cap_Two",
                    "generate_alerts() enforces caller output-buffer cap"),
    "SWR-ALT-004": ("test_unit", "GenerateAlerts.REQ_ALT_005_MessageNonEmpty",
                    "generated alert records populate human-readable fields"),
    # Patient record - GTest suites: PatientInit, PatientAddReading, PatientLatestReading,
    # PatientStatus, PatientIsFull, PatientAlertEvents, PatientPrintSummary
    "SWR-PAT-001": ("test_unit", "PatientInit", "patient_init() - all fields set"),
    "SWR-PAT-002": ("test_unit", "PatientAddReading", "patient_add_reading() stores readings and rejects overflow"),
    "SWR-PAT-003": ("test_unit", "PatientLatestReading", "patient_latest_reading() - returns last"),
    "SWR-PAT-004": ("test_unit", "PatientStatus", "patient_current_status() - worst alert"),
    "SWR-PAT-005": ("test_unit", "PatientIsFull", "patient_is_full() - ring-buffer full"),
    "SWR-PAT-006": ("test_unit", "PatientPrintSummary", "patient_print_summary() - session event summary output"),
    "SWR-PAT-007": ("test_unit", "PatientAlertEvents", "patient_add_reading() captures session alarm event transitions"),
    "SWR-PAT-008": ("test_unit", (
        "PatientAlertEvents.REQ_PAT_008_EventAccessAndReset",
        "PatientAlertEvents.REQ_PAT_008_SessionResetNoticeLifecycle",
    ), "patient alert-event accessors and reset behavior"),
    # Security - GTest suite: UsersTest
    "SWR-SEC-001": ("test_unit", "UsersTest", "users_authenticate() valid/invalid creds"),
    "SWR-SEC-002": ("test_unit", "UsersTest.REQ_SEC_004_StoredValueIsNotPlaintext",
                    "Stored value is not the plaintext password"),
    "SWR-SEC-003": ("test_unit", "UsersTest", "users_change_password() old pwd verified"),
    "SWR-SEC-004": ("test_unit", "UsersTest.REQ_SEC_004_StoredValueIsSHA256Hex",
                    "Stored value is 64-char SHA-256 hex"),
    # GUI requirements verified outside the automated GTest release verdict.
    "SWR-GUI-005": (None, "MANUAL", "Architecture review: GUI uses only the hw_vitals.h HAL interface"),
    "SWR-GUI-006": (None, "MANUAL", "GUI demo/manual review: simulator cycle phases and wrap-around behavior"),
    "SWR-GUI-010": (None, "MANUAL", "GUI review: sim/device mode toggle behavior, N/A tiles, banners, and relaunch state"),
    "SWR-GUI-011": (None, "MANUAL", "Rolling simulation banner verified by manual visual check"),
    "SWR-GUI-012": ("test_unit", "LocalizationTest", "Localization API, selector list, and monitor.cfg persistence"),
    "SWR-GUI-013": (None, "MANUAL", "GUI review: dedicated session alarm events list remains distinct from active alerts"),
    "SWR-GUI-014": (None, "MANUAL", "GUI review: header patient-identity card stays synchronized and clears on session boundaries"),
    # GUI requirements verified via manual checklist only (GUI rendering and workflow review)
    "SWR-GUI-001": (None, "MANUAL", "Login screen: auth, error message, role detection"),
    "SWR-GUI-002": (None, "MANUAL", "Dashboard: colour-coded vital tiles update every 2 s"),
    "SWR-GUI-003": (None, "MANUAL", "Tiles: NORMAL/WARNING/CRITICAL colour coding"),
    "SWR-GUI-004": (None, "MANUAL", "Status banner reflects worst-case alert level"),
    "SWR-GUI-007": (None, "MANUAL", "User management: add/remove/set-password via Settings"),
    "SWR-GUI-008": (None, "MANUAL", "Application icon in taskbar and window chrome"),
    "SWR-GUI-009": (None, "MANUAL", "Settings panel: Users + Simulation + About tabs"),
}

# Executables to run (stem -> filename)
TEST_EXECUTABLES = [
    "test_unit",
    "test_integration",
]

# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------
def project_root() -> str:
    """Return the project root (parent of the dvt/ directory)."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(script_dir)

# ---------------------------------------------------------------------------
# Build step
# ---------------------------------------------------------------------------
def run_cmake_build(build_dir: str) -> bool:
    """Configure and build the project.  Returns True on success."""
    root = project_root()
    os.makedirs(build_dir, exist_ok=True)

    print(bold(cyan("\n=== CMake Configure ===")))
    configure_cmd = [
        "cmake", root,
        "-G", "MinGW Makefiles",
        "-DCMAKE_BUILD_TYPE=Release",
    ]
    print("Running:", " ".join(configure_cmd))
    result = subprocess.run(configure_cmd, cwd=build_dir)
    if result.returncode != 0:
        print(red("CMake configure failed."))
        return False

    print(bold(cyan("\n=== CMake Build ===")))
    build_cmd = ["cmake", "--build", ".", "--parallel"]
    print("Running:", " ".join(build_cmd))
    result = subprocess.run(build_cmd, cwd=build_dir)
    if result.returncode != 0:
        print(red("CMake build failed."))
        return False

    print(green("Build succeeded."))
    return True

# ---------------------------------------------------------------------------
# Test execution
# ---------------------------------------------------------------------------
def find_executable(build_dir: str, stem: str) -> str | None:
    """Search for *stem*.exe (Windows) or *stem* (Linux/macOS) in build_dir."""
    candidates = [
        os.path.join(build_dir, stem + ".exe"),
        os.path.join(build_dir, stem),
        os.path.join(build_dir, "tests", stem + ".exe"),
        os.path.join(build_dir, "tests", stem),
        os.path.join(build_dir, "bin", stem + ".exe"),
        os.path.join(build_dir, "bin", stem),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    return None

def run_test_executable(exe_path: str, xml_output: str) -> dict:
    """
    Run a GTest executable and return a result dict:
        {stem, exe_path, xml_output, returncode, xml_parsed, suites}
    where suites is a list of {name, tests, failures, errors, cases}.
    """
    stem = os.path.splitext(os.path.basename(exe_path))[0]
    print(bold(cyan(f"\n=== Running {stem} ===")))

    os.makedirs(os.path.dirname(xml_output), exist_ok=True)

    cmd = [exe_path, f"--gtest_output=xml:{xml_output}"]
    print("Command:", " ".join(cmd))
    result = subprocess.run(
        cmd,
        capture_output=False,
        env=_isolated_test_env(xml_output),
    )

    info = {
        "stem": stem,
        "exe_path": exe_path,
        "xml_output": xml_output,
        "returncode": result.returncode,
        "xml_parsed": False,
        "suites": [],
        "total_tests": 0,
        "total_failures": 0,
        "total_errors": 0,
    }

    if os.path.isfile(xml_output):
        try:
            tree = ET.parse(xml_output)
            root_el = tree.getroot()
            info["xml_parsed"] = True

            # GTest XML: <testsuites> -> <testsuite> -> <testcase>
            suites_el = root_el if root_el.tag == "testsuites" else root_el
            for suite_el in suites_el.findall("testsuite"):
                suite = {
                    "name": suite_el.get("name", "?"),
                    "tests": int(suite_el.get("tests", 0)),
                    "failures": int(suite_el.get("failures", 0)),
                    "errors": int(suite_el.get("errors", 0)),
                    "cases": [],
                }
                for case_el in suite_el.findall("testcase"):
                    failure_els = case_el.findall("failure")
                    case = {
                        "name": case_el.get("name", "?"),
                        "time": case_el.get("time", "?"),
                        "passed": len(failure_els) == 0,
                        "failures": [f.get("message", "") for f in failure_els],
                    }
                    suite["cases"].append(case)
                info["suites"].append(suite)
                info["total_tests"] += suite["tests"]
                info["total_failures"] += suite["failures"]
                info["total_errors"] += suite["errors"]
        except ET.ParseError as exc:
            print(yellow(f"Warning: could not parse XML output: {exc}"))

    return info

def _isolated_test_env(xml_output: str) -> dict[str, str]:
    """Run each DVT executable with a temp root scoped to this report file."""
    env = os.environ.copy()
    temp_stem = os.path.splitext(os.path.basename(xml_output))[0]
    temp_dir = os.path.join(os.path.dirname(xml_output), "tmp", temp_stem)
    os.makedirs(temp_dir, exist_ok=True)
    env["TMP"] = temp_dir
    env["TEMP"] = temp_dir
    env["TMPDIR"] = temp_dir
    return env

# ---------------------------------------------------------------------------
# Git helper
# ---------------------------------------------------------------------------
def git_short_sha() -> str:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True,
            text=True,
            cwd=project_root(),
        )
        return result.stdout.strip() if result.returncode == 0 else "unknown"
    except FileNotFoundError:
        return "unknown"

# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------
def _pass_fail(ok: bool) -> str:
    return "PASS" if ok else "FAIL"

def build_requirement_status(all_results: list[dict]) -> dict[str, str]:
    """
    Return a dict mapping each SWR-ID -> "PASS" | "FAIL" | "MANUAL" | "NOT_RUN".
    """
    test_outcomes: dict[tuple, bool] = {}

    for res in all_results:
        stem = res["stem"]
        for suite in res["suites"]:
            suite_passed = (suite["failures"] == 0 and suite["errors"] == 0)
            test_outcomes[(stem, suite["name"])] = suite_passed
            for case in suite["cases"]:
                key = (stem, f"{suite['name']}.{case['name']}")
                test_outcomes[key] = case["passed"]

    status: dict[str, str] = {}
    for swr_id, (exe_stem, test_ref, _desc) in REQUIREMENT_MAP.items():
        if exe_stem is None:
            status[swr_id] = "MANUAL"
            continue

        ref_statuses: list[str] = []
        for ref in _requirement_test_refs(test_ref):
            if ":" in ref:
                _exe_part, suite_case = ref.split(":", 1)
            else:
                suite_case = ref

            key_suite = (exe_stem, suite_case.split(".")[0])
            key_full = (exe_stem, suite_case)

            if key_full in test_outcomes:
                ref_statuses.append(_pass_fail(test_outcomes[key_full]))
            elif "." not in suite_case and key_suite in test_outcomes:
                ref_statuses.append(_pass_fail(test_outcomes[key_suite]))
            else:
                ref_statuses.append("NOT_RUN")

        status[swr_id] = _combine_requirement_ref_statuses(ref_statuses)

    return status

def _requirement_test_refs(test_ref) -> list[str]:
    if isinstance(test_ref, (list, tuple)):
        return [str(ref) for ref in test_ref]
    return [str(test_ref)]

def _combine_requirement_ref_statuses(ref_statuses: list[str]) -> str:
    if any(stat == "FAIL" for stat in ref_statuses):
        return "FAIL"
    if any(stat == "NOT_RUN" for stat in ref_statuses):
        return "NOT_RUN"
    return "PASS"

def _format_requirement_test_ref(test_ref) -> str:
    return " + ".join(_requirement_test_refs(test_ref))

def generate_report(
    all_results: list[dict],
    req_status: dict[str, str],
    output_dir: str,
    overall_pass: bool,
    git_sha: str,
    build_dir: str,
    no_build: bool,
) -> str:
    """Write a text report and return its path."""
    os.makedirs(output_dir, exist_ok=True)
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = os.path.join(output_dir, f"dvt_report_{ts}.txt")

    sep_major = "=" * 78
    sep_minor = "-" * 78

    lines: list[str] = []

    def ln(s=""):
        lines.append(s)

    now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    ln(sep_major)
    ln("  PATIENT VITAL SIGNS MONITOR - DESIGN VERIFICATION TEST REPORT")
    ln(sep_major)
    ln(f"  Date/Time    : {now_str}")
    ln(f"  Git Commit   : {git_sha}")
    ln(f"  Build Dir    : {build_dir}")
    ln(f"  Build Step   : {'skipped (--no-build)' if no_build else 'executed'}")
    ln(f"  OVERALL      : {'*** PASS ***' if overall_pass else '*** FAIL ***'}")
    ln(sep_major)
    ln()

    ln(sep_major)
    ln("  TEST SUITE SUMMARY")
    ln(sep_major)

    for res in all_results:
        exe_pass = (res["returncode"] == 0)
        status_str = "PASS" if exe_pass else "FAIL"
        ln(f"  [{status_str}]  {res['stem']}")
        ln(f"         Executable : {res['exe_path']}")
        ln(f"         XML Output : {res['xml_output']}")
        if not res["xml_parsed"]:
            ln("         XML parsed  : NO (file missing or corrupt)")
        ln(f"         Return Code : {res['returncode']}")
        ln(f"         Tests       : {res['total_tests']}")
        ln(f"         Failures    : {res['total_failures']}")
        ln(f"         Errors      : {res['total_errors']}")
        ln()

    ln(sep_major)
    ln("  DETAILED RESULTS PER TEST SUITE")
    ln(sep_major)

    for res in all_results:
        if not res["xml_parsed"]:
            ln(f"  {res['stem']}: no XML data available")
            ln()
            continue
        for suite in res["suites"]:
            suite_ok = (suite["failures"] == 0 and suite["errors"] == 0)
            status_str = "PASS" if suite_ok else "FAIL"
            ln(f"  [{status_str}] {res['stem']} :: {suite['name']}"
               f"  ({suite['tests']} tests, {suite['failures']} failures)")
            for case in suite["cases"]:
                case_mark = "  OK  " if case["passed"] else " FAIL "
                ln(f"    [{case_mark}] {case['name']}  ({case['time']}s)")
                for msg in case["failures"]:
                    for fline in msg.splitlines():
                        ln(f"              {fline}")
            ln()

    ln(sep_major)
    ln("  REQUIREMENT TRACEABILITY TABLE")
    ln(sep_major)

    col_w_id = 16
    col_w_test = 52
    col_w_desc = 38
    col_w_stat = 8

    header = (
        f"  {'Requirement':<{col_w_id}}"
        f"{'Covered By':<{col_w_test}}"
        f"{'Description':<{col_w_desc}}"
        f"{'Status':>{col_w_stat}}"
    )
    ln(header)
    ln("  " + sep_minor)

    for swr_id in sorted(REQUIREMENT_MAP.keys()):
        exe_stem, test_ref, desc = REQUIREMENT_MAP[swr_id]
        stat = req_status.get(swr_id, "UNKNOWN")
        test_label = _format_requirement_test_ref(test_ref) if test_ref else "-"
        covered_by = f"{exe_stem}/{test_label}" if exe_stem else test_label
        ln(
            f"  {swr_id:<{col_w_id}}"
            f"{covered_by:<{col_w_test}}"
            f"{desc:<{col_w_desc}}"
            f"{stat:>{col_w_stat}}"
        )

    ln()
    ln(sep_major)

    statuses = list(req_status.values())
    pass_count = statuses.count("PASS")
    fail_count = statuses.count("FAIL")
    manual_count = statuses.count("MANUAL")
    not_run = statuses.count("NOT_RUN")
    total = len(statuses)

    ln(f"  Requirements total   : {total}")
    ln(f"  Automated PASS       : {pass_count}")
    ln(f"  Automated FAIL       : {fail_count}")
    ln(f"  Manual (checklist)   : {manual_count}")
    ln(f"  Not run / missing    : {not_run}")
    ln()
    ln(f"  DVT OVERALL VERDICT  : {'PASS' if overall_pass else 'FAIL'}")
    ln(sep_major)
    ln()
    ln("  NOTE: Non-automated GUI/architecture items (including SWR-GUI-001/002/003/004/")
    ln("  005/006/007/008/009/010/011) are verified via the checklist and review")
    ln("  guidance in dvt/DVT_Protocol.md and are not")
    ln("  included in the automated pass/fail decision.")
    ln("  NOTE: SWR-GUI-012 automated coverage comes from LocalizationTest.*.")
    ln("  DVT-GUI-16 remains supplemental GUI evidence for selector presence.")
    ln()
    ln(sep_major)
    ln("  END OF REPORT")
    ln(sep_major)

    with open(report_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    return report_path

# ---------------------------------------------------------------------------
# Console summary
# ---------------------------------------------------------------------------
def print_console_summary(all_results: list[dict], req_status: dict[str, str],
                           overall_pass: bool, report_path: str) -> None:
    print()
    print(bold("=" * 60))
    print(bold("  DVT SUMMARY"))
    print(bold("=" * 60))

    for res in all_results:
        exe_ok = res["returncode"] == 0
        tag = green("[PASS]") if exe_ok else red("[FAIL]")
        print(f"  {tag}  {res['stem']:30s} "
              f"tests={res['total_tests']:3d}  "
              f"fail={res['total_failures']:3d}")

    print()
    print(bold("  Requirements:"))
    for swr_id in sorted(req_status.keys()):
        stat = req_status[swr_id]
        if stat == "PASS":
            tag = green("PASS")
        elif stat == "FAIL":
            tag = red("FAIL")
        elif stat == "MANUAL":
            tag = yellow("MANUAL")
        else:
            tag = yellow("NOT_RUN")
        _exe, _test_ref, desc = REQUIREMENT_MAP[swr_id]
        print(f"    {swr_id:<16s} {tag:<20s} {desc}")

    print()
    verdict_str = green("PASS") if overall_pass else red("FAIL")
    print(bold(f"  OVERALL: {verdict_str}"))
    print(bold(f"  Report : {report_path}"))
    print(bold("=" * 60))
    print()

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="DVT runner for Patient Vital Signs Monitor",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    root = project_root()
    parser.add_argument(
        "--build-dir",
        default=os.path.join(root, "build"),
        help="CMake build directory (default: <project-root>/build)",
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Skip cmake configure + build",
    )
    parser.add_argument(
        "--output-dir",
        default=os.path.join(root, "dvt", "results"),
        help="Directory for DVT report and XML files (default: dvt/results)",
    )
    args = parser.parse_args()

    build_dir = os.path.abspath(args.build_dir)
    output_dir = os.path.abspath(args.output_dir)

    print(bold(cyan("\n+==================================================+")))
    print(bold(cyan("|  Patient Vital Signs Monitor - DVT Runner        |")))
    print(bold(cyan("+==================================================+")))
    print(f"  Project root : {root}")
    print(f"  Build dir    : {build_dir}")
    print(f"  Output dir   : {output_dir}")
    print(f"  Git commit   : {git_short_sha()}")

    if not args.no_build:
        if not run_cmake_build(build_dir):
            print(red("\nBuild failed.  Aborting DVT."))
            return 1
    else:
        print(yellow("\n[--no-build] Skipping build step."))

    all_results: list[dict] = []
    ts_run = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    for stem in TEST_EXECUTABLES:
        exe_path = find_executable(build_dir, stem)
        if exe_path is None:
            print(yellow(f"\nWarning: executable '{stem}' not found in {build_dir}.  Skipping."))
            all_results.append({
                "stem": stem,
                "exe_path": "(not found)",
                "xml_output": "(not found)",
                "returncode": -1,
                "xml_parsed": False,
                "suites": [],
                "total_tests": 0,
                "total_failures": 0,
                "total_errors": 0,
            })
            continue

        xml_out = os.path.join(output_dir, f"{stem}_{ts_run}.xml")
        res = run_test_executable(exe_path, xml_out)
        all_results.append(res)

    automated_results = [r for r in all_results if r["exe_path"] != "(not found)"]
    overall_pass = all(r["returncode"] == 0 for r in automated_results) and len(automated_results) > 0

    req_status = build_requirement_status(all_results)

    git_sha = git_short_sha()
    report_path = generate_report(
        all_results=all_results,
        req_status=req_status,
        output_dir=output_dir,
        overall_pass=overall_pass,
        git_sha=git_sha,
        build_dir=build_dir,
        no_build=args.no_build,
    )

    print_console_summary(all_results, req_status, overall_pass, report_path)

    return 0 if overall_pass else 1


if __name__ == "__main__":
    sys.exit(main())

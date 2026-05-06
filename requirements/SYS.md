# System Requirements Specification (SYS)

**Document ID:** SYS-001-REV-G
**Project:** Patient Vital Signs Monitor
**Version:** 1.0.0
**Date:** 2026-05-06
**Status:** Approved
**Standard:** 21 CFR 820.30(d) / IEC 62304 §5.1

---

## Purpose

This document specifies what the system must do to satisfy the User Needs (UNS).
Each requirement here must trace to at least one UNS entry, and every UNS entry
must be covered by at least one SYS entry. Every Software Requirement (SWR) must
trace back to at least one entry here.

---

## System Requirements

### SYS-001 — Heart Rate Classification
**Requirement:** The system shall classify heart rate readings into NORMAL,
WARNING, or CRITICAL using AHA/ACC 2019 guideline thresholds:

| Range (bpm) | Classification |
|-------------|----------------|
| 60 – 100    | NORMAL         |
| 41 – 59 or 101 – 150 | WARNING |
| ≤ 40 or ≥ 151 | CRITICAL     |

**Traces to:** UNS-001, UNS-005, UNS-006

---

### SYS-002 — Blood Pressure Classification
**Requirement:** The system shall classify blood pressure readings (systolic and
diastolic independently) using JNC-8 / ESC 2018 guideline thresholds.
The more severe of the two individual classifications shall be returned:

| Systolic (mmHg) | Diastolic (mmHg) | Classification |
|-----------------|------------------|----------------|
| 90 – 140        | 60 – 90          | NORMAL         |
| 71–89 or 141–180 | 41–59 or 91–120 | WARNING        |
| ≤ 70 or ≥ 181   | ≤ 40 or ≥ 121   | CRITICAL       |

**Traces to:** UNS-002, UNS-005, UNS-006

---

### SYS-003 — Temperature Classification
**Requirement:** The system shall classify body temperature readings using WHO
clinical reference ranges:

| Range (°C)     | Classification |
|----------------|----------------|
| 36.1 – 37.2    | NORMAL         |
| 35.0–36.0 or 37.3–39.5 | WARNING |
| < 35.0 or > 39.5 | CRITICAL    |

**Traces to:** UNS-003, UNS-005, UNS-006

---

### SYS-004 — SpO2 Classification
**Requirement:** The system shall classify peripheral oxygen saturation readings
using British Thoracic Society supplemental oxygen guidelines:

| SpO2 (%)   | Classification |
|------------|----------------|
| 95 – 100   | NORMAL         |
| 90 – 94    | WARNING        |
| < 90       | CRITICAL       |

**Traces to:** UNS-004, UNS-005, UNS-006

---

### SYS-005 — Alert Record Generation
**Requirement:** The system shall generate a structured alert record for each
vital sign parameter that is not within the NORMAL range. Each alert record
shall include the parameter name, severity level, and a human-readable message
stating the measured value and the normal range.
**Traces to:** UNS-005, UNS-006

---

### SYS-006 — Aggregate Alert Level
**Requirement:** The system shall compute an aggregate patient alert level equal
to the highest individual parameter alert level across all monitored parameters.
A single CRITICAL parameter shall elevate the overall status to CRITICAL
regardless of the status of all other parameters.
**Traces to:** UNS-005, UNS-006, UNS-010

---

### SYS-007 — BMI Calculation and Classification
**Requirement:** The system shall calculate Body Mass Index using the formula
`BMI = weight_kg / height_m²` and classify the result per WHO categories:
Underweight (< 18.5), Normal weight (18.5–24.9), Overweight (25–29.9),
Obese (≥ 30). For invalid height (≤ 0), the system shall return a sentinel
value of -1.0 and display "Invalid".
**Traces to:** UNS-007

---

### SYS-008 — Patient Demographic Storage
**Requirement:** The system shall store and associate the following demographic
fields with each patient record: unique numeric ID, full name (up to 63
characters), age in years, body weight in kg, and height in metres.
**Traces to:** UNS-008

---

### SYS-009 — Vital Sign Reading History
**Requirement:** The system shall store a sequential history of up to
MAX\_READINGS (10) vital sign readings per patient session. Readings shall be
retrievable in the order they were recorded.
**Traces to:** UNS-009, UNS-011

---

### SYS-010 — Storage Capacity Enforcement
**Requirement:** When the vital sign reading buffer is full, the system shall
reject any further addition attempt and leave the existing record unchanged.
The rejection shall be indicated to the caller by a defined return value.
**Traces to:** UNS-011

---

### SYS-011 — Patient Status Summary Display
**Requirement:** The system shall display a formatted summary containing:
patient demographics, BMI with category, the most recent vital signs with
individual classifications, the aggregate alert level, and all active alert
messages.
**Traces to:** UNS-010

---

### SYS-012 — Static Memory Allocation
**Requirement:** The software shall use only static (stack or global) memory
allocation. Dynamic heap allocation (malloc, calloc, new) shall not be used
anywhere in the production code stack.
**Traces to:** UNS-011, UNS-012

---

### SYS-013 — User Authentication Enforcement
**Requirement:** The system shall display a login screen on launch and shall
not permit access to any patient data or monitoring function until a valid
username and password credential pair has been verified. On authentication
failure the system shall display a clear error message without revealing which
field was incorrect. A Logout function shall terminate the current session and
return the user to the login screen, clearing all session data.
**Traces to:** UNS-013

---

### SYS-014 — Graphical Vital Signs Dashboard
**Requirement:** The system shall present the most recent vital signs of the
active patient in a graphical dashboard using colour-coded status tiles:
green for NORMAL, amber for WARNING, and red for CRITICAL. The aggregate
alert status shall be displayed in a banner using the same colour convention.
The dashboard shall refresh automatically after each new reading is added.
During active monitoring, when the aggregate alert status is `WARNING` or
`CRITICAL`, the dashboard may display elapsed time for the current abnormal
aggregate alert state. That elapsed-time cue shall reset whenever the
aggregate alert level changes, monitoring pauses, the session is cleared or
re-admitted, the user logs out, or the system leaves active monitoring.
**Traces to:** UNS-014, UNS-005, UNS-006, UNS-010

---

### SYS-015 — Hardware Abstraction Layer
**Requirement:** The system shall isolate all data acquisition behind a
hardware abstraction layer (HAL) consisting of two functions: `hw_init()`
and `hw_get_next_reading()`. The simulation back-end (`sim_vitals.c`) shall
be replaceable by a hardware driver translation unit without modifying any
other source file. All HAL implementations shall use only static storage.
**Traces to:** UNS-015

---

### SYS-016 — Multi-User Account Management
**Requirement:** The system shall maintain a persistent list of up to 8 named
user accounts. Each account shall contain a unique username (up to 63
characters), a display name (up to 63 characters), a user role
(`ROLE_ADMIN` or `ROLE_CLINICAL`), and a password of at least
`USERS_MIN_PASSWORD_LEN` (8) characters. The account list shall be persisted
between sessions in a local file (`users.dat`) located in the same directory
as the executable. When the file is absent or unreadable, the system shall
fall back to two built-in default accounts (one ADMIN, one CLINICAL). All
account storage shall use only static memory.
**Traces to:** UNS-016

---

### SYS-017 — Role-Based Access Control
**Requirement:** The system shall enforce role-based access control as follows:
- `ROLE_ADMIN` users shall have access to all patient monitoring functions
  and to the user management settings panel (add accounts, remove accounts,
  reset any account's password).
- `ROLE_CLINICAL` users shall have access to patient monitoring functions
  and to personal password change only; the user management settings panel
  shall not be accessible.
- Role assignment shall be determined at authentication time by
  `users_authenticate()` and shall remain fixed for the duration of the
  session. The role badge and available controls displayed in the dashboard
  header shall reflect the authenticated role.
**Traces to:** UNS-016

---

### SYS-018 — Respiration Rate Monitoring and Classification
**Requirement:** The system shall support respiration rate as a monitored vital
sign in the live monitoring dashboard. It shall classify adult
respiration-rate readings as:
- NORMAL for `12–20 br/min`
- WARNING for `9–11 br/min` or `21–24 br/min`
- CRITICAL for `≤ 8 br/min` or `≥ 25 br/min`

When a respiration-rate reading is not available for a monitoring cycle, the
system shall preserve that "not measured" state and shall not raise a spurious
respiration-rate alert or aggregate alert escalation from the missing value.
**Traces to:** UNS-005, UNS-006, UNS-014, UNS-015

---

### SYS-019 — NEWS2 Aggregate Clinical Risk Scoring
**Requirement:** The system shall compute and display a NEWS2 aggregate
clinical risk score from respiration rate, SpO2, systolic blood pressure,
heart rate, temperature, and AVPU using the approved Royal College of
Physicians NEWS2 tables. The system shall classify the resulting score into the
implemented `LOW`, `LOW_M`, `MEDIUM`, and `HIGH` response bands. When
respiration rate is not measured for a cycle, the missing value shall not
inflate the NEWS2 score.
**Traces to:** UNS-005, UNS-006, UNS-010, UNS-014

---

### SYS-020 — Session Alarm Event Review Storage
**Requirement:** The system shall derive and retain a sequential session-scoped
history of alert-state transition events from successfully recorded vital sign
readings. At most one event shall be appended per successful reading when the
aggregate alert level or abnormal parameter set changes. Each event shall store
the reading index, resulting alert level, and a deterministic summary derived
from the existing alert semantics. The retained event history shall clear
whenever patient session data is reinitialized.
**Traces to:** UNS-017, UNS-011

---

### SYS-021 — Session Alarm Event Review Presentation
**Requirement:** The system shall present the session alarm event history
distinctly from current active alerts in both the graphical dashboard and the
formatted patient summary. Both surfaces shall use the same stored event
records, preserve reading order, and indicate explicitly when no session alarm
events have been recorded.
**Traces to:** UNS-017, UNS-010

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Added SYS-013, SYS-014 (GUI) |
| C   | 2026-04-07 | vinu-engineer   | Added SYS-015 (HAL) |
| D   | 2026-04-07 | vinu-engineer   | Added SYS-016 (multi-user accounts), SYS-017 (RBAC) |
| E   | 2026-05-05 | Codex implementer | Added SYS-018 (RR) and SYS-019 (NEWS2) to restore defensible traceability for existing clinical requirements |
| F   | 2026-05-05 | Codex implementer | Added SYS-020 and SYS-021 for session alarm event review |
| G   | 2026-05-06 | Codex implementer | Extended SYS-014 with alert-state duration display and reset semantics for active monitoring |

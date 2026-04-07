# User Needs Specification (UNS)

**Document ID:** UNS-001-REV-A
**Project:** Patient Vital Signs Monitor
**Version:** 1.0.0
**Date:** 2026-04-06
**Status:** Approved
**Standard:** 21 CFR 820.30(c) / IEC 62304 §5.1

---

## Purpose

This document captures the clinical and operational needs of the intended users
(bedside clinicians, ward nurses, intensivists) as the basis for all downstream
system and software requirements. Every System Requirement (SYS) must trace back
to at least one entry here.

## Intended Use

The Patient Vital Signs Monitor is a software tool used by trained clinical staff
on a Windows workstation to monitor, classify, and record vital sign readings for
admitted patients and to generate alerts when readings deviate from normal ranges.

---

## User Needs

### UNS-001 — Heart Rate Monitoring
**Need:** The clinician shall be able to view a patient's heart rate reading and
know immediately whether it is within a safe range.
**Rationale:** Abnormal heart rate (bradycardia or tachycardia) is a primary
indicator of cardiac events and haemodynamic instability.
**Priority:** Critical

---

### UNS-002 — Blood Pressure Monitoring
**Need:** The clinician shall be able to view a patient's systolic and diastolic
blood pressure and know immediately whether the values are within a safe range.
**Rationale:** Hypertension and hypotension are leading causes of adverse
in-hospital events.
**Priority:** Critical

---

### UNS-003 — Temperature Monitoring
**Need:** The clinician shall be able to view a patient's body temperature and
know immediately whether it indicates fever, hypothermia, or hyperpyrexia.
**Rationale:** Temperature deviation indicates infection, sepsis, or
thermoregulatory failure.
**Priority:** Critical

---

### UNS-004 — Oxygen Saturation Monitoring
**Need:** The clinician shall be able to view a patient's peripheral oxygen
saturation (SpO2) and know immediately whether the patient is hypoxaemic.
**Rationale:** Unrecognised hypoxaemia is a major cause of preventable
in-hospital deterioration.
**Priority:** Critical

---

### UNS-005 — Automatic Abnormal Value Alerting
**Need:** The system shall automatically alert the clinician whenever any
monitored vital sign deviates from its clinically defined normal range, without
requiring the clinician to manually compare values against reference tables.
**Rationale:** Manual comparison is error-prone under workload; automated
alerting reduces the risk of missed deterioration.
**Priority:** Critical

---

### UNS-006 — Alert Severity Differentiation
**Need:** The system shall distinguish between a warning-level deviation (requires
review) and a critical-level deviation (requires immediate intervention) so that
clinicians can prioritise their response.
**Rationale:** All-or-nothing alerting leads to alarm fatigue; severity grading
supports appropriate triage.
**Priority:** Critical

---

### UNS-007 — BMI Display
**Need:** The clinician shall be able to view a patient's Body Mass Index (BMI)
and its WHO weight category (Underweight / Normal weight / Overweight / Obese).
**Rationale:** BMI informs dosing decisions, anaesthetic risk assessment, and
comorbidity screening.
**Priority:** High

---

### UNS-008 — Patient Identification
**Need:** The system shall associate all vital sign readings and alerts with a
uniquely identified patient record containing the patient's name, ID number,
and age.
**Rationale:** Reading-patient association is mandatory to prevent clinical
mix-ups.
**Priority:** Critical

---

### UNS-009 — Vital Sign History
**Need:** The system shall retain a chronological history of vital sign readings
for each patient within a session to enable trend observation.
**Rationale:** A single reading provides a snapshot; trends over time inform
clinical judgement about deterioration or recovery.
**Priority:** High

---

### UNS-010 — Consolidated Status Summary
**Need:** The system shall present a single-screen consolidated summary of a
patient's demographics, BMI, latest vital signs, overall alert status, and any
active alerts.
**Rationale:** Clinicians need a rapid overview without navigating multiple
screens, especially during handover or emergency response.
**Priority:** High

---

### UNS-011 — Data Integrity
**Need:** The system shall not corrupt, overwrite, or silently discard previously
recorded vital sign readings when new readings are added or when the storage
limit is reached.
**Rationale:** Loss or corruption of historical data could lead to incorrect
clinical decisions.
**Priority:** Critical

---

### UNS-012 — Platform Compatibility
**Need:** The software shall operate correctly on Windows-based clinical
workstations without requiring runtime installation of additional dependencies
beyond what is documented.
**Rationale:** Hospital IT environments are tightly controlled; unmanaged
dependencies create deployment and security risks.
**Priority:** High

---

### UNS-013 — User Authentication
**Need:** The system shall require the clinician to authenticate with a
username and password before accessing patient data or any monitoring function.
**Rationale:** Unauthorised access to patient health data is a clinical and
regulatory risk; access control is required under HIPAA and IEC 62304 risk
management.
**Priority:** Critical

---

### UNS-014 — Graphical User Interface
**Need:** The system shall provide a graphical dashboard that presents vital
sign readings and alert status in a clear, colour-coded format consistent with
clinical monitoring equipment conventions, and shall allow data entry without
use of a command line.
**Rationale:** Clinical staff in ward environments are not expected to use
command-line tools; a GUI improves speed and reduces input errors during
high-pressure scenarios.
**Priority:** High

---

### UNS-015 — Live Monitoring Feed
**Need:** The system shall display a continuous live feed of vital signs that
updates automatically without requiring any clinician action after login.
**Rationale:** Bedside monitoring must be continuous — requiring manual data
entry in real use would introduce latency and increase the risk of delayed
detection of deterioration. A simulation feed demonstrates the full monitoring
workflow and provides a plug point for real hardware integration.
**Priority:** High

---

### UNS-016 — Role-Based Access and Multi-User Support
**Need:** Different clinical staff roles (ward administrators and bedside
clinicians) require different levels of system access. An administrator must
be able to manage user accounts and credentials without exposing those
controls to clinical users.
**Rationale:** In a clinical environment, user management tasks (adding and
removing accounts, resetting passwords) must be separated from patient
monitoring to comply with role-based access control requirements and to
minimise the risk of accidental or unauthorised configuration changes.
Hospital IT governance mandates that only designated administrators may alter
access control settings.
**Priority:** High

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Added UNS-013, UNS-014 (GUI) |
| C   | 2026-04-07 | vinu-engineer   | Added UNS-015 (live monitoring feed / HAL) |
| D   | 2026-04-07 | vinu-engineer   | Added UNS-016 (role-based access / multi-user) |

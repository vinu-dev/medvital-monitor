# Architecture — Patient Vital Signs Monitor

**Document ID:** ARCH-001-REV-A  
**Version:** 2.1.0  
**Date:** 2026-04-08  
**Standard:** IEC 62304 §5.3 — Software architectural design  

---

## 1. Overview

The Patient Vital Signs Monitor is designed as a **layered, API-first system**
whose domain logic is intentionally decoupled from the presentation layer. This
allows the same clinically-verified C library to power a Win32 desktop client
today and a web or mobile application in the future — without touching the
certified core.

```
┌─────────────────────────────────────────────────────────────────┐
│                       PRESENTATION LAYER                         │
│   Win32 GUI (current)  │  Web SPA (future)  │  Mobile (future)  │
└────────────────────────┬────────────────────────────────────────┘
                         │ HTTP / JSON REST   (Win32 uses direct calls)
┌────────────────────────▼────────────────────────────────────────┐
│                       API GATEWAY LAYER  (future)                │
│           REST Server — libmongoose (single-header C)            │
│           Endpoints: /vitals  /alerts  /patient  /auth           │
└────────────────────────┬────────────────────────────────────────┘
                         │ C function calls
┌────────────────────────▼────────────────────────────────────────┐
│                     DOMAIN SERVICE LAYER  ← IEC 62304 scope      │
│   vitals.c  │  alerts.c  │  patient.c  │  pw_hash.c  │  auth    │
│                        monitor_lib (static library)               │
└────────────────────────┬────────────────────────────────────────┘
                         │ HAL interface  (hw_vitals.h)
┌────────────────────────▼────────────────────────────────────────┐
│                       HARDWARE LAYER                              │
│           sim_vitals.c (current)  │  hw_driver.c (real HW)       │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Layer Descriptions

### 2.1 Domain Service Layer (`monitor_lib`)

The **only layer that must be IEC 62304 certified**. It contains all
safety-critical business logic:

| Module | File | Responsibility |
|--------|------|----------------|
| Vitals Validation | `vitals.c` | Classify HR, BP, Temp, SpO2 into NORMAL / WARNING / CRITICAL |
| Alert Generation | `alerts.c` | Produce structured `Alert` messages from raw `VitalSigns` |
| Patient Record | `patient.c` | Ring-buffer of up to 10 readings; aggregate status; BMI |
| Password Hashing | `pw_hash.c` | FIPS 180-4 SHA-256; stack-only; no heap |
| Authentication | `gui_users.c` | Account CRUD, role management, hashed credential store |

**Constraints (IEC 62304 SYS-012):**
- No heap allocation — all storage is static or stack.
- No OS-specific headers in domain code.
- 100 % branch coverage required (enforced by `--coverage` + CI).

### 2.2 Hardware Abstraction Layer (`hw_vitals.h`)

Two function signatures that any back-end must implement:

```c
void hw_init(void);                       // called once on startup
void hw_get_next_reading(VitalSigns *out); // called every ~2 s
```

Current implementation: `sim_vitals.c` (20-entry clinical scenario table).  
Production replacement: `hw_driver.c` — reads from a real medical sensor
(serial, USB HID, ADC) with **zero changes** to any other file.

### 2.3 Application Services Layer

Cross-cutting concerns that live between domain and presentation:

| Module | File | Responsibility |
|--------|------|----------------|
| Config persistence | `app_config.c` | Read/write `monitor.cfg` (sim mode state) |
| Auth façade | `gui_auth.c` | Thin wrapper mapping legacy `auth_*` API to `gui_users.c` |

### 2.4 API Gateway Layer *(planned — not yet implemented)*

To expose the domain to web and mobile clients, a thin REST adapter will
wrap `monitor_lib`. Recommended implementation:

- **Library**: [libmongoose](https://github.com/cesanta/mongoose) (single-header
  embedded HTTP/WebSocket server — MIT licence, no dependencies).
- **Build target**: `patient_monitor_api` executable (separate from GUI).
- **Endpoints** (JSON):

| Method | Path | Domain call | Description |
|--------|------|-------------|-------------|
| `POST` | `/api/v1/auth/login` | `users_authenticate()` | Issue session token |
| `GET` | `/api/v1/patient` | `patient_*()` | Current patient record |
| `POST` | `/api/v1/patient/reading` | `patient_add_reading()` | Add a VitalSigns reading |
| `GET` | `/api/v1/vitals/latest` | `patient_latest_reading()` | Latest reading + alert status |
| `GET` | `/api/v1/alerts` | `generate_alerts()` | Active alerts |
| `GET` | `/api/v1/sim/reading` | `hw_get_next_reading()` | Simulated reading (sim mode only) |

The API gateway will be the **only component** that needs to change when adding
a new client type. The certified domain layer remains unchanged.

### 2.5 Presentation Layer

Current: **Win32 GUI** (`gui_main.c`) — a single-process C application that
calls `monitor_lib` directly (no HTTP overhead for local use).

Planned clients call the REST API:

| Client | Technology | Transport |
|--------|-----------|-----------|
| Web SPA | React + TypeScript | HTTP/JSON |
| Mobile | React Native or Flutter | HTTP/JSON |
| Embedded dashboard | LVGL on STM32 | Direct C calls to monitor_lib |

---

## 3. Technology Recommendation for Future Clients

### Why REST + JSON over a shared library

| Concern | Direct C library | REST API |
|---------|-----------------|----------|
| Language freedom | C/C++ only | Any language |
| Platform freedom | Must recompile per platform | Any platform with HTTP |
| IEC 62304 re-certification scope | Must re-certify every client | Only API gateway re-certified |
| Deployment | Copy exe | Server once; clients anywhere |
| Testability | Unit tests | Unit + contract tests |

### Recommended web stack

```
monitor_lib (C, certified)
    │
    ▼ linked into
patient_monitor_api (C + mongoose.h)  ← single binary, runs on Windows/Linux
    │ HTTP/JSON on localhost:8080
    ▼
React SPA (TypeScript + Vite)         ← served from the same binary as static files
    │
    ▼ (same API, remote endpoint)
React Native app (iOS + Android)
```

This lets the **certified C core run on the medical device hardware** while the
UI runs on a tablet, phone, or browser — connected over local network or USB
ethernet.

---

## 4. Data Flow (current Win32 client)

```
WM_TIMER (2 s)
    └─► hw_get_next_reading(&v)      [HAL — sim_vitals.c]
            └─► patient_add_reading(&patient, &v)  [domain]
                    └─► generate_alerts(&latest, alerts, MAX_ALERTS)  [domain]
                            └─► update_dashboard(hwnd)  [presentation]
                                    └─► InvalidateRect → WM_PAINT → paint_tiles()
```

---

## 5. Security Architecture

| Concern | Mechanism | Standard |
|---------|-----------|----------|
| Password storage | SHA-256 (FIPS 180-4), hex digest, never plaintext | SWR-SEC-004 |
| File permissions | `_sopen_s(_S_IREAD\|_S_IWRITE)` — owner-only | CWE-732 |
| Role enforcement | `ROLE_ADMIN` / `ROLE_CLINICAL` checked at every privileged action | SWR-SEC-002 |
| Session | In-process `g_app.logged_role` — cleared on logout | SWR-SEC-001 |
| Future API | JWT or session tokens over HTTPS (TLS 1.3) | OWASP API Top 10 |

---

## 6. Scalability Roadmap

| Milestone | Deliverable | Effort |
|-----------|------------|--------|
| **Current (v2.x)** | Win32 GUI, monitor_lib, HAL | — |
| **v3.0 — REST API** | Add mongoose.h adapter + JSON serialisation | 2–3 weeks |
| **v3.1 — Web client** | React SPA connecting to REST API | 3–4 weeks |
| **v3.2 — Mobile** | React Native app (iOS + Android) | 4–6 weeks |
| **v4.0 — Cloud** | API gateway → cloud FHIR store; real-time WebSocket alerts | 8+ weeks |

---

## 7. Directory Structure

```
patient-vital-signs-monitor/
├── include/              # Public C headers (domain + HAL + services)
│   ├── vitals.h          # VitalSigns, AlertLevel, check_* API
│   ├── alerts.h          # Alert, generate_alerts, overall_alert_level
│   ├── patient.h         # PatientRecord, patient_* API
│   ├── hw_vitals.h       # HAL interface (swap for real hardware)
│   ├── gui_users.h       # Account management API
│   ├── gui_auth.h        # Auth façade (backward compat)
│   ├── pw_hash.h         # SHA-256 password hashing
│   └── app_config.h      # Config persistence (sim mode)
├── src/                  # Implementation
│   ├── vitals.c          # Domain: vital sign classification
│   ├── alerts.c          # Domain: alert generation
│   ├── patient.c         # Domain: patient record
│   ├── pw_hash.c         # Domain: SHA-256
│   ├── gui_users.c       # Service: user account management
│   ├── gui_auth.c        # Service: auth façade
│   ├── app_config.c      # Service: config persistence
│   ├── sim_vitals.c      # HAL: simulation back-end
│   ├── gui_main.c        # Presentation: Win32 dashboard
│   └── main.c            # CLI entry point
├── tests/
│   ├── unit/             # 136 unit tests
│   └── integration/      # 12 integration tests
├── dvt/                  # Design Verification Testing
│   ├── DVT_Protocol.md   # DVT-001-REV-A
│   ├── run_dvt.py        # Automated DVT runner
│   └── results/          # Timestamped test reports
├── requirements/         # SWR, SYS, UNS, TRACEABILITY
├── docs/                 # Architecture, design notes
└── .github/workflows/    # CI, DVT, CodeQL, static-analysis, release
```

---

## Revision History

| Rev | Date       | Author        | Description |
|-----|------------|---------------|-------------|
| A   | 2026-04-08 | vinu-engineer | Initial architecture document — v2.1.0 |

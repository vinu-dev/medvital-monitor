# Architecture â€” Patient Vital Signs Monitor

**Document ID:** ARCH-001-REV-A  
**Version:** 2.1.0  
**Date:** 2026-04-08  
**Standard:** IEC 62304 Â§5.3 â€” Software architectural design  

---

## 1. Overview

The Patient Vital Signs Monitor is designed as a **layered, API-first system**
whose domain logic is intentionally decoupled from the presentation layer. This
allows the same clinically-verified C library to power a Win32 desktop client
today and a web or mobile application in the future â€” without touching the
certified core.

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                       PRESENTATION LAYER                         â”‚
â”‚   Win32 GUI (current)  â”‚  Web SPA (future)  â”‚  Mobile (future)  â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                         â”‚ HTTP / JSON REST   (Win32 uses direct calls)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                       API GATEWAY LAYER  (future)                â”‚
â”‚           REST Server â€” libmongoose (single-header C)            â”‚
â”‚           Endpoints: /vitals  /alerts  /patient  /auth           â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                         â”‚ C function calls
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                     DOMAIN SERVICE LAYER  â† IEC 62304 scope      â”‚
â”‚   vitals.c  â”‚  alerts.c  â”‚  patient.c  â”‚  pw_hash.c  â”‚  auth    â”‚
â”‚                        monitor_lib (static library)               â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                         â”‚ HAL interface  (hw_vitals.h)
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                       HARDWARE LAYER                              â”‚
â”‚           sim_vitals.c (current)  â”‚  hw_driver.c (real HW)       â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
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
- No heap allocation â€” all storage is static or stack.
- No OS-specific headers in domain code.
- 100 % branch coverage required (enforced by `--coverage` + CI).

### 2.2 Hardware Abstraction Layer (`hw_vitals.h`)

Two function signatures that any back-end must implement:

```c
void hw_init(void);                       // called once on startup
void hw_get_next_reading(VitalSigns *out); // called every ~2 s
```

Current implementation: `sim_vitals.c` (20-entry clinical scenario table).  
Production replacement: `hw_driver.c` â€” reads from a real medical sensor
(serial, USB HID, ADC) with **zero changes** to any other file.

### 2.3 Application Services Layer

Cross-cutting concerns that live between domain and presentation:

| Module | File | Responsibility |
|--------|------|----------------|
| Config persistence | `app_config.c` | Read/write `monitor.cfg` (sim mode state) |
| Auth faÃ§ade | `gui_auth.c` | Thin wrapper mapping legacy `auth_*` API to `gui_users.c` |

### 2.4 API Gateway Layer *(planned â€” not yet implemented)*

To expose the domain to web and mobile clients, a thin REST adapter will
wrap `monitor_lib`. Recommended implementation:

- **Library**: [libmongoose](https://github.com/cesanta/mongoose) (single-header
  embedded HTTP/WebSocket server â€” MIT licence, no dependencies).
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

Current: **Win32 GUI** (`gui_main.c`) â€” a single-process C application that
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
    â”‚
    â–¼ linked into
patient_monitor_api (C + mongoose.h)  â† single binary, runs on Windows/Linux
    â”‚ HTTP/JSON on localhost:8080
    â–¼
React SPA (TypeScript + Vite)         â† served from the same binary as static files
    â”‚
    â–¼ (same API, remote endpoint)
React Native app (iOS + Android)
```

This lets the **certified C core run on the medical device hardware** while the
UI runs on a tablet, phone, or browser â€” connected over local network or USB
ethernet.

---

## 4. Data Flow (current Win32 client)

```
WM_TIMER (2 s)
    â””â”€â–º hw_get_next_reading(&v)      [HAL â€” sim_vitals.c]
            â””â”€â–º patient_add_reading(&patient, &v)  [domain]
                    â””â”€â–º generate_alerts(&latest, alerts, MAX_ALERTS)  [domain]
                            â””â”€â–º update_dashboard(hwnd)  [presentation]
                                    â””â”€â–º InvalidateRect â†’ WM_PAINT â†’ paint_tiles()
```

---

## 5. Security Architecture

| Concern | Mechanism | Standard |
|---------|-----------|----------|
| Password storage | SHA-256 (FIPS 180-4), hex digest, never plaintext | SWR-SEC-004 |
| File permissions | `_sopen_s(_S_IREAD\|_S_IWRITE)` â€” owner-only | CWE-732 |
| Role enforcement | `ROLE_ADMIN` / `ROLE_CLINICAL` checked at every privileged action | SWR-SEC-002 |
| Session | In-process `g_app.logged_role` â€” cleared on logout | SWR-SEC-001 |
| Future API | JWT or session tokens over HTTPS (TLS 1.3) | OWASP API Top 10 |

---

## 6. Scalability Roadmap

| Milestone | Deliverable | Effort |
|-----------|------------|--------|
| **Current (v2.x)** | Win32 GUI, monitor_lib, HAL | â€” |
| **v3.0 â€” REST API** | Add mongoose.h adapter + JSON serialisation | 2â€“3 weeks |
| **v3.1 â€” Web client** | React SPA connecting to REST API | 3â€“4 weeks |
| **v3.2 â€” Mobile** | React Native app (iOS + Android) | 4â€“6 weeks |
| **v4.0 â€” Cloud** | API gateway â†’ cloud FHIR store; real-time WebSocket alerts | 8+ weeks |

---

## 7. Directory Structure

```
patient-vital-signs-monitor/
â”œâ”€â”€ include/              # Public C headers (domain + HAL + services)
â”‚   â”œâ”€â”€ vitals.h          # VitalSigns, AlertLevel, check_* API
â”‚   â”œâ”€â”€ alerts.h          # Alert, generate_alerts, overall_alert_level
â”‚   â”œâ”€â”€ patient.h         # PatientRecord, patient_* API
â”‚   â”œâ”€â”€ hw_vitals.h       # HAL interface (swap for real hardware)
â”‚   â”œâ”€â”€ gui_users.h       # Account management API
â”‚   â”œâ”€â”€ gui_auth.h        # Auth faÃ§ade (backward compat)
â”‚   â”œâ”€â”€ pw_hash.h         # SHA-256 password hashing
â”‚   â””â”€â”€ app_config.h      # Config persistence (sim mode)
â”œâ”€â”€ src/                  # Implementation
â”‚   â”œâ”€â”€ vitals.c          # Domain: vital sign classification
â”‚   â”œâ”€â”€ alerts.c          # Domain: alert generation
â”‚   â”œâ”€â”€ patient.c         # Domain: patient record
â”‚   â”œâ”€â”€ pw_hash.c         # Domain: SHA-256
â”‚   â”œâ”€â”€ gui_users.c       # Service: user account management
â”‚   â”œâ”€â”€ gui_auth.c        # Service: auth faÃ§ade
â”‚   â”œâ”€â”€ app_config.c      # Service: config persistence
â”‚   â”œâ”€â”€ sim_vitals.c      # HAL: simulation back-end
â”‚   â”œâ”€â”€ gui_main.c        # Presentation: Win32 dashboard
â”‚   â””â”€â”€ main.c            # CLI entry point
â”œâ”€â”€ tests/
â”‚   â”œâ”€â”€ unit/             # 307 unit tests
â”‚   â””â”€â”€ integration/      # 14 integration tests
â”œâ”€â”€ dvt/                  # Design Verification Testing
â”‚   â”œâ”€â”€ DVT_Protocol.md   # DVT-001-REV-A
â”‚   â”œâ”€â”€ run_dvt.py        # Automated DVT runner
â”‚   â””â”€â”€ results/          # Timestamped test reports
â”œâ”€â”€ requirements/         # SWR, SYS, UNS, TRACEABILITY
â”œâ”€â”€ docs/                 # Architecture, design notes
â””â”€â”€ .github/workflows/    # CI, DVT, CodeQL, static-analysis, release
```

---

## Revision History

| Rev | Date       | Author        | Description |
|-----|------------|---------------|-------------|
| A   | 2026-04-08 | vinu-engineer | Initial architecture document â€” v2.1.0 |

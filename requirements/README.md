# Requirements — Maintenance Guide

**Project:** Patient Vital Signs Monitor
**Standard:** IEC 62304 §5.2, §5.7.3 / 21 CFR 820.30

---

## Document Map

```
requirements/
├── UNS.md           User Needs Specification         (what the clinician needs)
├── SYS.md           System Requirements Specification (what the system must do)
├── SWR.md           Software Requirements Specification (what the code must do)
├── TRACEABILITY.md  Requirements Traceability Matrix  (links everything together)
└── README.md        This file — maintenance guide
```

---

## ID Scheme

| Prefix     | Document | Example       | Meaning                          |
|------------|----------|---------------|----------------------------------|
| `UNS-NNN`  | UNS.md   | `UNS-001`     | User need, three-digit number    |
| `SYS-NNN`  | SYS.md   | `SYS-003`     | System requirement               |
| `SWR-VIT-NNN` | SWR.md | `SWR-VIT-001` | Software req — vitals module     |
| `SWR-ALT-NNN` | SWR.md | `SWR-ALT-002` | Software req — alerts module     |
| `SWR-PAT-NNN` | SWR.md | `SWR-PAT-004` | Software req — patient module    |

**Rules:**
- IDs are permanent — once assigned, never reuse a number even if the requirement is deleted.
- Deleted requirements are marked `**[RETIRED]**` with a retirement date and reason.
- New requirements get the next available number in sequence.

---

## How to Add a New Requirement

### Step 1 — Add the User Need (if new)
Open `UNS.md` and add a new `### UNS-NNN` section following the existing format.
Include: **Need**, **Rationale**, **Priority**.

### Step 2 — Add or update a System Requirement
Open `SYS.md`. Add a new `### SYS-NNN` section or update an existing one.
Each SYS entry must state **Traces to: UNS-NNN**.

### Step 3 — Add the Software Requirement
Open `SWR.md`. Add to the appropriate module section.
Each SWR entry must state:
- **Traces to:** SYS-NNN
- **Implemented in:** filename and function name
- **Verified by:** test file and test suite pattern

### Step 4 — Update the code
Add a Doxygen requirement annotation such as `@par Requirement` followed by the
`SWR-XXX-NNN` ID on the relevant public API declaration and implementation
comments where applicable.

### Step 5 — Add the test
Write a Google Test case. Name it following the pattern:
`TEST(SuiteName, REQ_XXX_NNN_Description)`.

### Step 6 — Update the traceability matrix
Open `TRACEABILITY.md` and:
1. Add a row to **Section 1** (forward trace).
2. Add a row to **Section 2** (backward trace).
3. Add the UNS to **Section 3** if new, or mark existing UNS as still covered.
4. Add the SWR to **Section 4** with its test count.
5. Update **Section 5** test count totals.

### Step 7 — Commit with a meaningful message
```
git add requirements/ src/ tests/
git commit -m "Add SWR-VIT-008: <description>

Traces: UNS-005 -> SYS-018 -> SWR-VIT-008
Implemented in: src/vitals.c
Verified by: tests/unit/test_vitals.cpp"
```

---

## How to Change an Existing Requirement

1. Update the requirement text in the relevant `.md` file.
2. Increment the document revision in the header table (Rev A → Rev B).
3. If the change affects the implementation, update the source code and `@req` tags.
4. If the change affects test coverage, update or add tests.
5. Update `TRACEABILITY.md` if function names or test IDs change.
6. Commit all changes together in a single git commit so the change is atomic.

---

## How to Retire a Requirement

Never delete a requirement ID. Instead:

1. Replace the requirement body with:
   ```
   **[RETIRED]** Retired 2026-MM-DD. Reason: <reason>.
   Replaced by: SWR-XXX-NNN (if applicable).
   ```
2. In `TRACEABILITY.md` mark the row with `[RETIRED]` in the Covered? column.
3. Remove `@req` tags for this ID from the source code.
4. Keep the associated tests but mark them `DISABLED_` if no longer applicable.

---

## Audit Checklist

Before submitting for regulatory review, verify:

- [ ] Every `UNS-NNN` in `UNS.md` has at least one row in TRACEABILITY Section 3
- [ ] Every `SYS-NNN` in `SYS.md` appears in at least one SWR's "Traces to" field
- [ ] Every `SWR-NNN` in `SWR.md` has an "Implemented in" and "Verified by" field
- [ ] Every `SWR-NNN` appears in TRACEABILITY Section 4 with a test count > 0
- [ ] All requirement annotations in code comments refer to valid SWR IDs in `SWR.md`
- [ ] All Google Test names follow the `REQ_XXX_NNN_Description` pattern
- [ ] `docs/doxygen_warnings.log` is empty (no undocumented or malformed comments)
- [ ] All 307 tests pass (`run_tests.bat`)
- [ ] `AuthValidation.*` and `AuthDisplayName.*` (15 tests) all pass
- [ ] Line coverage ≥ 99% for `vitals.c`, `alerts.c`, `patient.c` (`run_coverage.bat`)
- [ ] No dynamic memory allocation in production code
- [ ] Git history is clean — one commit per change, meaningful messages

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Updated for v1.3 GUI additions |
| C   | 2026-05-05 | codex           | Updated RR example trace for SYS-018 / SWR-VIT-008 repair |
| D   | 2026-05-06 | Codex implementer | Updated checklist totals for session-reset disclosure coverage |

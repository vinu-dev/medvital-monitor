# Risk Note: Issue #71

Date: 2026-05-06
Issue: `#71` - Feature: add a large-font readability mode
Branch: `feature/71-large-font-readability-mode`

## proposed change

Add an optional large-font readability mode for the Win32 dashboard so the most
important vital values and labels are easier to read from a short distance.
The approved risk boundary is presentation-layer only:

- no change to measured values, thresholds, NEWS2 scoring, alert-generation
  logic, colors, or data flow
- no change to alarms, patient records, or hardware acquisition
- dashboard-only scope with optional settings persistence if persistence is
  implemented through the existing configuration path

Repo evidence supports that this is technically localizable to the presentation
layer: `src/gui_main.c` centralizes tile painting in `paint_tile()` and
`paint_tiles()`, the status banner in `paint_status_banner()`, and existing
settings persistence is already handled through `src/app_config.c`.

## product hypothesis and intended user benefit

Hypothesis: when clinicians or demo operators are standing away from the
workstation, increasing the size of the key dashboard numerics and labels will
reduce visual strain and make rapid status recognition easier without changing
clinical meaning.

Intended user benefit:

- faster glance readability for bedside review and demonstrations
- reduced dependence on dense default layout when distance matters more than
  information density
- no retraining on new alert semantics if the mode preserves the current color
  and status vocabulary

## source evidence quality

Source evidence is adequate for product-discovery precedent, not for clinical
effectiveness claims.

- The issue cites a public Mindray DPM 5 operator manual describing a "Large
  Font Screen" that enlarges HR, SpO2, and NIBP values. This is useful evidence
  that large-font monitor views are an established UI pattern in the market.
- It is still a single competitor/operator-manual source, not independent human
  factors evidence and not clinical validation.
- The source is therefore sufficient to justify a narrow non-copying MVP
  exploration, but insufficient to claim improved clinical outcomes,
  accessibility compliance, or reduced medical risk by itself.

## MVP boundary that avoids copying proprietary UX

- Use one local display preference such as "Large Font Mode" or "Readability
  Mode" rather than copying a competitor face-selection workflow.
- Reuse the project's existing tile, banner, and settings architecture instead
  of cloning a competitor screen composition, terminology, or menu behavior.
- Limit the MVP to enlarging already displayed dashboard text and reflowing
  layout as needed; do not duplicate another product's exact grouping,
  navigation, or waveform placement.
- Keep any persistence implementation within the existing `monitor.cfg` style
  rather than inventing a new settings subsystem.

## clinical-safety boundary and claims that must not be made

- This feature must be treated as a human-factors presentation option only.
- It must not be described as changing clinical sensitivity, alarm performance,
  diagnostic capability, or patient safety by itself.
- It must not claim accessibility compliance, low-vision accommodation, or safe
  use at any specific viewing distance unless the team later defines and
  validates those claims explicitly.
- It must not alter or suppress values, units, colors, alert badges, or warning
  hierarchy in order to make text larger.

## medical-safety impact

Direct clinical logic impact is low because the intended change does not modify
vital-sign classification, NEWS2, alert thresholds, or patient data handling.

The meaningful safety risk is display integrity: if larger text causes clipping,
overlap, hidden units, hidden alert badges, or misread color/status cues, a
clinician could misinterpret the current patient state or miss a deterioration
signal. This is therefore a low-code-risk but non-zero human-factors-risk
feature.

## security and privacy impact

No new patient-data flow, credential, or RBAC behavior is needed.

If the mode is persisted, the persistence should stay in the existing local
configuration file path and store only a UI preference. No new PHI should be
stored, transmitted, or exposed. Security and privacy impact is low.

## affected requirements or "none"

No currently approved requirement explicitly covers a user-selectable large-font
mode. Design should not assume this feature is already approved under existing
GUI text.

Adjacent approved requirements likely affected by downstream design/spec work:

- `UNS-010` consolidated status summary
- `UNS-014` graphical user interface
- `SYS-011` patient status summary display
- `SYS-014` graphical vital signs dashboard
- `SWR-GUI-003` colour-coded vital signs display
- `SWR-GUI-004` patient data entry via GUI
- `SWR-GUI-010` simulation mode toggle, if a persisted readability preference
  shares the same configuration surface
- `SWR-GUI-012` localization/persistence patterns, if the readability label is
  localized and stored through the existing config path

Architect/spec work should add explicit requirement text rather than stretching
the current baseline by implication.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:
trained clinical staff or demo operators use an optional readability mode to
view the same live dashboard information more clearly on a Windows workstation.

User population:
bedside clinicians, ward nurses, intensivists, and product/demo operators
already expected to understand the existing alert colors and terminology.

Operating environment:
desktop or bedside workstation use in the same simulated monitoring environment
already described by the repository; no new hardware context is introduced.

Foreseeable misuse:

- user assumes larger text changes alarm severity or value meaning
- text enlargement causes truncated digits, units, or labels on smaller windows
- user leaves the mode enabled on a display size where some fields no longer fit
- future implementer enlarges values but reduces or hides color/status cues to
  preserve layout density

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity:
moderate, because a display-integrity defect could contribute to delayed or
incorrect recognition of an abnormal patient state even though core calculations
remain unchanged.

Probability:
occasional before controls, because GUI reflow and clipping defects are common
when enlarging text in fixed-layout desktop UIs.

Initial risk:
medium.

Risk controls:

- preserve identical numeric values, units, alert levels, and color semantics in
  both standard and large-font modes
- ensure every critical dashboard field remains fully visible at supported
  window sizes and font scales
- keep the mode optional and user-invoked rather than auto-switching
- keep scope to the dashboard; do not introduce hidden alternate workflows
- use existing settings persistence infrastructure only if the preference can be
  saved without affecting unrelated config behavior
- define supported resolutions/window states explicitly in the design

Verification method:

- targeted GUI/manual verification that no tile clips or overlaps values, units,
  labels, sparkline areas, or alert cues in either mode
- regression checks that alert colors, banners, and statuses are unchanged for
  identical input data
- config persistence verification only if the preference is persisted
- changed-file review confirming the implementation remains presentation-layer
  only

Residual risk:
low if the controls above are implemented and verified.

Residual-risk acceptability rationale:
acceptable for this pilot because the feature is optional, presentation-only,
and can be bounded to unchanged clinical semantics with focused GUI validation.
Residual risk becomes unacceptable if the design attempts to claim accessibility
or clinical-benefit performance without explicit human validation criteria.

## hazards and failure modes

- clipped or obscured vital values cause a clinician to misread the current
  measurement
- hidden units or labels make enlarged numbers ambiguous
- enlarged layout pushes alert badges, status text, or NEWS2 information off the
  visible dashboard region
- color contrast or background handling changes during rework and weakens alert
  recognition
- persistence restores a broken layout on startup with no easy recovery path
- competitor-inspired design drift leads to copied workflow or UI language
  instead of an original bounded implementation

## existing controls

- The issue already constrains scope to font scale/density rather than clinical
  logic changes.
- `README.md`, `requirements/SYS.md`, and `requirements/SWR.md` establish that
  alert semantics, thresholds, and dashboard color meaning already exist and
  should remain unchanged.
- `src/gui_main.c` keeps tile drawing and banner drawing centralized, which
  reduces the risk of inconsistent per-widget behavior.
- `src/app_config.c` already persists other UI preferences in a bounded local
  config file format.
- The issue provides explicit validation expectations for build/tests plus manual
  GUI review.

## required design controls

- Define the mode as dashboard presentation only, with no clinical-algorithm
  changes.
- Specify exactly which dashboard elements enlarge, which stay fixed, and what
  may reflow.
- Preserve all existing alert colors, status words, and units.
- Define supported minimum window dimensions or fixed dashboard geometry for the
  large-font layout.
- If persistence is included, define a safe default and a clear recovery path if
  the stored preference produces an unusable layout.
- Require manual GUI verification evidence for both standard and large-font
  views before release.
- Keep proprietary mimicry out of scope; precedent may inform the problem but
  not the final UX structure.

## validation expectations

- confirm the changed files stay within `src/gui_main.c`, localization/config
  support if needed, tests, and documentation; no core clinical modules should
  need modification
- verify identical readings produce identical alert levels, banner states, and
  NEWS2 scores before and after enabling the mode
- manually inspect all dashboard tiles for clipping, overlap, and preserved
  unit visibility in the supported layout
- verify the mode does not hide the alerts list, patient bar, or other safety-
  relevant status cues required for the pilot workflow
- if persistence is implemented, verify restart behavior and ability to recover
  from an unsupported display geometry

## residual risk for this pilot

Low, provided the feature remains a bounded presentation-layer option and the
team treats clipping/visibility as the primary safety concern rather than
assuming "font only" implies zero risk.

## whether the candidate is safe to send to Architect

Yes, with constraints. This issue is safe to send to Architect as a
presentation-layer feature candidate if the design stays within the boundaries
above and adds explicit requirement text for the new UI behavior and any
settings persistence.

## human owner decision needed, if any

The human owner should confirm two boundaries before implementation begins:

- the feature is a readability/convenience mode, not a claimed accessibility or
  clinical-performance feature
- dashboard-only scope is acceptable for the MVP, even if other screens remain
  unchanged

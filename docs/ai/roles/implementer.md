# Implementer Role

## Mission

Implement one approved spec with minimal, traceable changes and matching tests.

## Inputs

Read:

- The GitHub issue
- `docs/history/specs/<issue>-<slug>.md`
- `CLAUDE.md`
- The source, header, tests, and requirement files named by the spec

If the Agentry Work Packet names a `Selected Candidate`, implement or repair
that issue only in this run. Do not inspect or relabel another queued issue
unless it directly blocks the selected issue.

## Coding Rules

- Keep production C code compatible with C11.
- Keep tests compatible with the existing Google Test layout.
- Preserve the no-heap-allocation production design unless a spec explicitly
  approves otherwise.
- Do not change clinical thresholds, NEWS2 scoring, alarm-limit behavior,
  authentication, or password hashing beyond the approved spec.
- Do not edit generated Doxygen HTML/XML directly.
- Use explicit `git add` paths. Do not use `git add -A`.
- Never push to `main`.

## Traceability Rules

If behavior changes, update the relevant requirement and traceability docs in
the same branch:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- README module/test-count summaries when affected

If requirements do not change, state that in the implementation comment.

## Validation Matrix

Run the smallest relevant set first, then expand when touching shared logic:

| Changed paths | Required validation |
| --- | --- |
| `src/*.c`, `include/*.h`, `tests/**`, `CMakeLists.txt` | `cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON -Wno-dev`; `cmake --build build --target test_unit test_integration`; `ctest --test-dir build --output-on-failure` |
| `requirements/**`, `docs/ARCHITECTURE.md`, README traceability sections | Review forward/backward traceability and update affected requirement IDs |
| `.github/workflows/**`, `scripts/**` | Validate syntax where possible and document any CI-only checks |
| `dvt/**`, GUI behavior | Run unit/integration tests and document DVT/manual verification needed |
| `installer.iss`, release scripts | Build only when tooling is available; otherwise document exact skipped command |

## Handoff

On success, push the feature branch and move the issue to `ready-for-test`.
On validation failure, keep or add `tests-failed` with concise failure output.

---
name: add-arq-unit-tests
description: Add, extend, or repair ARQ C++ unit tests using the repository's GoogleTest/GoogleMock, CMake, test-entry-point, naming, and build conventions. Use for requests to test ARQLib code, add regression coverage for a C++ change or bug, create a new library test target, improve deterministic financial or snapshot tests, or diagnose how an ARQ test should be structured and run.
---

# Add ARQ Unit Tests

Add focused, deterministic tests that exercise public behavior and fit the owning library's existing test target.

## Inspect Before Editing

1. Read the applicable `AGENTS.md`, `README.md`, `docs/product/vision.md`, `ARCHITECTURE.md`, and `docs/roadmap/roadmap.md` before material work.
2. Inspect the implementation, public header, dependencies, and call sites of the behavior under test. Do not infer capability from filenames or scaffolding.
3. Identify the owning `ARQLib/<Library>` target from its `CMakeLists.txt` and actual target dependencies.
4. Inspect the owning `test/` directory, especially `main.cpp` and nearby `t_*.cpp` files. Treat current local patterns as authoritative when they differ from examples here.
5. Check `git status` and preserve unrelated user changes.

## Fit the ARQ Test Layout

- Put unit-test sources in `ARQLib/<Library>/test/`.
- Name each new test source `t_<subject>.cpp`, using lowercase snake case consistent with nearby files.
- Keep exactly one `main.cpp` per test executable. Never add `main()` to a `t_*.cpp` file.
- Include `<gtest/gtest.h>` for GoogleTest and `<gmock/gmock.h>` only when mocks or matchers require it.
- Use `TEST` for self-contained cases and `TEST_F` when setup, teardown, or shared helpers materially improve isolation.
- Match existing C++ formatting and naming. Follow `.editorconfig` and `CODE_STYLE.md` when present.

The root CMake function `ARQ_define_dynalib_tests(<Library>)` currently:

- globs `test/*.cpp` and `test/*.h`;
- creates the `t_<Library>` executable;
- links the library plus GoogleTest and GoogleMock;
- registers cases with `gtest_discover_tests`.

When that function is already called by the library's `CMakeLists.txt`, add the test file without listing it manually. Because the glob does not use `CONFIGURE_DEPENDS`, reconfigure after adding or renaming a test file. Do not modify the shared CMake helper unless the requested work genuinely changes the build system.

If a library has no test target, inspect adjacent ARQLib libraries before adding one. Add `ARQ_define_dynalib_tests(<Library>)` to the owning CMake file and create one local `test/main.cpp` whose initialization matches that library. Existing entry points are not interchangeable: `ARQUtils` installs a test logger, many libraries create an `ARQ::LibGuard`, and some targets customize test log paths. Copy the nearest valid pattern only after checking its dependencies and runtime needs.

## Design Useful Tests

1. Translate the requested behavior or bug into observable inputs, outputs, state changes, and failure conditions.
2. Cover the smallest meaningful behavior first, then add relevant boundary, invalid-input, and regression cases.
3. Use `ASSERT_*` when later statements require the condition to hold; use `EXPECT_*` for independent checks that can continue safely.
4. Keep arrange, act, and assert sections easy to see without ceremonial comments.
5. Prefer public APIs. Use internal seams only when public behavior cannot establish the required state or observation.
6. Give suites and cases behavior-focused names such as `MarketSnapshotTest` and `RejectsOlderRevision`.
7. Do not add empty, commented-out, timing-sensitive, or environment-dependent tests.

Make unit tests deterministic:

- use fixed timestamps, IDs, inputs, calendars, snapshot revisions, and random seeds;
- avoid wall-clock sleeps, live network calls, databases, Kafka, NATS, Redis, ClickHouse, and machine-specific files;
- inject or fake boundaries rather than testing external infrastructure in an ordinary unit test;
- restore singleton registrations, factories, environment state, and global callbacks in `TearDown` or RAII cleanup so cases remain order-independent;
- test exact immutable reference-data, market, and model inputs for financial calculations;
- distinguish business-effective, source, recorded, and processing time when temporal behavior is relevant;
- test replay, duplicates, ordering, restart, and failure windows at the appropriate service, projection, contract, or integration-test level. Do not claim a mock proves cross-system recovery.

For a bug regression, make the test fail for the specific incorrect behavior and pass for the intended contract. Avoid assertions that merely execute code or restate the implementation.

## Build and Verify

Use the repository build entry points for normal C++ operations; do not invoke CMake, CTest, or scripts under `scripts/` directly.

On Windows, after adding or renaming a test source:

```powershell
.\bld.bat c
.\bld.bat b
.\bld.bat t
```

On Linux or macOS:

```bash
./bld.sh c
./bld.sh b
./bld.sh t
```

Append `d` to each command for a Debug run when appropriate. If only an already-discovered test source changed, omit configure unless CMake inputs changed.

When verification fails:

1. Read the first relevant compiler or test failure, not only the final summary.
2. Decide whether the failure exposes a test mistake, a real product defect, stale configuration, or unavailable external infrastructure.
3. Fix only work within the user's requested scope. Do not weaken assertions to accommodate incorrect behavior.
4. Rerun the narrowest supported build step, then the full test command before handoff when practical.

Report the tests added, behavior covered, commands run, and any unverified or integration-dependent coverage. Clearly distinguish passing unit coverage from behavior that still requires real infrastructure testing.

# C++/Qt Rewrite Completion Audit

Audit date: 2026-05-20

## Objective

Rewrite gMKVExtractGUI as a C++/Qt desktop app with functional UI parity and
Linux/Windows support, while preserving Linux `mkvmerge`/`mkvinfo`/`mkvextract`
integration, Windows MKVToolNix detection, localization, settings, jobs,
extraction naming, tests, and packaging documentation.

## Prompt-To-Artifact Checklist

| Requirement | Evidence | Status |
| --- | --- | --- |
| C++ desktop app as active product | Root `CMakeLists.txt`, `src/gMKVToolNix.Cpp`, `src/gMKVExtractGUI.Cpp`, `tests/gMKVToolNix.Cpp.Tests` | Implemented |
| Qt 6 Widgets and CMake | `find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)`, `qt_add_executable(gMKVExtractGUIQt)` | Implemented |
| Main window workflow | `MainWindow` covers tool path, input tree, output options, extraction modes, jobs, logs, options, abort controls | Implemented; covered by headless structural smoke |
| Options, Log, Job Manager, Translation Editor | Dedicated Qt dialogs exist and are constructed by `--smoke-test`, which asserts representative controls in each window | Implemented; covered by headless structural smoke |
| Tool auto-detection | Linux resolves `mkvmerge`, `mkvinfo`, and `mkvextract` directly through saved/app locations or PATH; Windows keeps MKVToolNix directory and registry discovery | Implemented; Linux behavior validated |
| Analyze MKV files | `SegmentAnalyzer`, `MkvMergeService`, `MkvInfoService`, and `gMKVExtractGUIQt --analyze-smoke-test` against the generated Linux fixture | Implemented; Linux app-core smoke validated |
| Extract supported elements/modes | `MkvExtractPlanner`, `MkvExtractRunner`, extraction naming tests, Linux smoke extracts tracks, timestamps, cues, chapters, tags, attachments, and CUE sheet | Implemented; combined-mode GUI parity still needs manual UI evidence |
| Filename pattern parity | C++ filename pattern defaults, placeholder replacement, extension mapping, collision tests | Automated tests pass |
| Settings compatibility | Legacy settings load/save tests and GUI settings wiring | Automated tests pass |
| Localization compatibility | Existing JSON files staged from `src/gMKVExtractGUI.Cpp/resources/locales`; fallback, aliases, bad format, sync/template tests; UI strings use JSON keys | Automated tests pass |
| Jobs | XML save/load, queue duplicate rejection, Job Manager controller wiring | Automated tests pass; manual restart parity evidence still pending |
| Responsive long-running work and abort | Analysis thread, extraction controller, abort/abort-all wiring | Implemented; manual long-run evidence pending |
| Linux build/launch/package | Configure/build/ctest/install passed on Linux Mint 22.3; `scripts/cpp_manual_smoke.sh` passed | Validated |
| Windows build/launch/package | CMake commands, `windeployqt` docs, Windows registry discovery, Windows icon resource | Implemented paths documented; smoke/build was skipped by user request and is not host-validated |
| Manual parity testing documented | `docs/CPP_MANUAL_PARITY_CHECKLIST.md` | Documented |
| Commit gate | Work commit `4df4ef7 feat(cpp): add native Qt rewrite` | Completed |

## Current Completion Decision

The C++ application has validated Linux evidence and active-tree packaging docs.
Windows host validation and full manual GUI parity rows still need dated
evidence before a release claim.
